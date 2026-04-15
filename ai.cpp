#include "ai.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {

std::vector<std::pair<int, int>> collectEmptyCells(const Board& board) {
    std::vector<std::pair<int, int>> emptyCells;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            if (board.isEmpty(row, col)) {
                emptyCells.emplace_back(row, col);
            }
        }
    }
    return emptyCells;
}

int contiguousCount(const Board& board, int row, int col, int dRow, int dCol, Piece piece) {
    int count = 0;
    int currentRow = row + dRow;
    int currentCol = col + dCol;
    while (board.isInside(currentRow, currentCol) &&
           board.pieceAt(currentRow, currentCol) == piece) {
        ++count;
        currentRow += dRow;
        currentCol += dCol;
    }
    return count;
}

int evaluateLine(const Board& board, int row, int col, int dRow, int dCol, Piece piece) {
    const int forward = contiguousCount(board, row, col, dRow, dCol, piece);
    const int backward = contiguousCount(board, row, col, -dRow, -dCol, piece);
    const int total = forward + backward;

    // More than 4 means the candidate can immediately complete or approach five.
    int score = total * total * 15;
    if (total >= 3) {
        score += 100;
    }
    if (total >= 2) {
        score += 40;
    }
    return score;
}

int evaluatePosition(const Board& board, int row, int col, Piece aiPiece, Piece opponentPiece) {
    static const std::array<std::array<int, 2>, 4> directions = {{
        {{1, 0}}, {{0, 1}}, {{1, 1}}, {{1, -1}}
    }};

    int attackScore = 0;
    int defenseScore = 0;
    for (const auto& [dRow, dCol] : directions) {
        attackScore += evaluateLine(board, row, col, dRow, dCol, aiPiece);
        defenseScore += evaluateLine(board, row, col, dRow, dCol, opponentPiece);
    }

    // Prefer central area in the early / neutral stage.
    const int center = Board::kSize / 2;
    const int centerDistance = std::abs(row - center) + std::abs(col - center);
    const int centerBonus = std::max(0, 10 - centerDistance);

    // Defense is slightly stronger to reduce missed blocks of growing threats.
    return attackScore + defenseScore * 12 / 10 + centerBonus * 2;
}

int bestPositionScore(const Board& board, Piece aiPiece, Piece opponentPiece) {
    int best = std::numeric_limits<int>::min();
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            if (!board.isEmpty(row, col)) {
                continue;
            }
            best = std::max(best, evaluatePosition(board, row, col, aiPiece, opponentPiece));
        }
    }
    return best;
}

}  // namespace

AiPlayer::AiPlayer() : rng_(std::random_device{}()) {}

void AiPlayer::setDifficulty(AiDifficulty difficulty) {
    difficulty_ = difficulty;
}

AiDifficulty AiPlayer::difficulty() const {
    return difficulty_;
}

std::optional<std::pair<int, int>> AiPlayer::chooseMove(const Board& board,
                                                        Piece aiPiece,
                                                        Piece opponentPiece) {
    const auto winMove = findWinningMove(board, aiPiece);
    if (winMove.has_value()) {
        return winMove;
    }

    const auto blockMove = findWinningMove(board, opponentPiece);
    if (blockMove.has_value()) {
        return blockMove;
    }

    if (difficulty_ == AiDifficulty::Easy) {
        return chooseBestScoredMove(board, aiPiece, opponentPiece);
    }
    if (difficulty_ == AiDifficulty::Hard) {
        return chooseHardMove(board, aiPiece, opponentPiece);
    }
    return chooseBestScoredMove(board, aiPiece, opponentPiece);
}

std::optional<std::pair<int, int>> AiPlayer::findWinningMove(const Board& board, Piece piece) const {
    const auto emptyCells = collectEmptyCells(board);
    for (const auto& [row, col] : emptyCells) {
        Board testBoard = board;
        if (!testBoard.placePiece(row, col, piece)) {
            continue;
        }
        if (testBoard.isWinningMove(row, col, piece)) {
            return std::make_pair(row, col);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<int, int>> AiPlayer::chooseBestScoredMove(const Board& board,
                                                                  Piece aiPiece,
                                                                  Piece opponentPiece) {
    const auto emptyCells = collectEmptyCells(board);
    if (emptyCells.empty()) {
        return std::nullopt;
    }

    int bestScore = std::numeric_limits<int>::min();
    std::vector<std::pair<int, int>> bestMoves;
    bestMoves.reserve(emptyCells.size());

    for (const auto& [row, col] : emptyCells) {
        const int score = evaluatePosition(board, row, col, aiPiece, opponentPiece);
        if (score > bestScore) {
            bestScore = score;
            bestMoves.clear();
            bestMoves.emplace_back(row, col);
        } else if (score == bestScore) {
            bestMoves.emplace_back(row, col);
        }
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(bestMoves.size()) - 1);
    const auto picked = bestMoves[dist(rng_)];

    if (difficulty_ == AiDifficulty::Easy) {
        // Easy mode: occasionally makes a weaker random move to feel more beatable.
        std::uniform_int_distribution<int> chance(1, 100);
        if (chance(rng_) <= 35) {
            std::uniform_int_distribution<int> any(0, static_cast<int>(emptyCells.size()) - 1);
            return emptyCells[any(rng_)];
        }
    }

    return picked;
}

std::optional<std::pair<int, int>> AiPlayer::chooseHardMove(const Board& board,
                                                            Piece aiPiece,
                                                            Piece opponentPiece) {
    const auto emptyCells = collectEmptyCells(board);
    if (emptyCells.empty()) {
        return std::nullopt;
    }

    int bestScore = std::numeric_limits<int>::min();
    std::vector<std::pair<int, int>> bestMoves;
    bestMoves.reserve(emptyCells.size());

    for (const auto& [row, col] : emptyCells) {
        Board simulated = board;
        if (!simulated.placePiece(row, col, aiPiece)) {
            continue;
        }

        // Base score: immediate board quality for AI.
        int score = evaluatePosition(board, row, col, aiPiece, opponentPiece);

        // Penalize strong opponent reply in next turn (1-ply lookahead).
        const int opponentReply = bestPositionScore(simulated, opponentPiece, aiPiece);
        if (opponentReply != std::numeric_limits<int>::min()) {
            score -= opponentReply * 7 / 10;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMoves.clear();
            bestMoves.emplace_back(row, col);
        } else if (score == bestScore) {
            bestMoves.emplace_back(row, col);
        }
    }

    if (bestMoves.empty()) {
        return std::nullopt;
    }
    std::uniform_int_distribution<int> dist(0, static_cast<int>(bestMoves.size()) - 1);
    return bestMoves[dist(rng_)];
}
