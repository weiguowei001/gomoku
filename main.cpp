#include <cmath>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QWidget>

#include "ai.h"
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
        setWindowTitle(QStringLiteral("五子棋（人机对战）"));
        selectDifficulty();
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
        if (game_.currentPlayer() != Piece::Black) {
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
        playMoveSound();
        update();
        if (showGameResultIfFinished()) {
            return;
        }

        tryAiMove();
    }

private:
    void selectDifficulty() {
        const QStringList items = {
            QStringLiteral("简单"),
            QStringLiteral("普通"),
            QStringLiteral("困难"),
        };
        bool ok = false;
        const QString selected = QInputDialog::getItem(
            this,
            QStringLiteral("选择难度"),
            QStringLiteral("请选择 AI 难度："),
            items,
            1,
            false,
            &ok);

        if (!ok || selected == items[1]) {
            ai_.setDifficulty(AiDifficulty::Normal);
            setWindowTitle(QStringLiteral("五子棋（人机对战 - 普通）"));
            return;
        }
        if (selected == items[0]) {
            ai_.setDifficulty(AiDifficulty::Easy);
            setWindowTitle(QStringLiteral("五子棋（人机对战 - 简单）"));
            return;
        }
        ai_.setDifficulty(AiDifficulty::Hard);
        setWindowTitle(QStringLiteral("五子棋（人机对战 - 困难）"));
    }

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

    bool showGameResultIfFinished() {
        if (game_.state() == GameState::BlackWin) {
            playResultSound(/*isWin=*/true);
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("你（黑棋）获胜！"));
            return true;
        }
        if (game_.state() == GameState::WhiteWin) {
            playResultSound(/*isWin=*/false);
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("AI（白棋）获胜！"));
            return true;
        }
        if (game_.state() == GameState::Draw) {
            playDrawSound();
            QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("平局！"));
            return true;
        }
        return false;
    }

    void tryAiMove() {
        if (game_.isGameOver() || game_.currentPlayer() != Piece::White) {
            return;
        }

        const auto move = ai_.chooseMove(game_.board(), Piece::White, Piece::Black);
        if (!move.has_value()) {
            return;
        }

        const auto [row, col] = *move;
        if (!game_.makeMove(row, col)) {
            return;
        }

        playMoveSound();
        update();
        showGameResultIfFinished();
    }

    void playMoveSound() const {
        QApplication::beep();
    }

    void playResultSound(bool isWin) const {
        QApplication::beep();
        // 用不同的节奏区分胜负结果，不引入额外音频资源。
        QTimer::singleShot(isWin ? 120 : 300, this, []() {
            QApplication::beep();
        });
    }

    void playDrawSound() const {
        QApplication::beep();
        QTimer::singleShot(220, this, []() {
            QApplication::beep();
        });
    }

private:
    Game game_;
    AiPlayer ai_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    GomokuWidget widget;
    widget.show();
    return app.exec();
}
