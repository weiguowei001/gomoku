#ifndef GOMOKU_GAME_H
#define GOMOKU_GAME_H

#include "board.h"

enum class GameState {
    InProgress = 0,
    BlackWin,
    WhiteWin,
    Draw,
};

class Game {
public:
    Game();

    void reset();
    bool makeMove(int row, int col);

    const Board& board() const;
    Piece currentPlayer() const;
    GameState state() const;
    bool isGameOver() const;

private:
    Board board_;
    Piece currentPlayer_ = Piece::Black;
    GameState state_ = GameState::InProgress;
};

#endif  // GOMOKU_GAME_H
