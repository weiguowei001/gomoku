#include <cmath>
#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWidget>

#include "game.h"

constexpr int kBoardSize = 15;
constexpr int kCellSize = 40;
constexpr int kMargin = 40;
constexpr int kPieceRadius = 15;

constexpr int kWindowSize = kMargin * 2 + kCellSize * (kBoardSize - 1);

class GomokuWidget : public QWidget {
public:
    GomokuWidget() {
        setFixedSize(kWindowSize, kWindowSize);
        setWindowTitle(QStringLiteral("五子棋"));
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(245, 200, 120));

        drawBoard(painter);
        drawPieces(painter);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (game_.isGameOver() || event->button() != Qt::LeftButton) {
            return;
        }

        const QPoint pos = event->pos();
        int row = 0;
        int col = 0;
        if (!positionToCell(pos, row, col)) {
            return;
        }

        if (!game_.makeMove(row, col)) {
            return;
        }
        update();

        if (game_.state() == GameState::BlackWin) {
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("黑棋获胜！"));
            return;
        }
        if (game_.state() == GameState::WhiteWin) {
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("白棋获胜！"));
            return;
        }
        if (game_.state() == GameState::Draw) {
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("平局！"));
        }        
    }

private:
    void drawBoard(QPainter& painter) {
        QPen linePen(Qt::black);
        linePen.setWidth(2);
        painter.setPen(linePen);

        for (int i = 0; i < kBoardSize; ++i) {
            const int x = kMargin + i * kCellSize;
            const int y = kMargin + i * kCellSize;
            painter.drawLine(kMargin, y, kWindowSize - kMargin, y);
            painter.drawLine(x, kMargin, x, kWindowSize - kMargin);
        }
    }

    void drawPieces(QPainter& painter) {
        const Board& board = game_.board();
        for (int row = 0; row < kBoardSize; ++row) {
            for (int col = 0; col < kBoardSize; ++col) {
                const Piece piece = board.pieceAt(row, col);
                if (piece == Piece::Empty) {
                    continue;
                }

                const QPoint center(
                    kMargin + col * kCellSize,
                    kMargin + row * kCellSize);

                if (piece == Piece::Black) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(Qt::black);
                } else {
                    painter.setPen(QPen(Qt::black, 1));
                    painter.setBrush(Qt::white);
                }
                painter.drawEllipse(center, kPieceRadius, kPieceRadius);
            }
        }
    }

    bool positionToCell(const QPoint& pos, int& outRow, int& outCol) const {
        const int col = (pos.x() - kMargin + kCellSize / 2) / kCellSize;
        const int row = (pos.y() - kMargin + kCellSize / 2) / kCellSize;
        if (!game_.board().isInside(row, col)) {
            return false;
        }

        const int centerX = kMargin + col * kCellSize;
        const int centerY = kMargin + row * kCellSize;
        if (std::abs(pos.x() - centerX) > kCellSize / 2 ||
            std::abs(pos.y() - centerY) > kCellSize / 2) {
            return false;
        }

        outRow = row;
        outCol = col;
        return true;
    }

private:
    Game game_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    GomokuWidget widget;
    widget.show();
    return app.exec();
}
