#include "board.h"

Board::Board() : cells_(kSize, std::vector<Piece>(kSize, Piece::Empty)) {}

void Board::reset() {
    for (auto& row : cells_) {
        for (auto& cell : row) {
            cell = Piece::Empty;
        }
    }
}

bool Board::isInside(int row, int col) const {
    return row >= 0 && row < kSize && col >= 0 && col < kSize;
}

bool Board::isEmpty(int row, int col) const {
    return isInside(row, col) && cells_[row][col] == Piece::Empty;
}

bool Board::placePiece(int row, int col, Piece piece) {
    if (!isInside(row, col) || piece == Piece::Empty || !isEmpty(row, col)) {
        return false;
    }
    cells_[row][col] = piece;
    return true;
}

Piece Board::pieceAt(int row, int col) const {
    if (!isInside(row, col)) {
        return Piece::Empty;
    }
    return cells_[row][col];
}

bool Board::isFull() const {
    for (const auto& row : cells_) {
        for (Piece piece : row) {
            if (piece == Piece::Empty) {
                return false;
            }
        }
    }
    return true;
}

int Board::countInDirection(int row, int col, int dRow, int dCol, Piece piece) const {
    int count = 0;
    int currentRow = row + dRow;
    int currentCol = col + dCol;
    while (isInside(currentRow, currentCol) &&
           pieceAt(currentRow, currentCol) == piece) {
        ++count;
        currentRow += dRow;
        currentCol += dCol;
    }
    return count;
}

bool Board::isWinningMove(int row, int col, Piece piece) const {
    if (!isInside(row, col) || piece == Piece::Empty) {
        return false;
    }

    static const int directions[4][2] = {
        {1, 0}, {0, 1}, {1, 1}, {1, -1}
    };

    for (const auto& direction : directions) {
        const int dRow = direction[0];
        const int dCol = direction[1];
        const int count =
            1 + countInDirection(row, col, dRow, dCol, piece) +
            countInDirection(row, col, -dRow, -dCol, piece);
        if (count >= 5) {
            return true;
        }
    }

    return false;
}
