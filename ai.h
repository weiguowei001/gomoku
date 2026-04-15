#ifndef GOMOKU_AI_H
#define GOMOKU_AI_H

#include <optional>
#include <random>
#include <utility>

#include "board.h"

class AiPlayer {
public:
    AiPlayer();

    // Strategy: win first, block second, random otherwise.
    std::optional<std::pair<int, int>> chooseMove(const Board& board,
                                                  Piece aiPiece,
                                                  Piece opponentPiece);

private:
    std::optional<std::pair<int, int>> findWinningMove(const Board& board, Piece piece) const;
    std::optional<std::pair<int, int>> chooseRandomMove(const Board& board);

private:
    std::mt19937 rng_;
};

#endif  // GOMOKU_AI_H
