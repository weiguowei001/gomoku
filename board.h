#ifndef GOMOKU_BOARD_H
#define GOMOKU_BOARD_H

#include <vector>

enum class Piece {
    Empty = 0,
    Black = 1,
    White = 2,
};

class Board {
public:
    static constexpr int kSize = 15;

    Board();

    void reset();
    bool isInside(int row, int col) const;
    bool isEmpty(int row, int col) const;
    bool placePiece(int row, int col, Piece piece);
    Piece pieceAt(int row, int col) const;
    bool isFull() const;
    bool isWinningMove(int row, int col, Piece piece) const;

private:
    int countInDirection(int row, int col, int dRow, int dCol, Piece piece) const;

private:
    std::vector<std::vector<Piece>> cells_;
};

#endif  // GOMOKU_BOARD_H
