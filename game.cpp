#include "game.h"

namespace {

Piece nextPlayer(Piece piece) {
    return (piece == Piece::Black) ? Piece::White : Piece::Black;
}

}  // namespace

Game::Game() = default;

void Game::reset() {
    board_.reset();
    currentPlayer_ = Piece::Black;
    state_ = GameState::InProgress;
}

bool Game::makeMove(int row, int col) {
    if (state_ != GameState::InProgress) {
        return false;
    }

    if (!board_.placePiece(row, col, currentPlayer_)) {
        return false;
    }

    if (board_.isWinningMove(row, col, currentPlayer_)) {
        state_ = (currentPlayer_ == Piece::Black) ? GameState::BlackWin : GameState::WhiteWin;
        return true;
    }

    if (board_.isFull()) {
        state_ = GameState::Draw;
        return true;
    }

    currentPlayer_ = nextPlayer(currentPlayer_);
    return true;
}

const Board& Game::board() const {
    return board_;
}

Piece Game::currentPlayer() const {
    return currentPlayer_;
}

GameState Game::state() const {
    return state_;
}

bool Game::isGameOver() const {
    return state_ != GameState::InProgress;
}
