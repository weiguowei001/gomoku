#include "ai.h"

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

}  // namespace

AiPlayer::AiPlayer() : rng_(std::random_device{}()) {}

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

    return chooseRandomMove(board);
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

std::optional<std::pair<int, int>> AiPlayer::chooseRandomMove(const Board& board) {
    const auto emptyCells = collectEmptyCells(board);
    if (emptyCells.empty()) {
        return std::nullopt;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(emptyCells.size()) - 1);
    return emptyCells[dist(rng_)];
}
