#ifndef GOMOKU_AI_H
#define GOMOKU_AI_H

#include <optional>
#include <random>
#include <utility>

#include "board.h"

class AiPlayer {
public:
    AiPlayer();

    // Strategy: win first, block second, score-based best move otherwise.
    std::optional<std::pair<int, int>> chooseMove(const Board& board,
                                                  Piece aiPiece,
                                                  Piece opponentPiece);

private:
    std::optional<std::pair<int, int>> findWinningMove(const Board& board, Piece piece) const;
    std::optional<std::pair<int, int>> chooseBestScoredMove(const Board& board,
                                                            Piece aiPiece,
                                                            Piece opponentPiece);

private:
    std::mt19937 rng_;
};

#endif  // GOMOKU_AI_H
