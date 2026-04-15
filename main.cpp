#include <cmath>
#include <cstdint>
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QWidget>

#include "ai.h"
#include "game.h"

constexpr int kBoardSize = 15;
constexpr int kCellSize = 40;
constexpr int kMargin = 40;
constexpr int kPieceRadius = 15;

constexpr int kWindowSize = kMargin * 2 + kCellSize * (kBoardSize - 1);

namespace {

QString moveSoundFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
           QStringLiteral("/gomoku_move.wav");
}

QByteArray buildMoveSoundWavData() {
    constexpr int kSampleRate = 44100;
    constexpr int kDurationMs = 45;
    constexpr int kNumChannels = 1;
    constexpr int kBitsPerSample = 16;
    constexpr int kByteRate = kSampleRate * kNumChannels * (kBitsPerSample / 8);
    constexpr int kBlockAlign = kNumChannels * (kBitsPerSample / 8);
    constexpr int kNumSamples = kSampleRate * kDurationMs / 1000;
    const int dataSize = kNumSamples * kBlockAlign;

    QByteArray pcmData;
    pcmData.resize(dataSize);
    auto* sampleBytes = reinterpret_cast<std::int16_t*>(pcmData.data());

    constexpr double kPi = 3.14159265358979323846;
    for (int i = 0; i < kNumSamples; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kSampleRate);
        const double envelope = std::exp(-70.0 * t);
        const double tone = std::sin(2.0 * kPi * 2600.0 * t) + 0.45 * std::sin(2.0 * kPi * 4100.0 * t);
        const double value = tone * envelope;
        sampleBytes[i] = static_cast<std::int16_t>(value * 12000.0);
    }

    QByteArray wav;
    wav.reserve(44 + dataSize);
    QDataStream stream(&wav, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("RIFF", 4);
    stream << static_cast<quint32>(36 + dataSize);
    stream.writeRawData("WAVE", 4);
    stream.writeRawData("fmt ", 4);
    stream << static_cast<quint32>(16);  // PCM chunk size
    stream << static_cast<quint16>(1);   // Audio format PCM
    stream << static_cast<quint16>(kNumChannels);
    stream << static_cast<quint32>(kSampleRate);
    stream << static_cast<quint32>(kByteRate);
    stream << static_cast<quint16>(kBlockAlign);
    stream << static_cast<quint16>(kBitsPerSample);
    stream.writeRawData("data", 4);
    stream << static_cast<quint32>(dataSize);
    stream.writeRawData(pcmData.constData(), dataSize);

    return wav;
}

bool writeMoveSoundFileIfNeeded(const QString& filePath) {
    const QFileInfo info(filePath);
    if (!QDir().mkpath(info.absolutePath())) {
        return false;
    }

    if (info.exists() && info.size() > 0) {
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray wavData = buildMoveSoundWavData();
    const auto written = file.write(wavData);
    file.close();
    return written == wavData.size();
}

}  // namespace

class GomokuWidget : public QWidget {
public:
    GomokuWidget() {
        setFixedSize(kWindowSize, kWindowSize);
        setWindowTitle(QStringLiteral("五子棋（人机对战）"));
        selectDifficulty();
        initMoveSound();
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

    void initMoveSound() {
        const QString soundPath = moveSoundFilePath();
        if (!writeMoveSoundFileIfNeeded(soundPath)) {
            return;
        }
        moveSound_.setSource(QUrl::fromLocalFile(soundPath));
        moveSound_.setLoopCount(1);
        moveSound_.setVolume(0.7F);
    }

    void playMoveSound() {
        if (moveSound_.source().isEmpty()) {
            QApplication::beep();
            return;
        }
        moveSound_.stop();
        moveSound_.play();
    }

    void playResultSound(bool isWin) {
        QApplication::beep();
        // 用不同的节奏区分胜负结果，不引入额外音频资源。
        QTimer::singleShot(isWin ? 120 : 300, this, []() {
            QApplication::beep();
        });
    }

    void playDrawSound() {
        QApplication::beep();
        QTimer::singleShot(220, this, []() {
            QApplication::beep();
        });
    }

private:
    Game game_;
    AiPlayer ai_;
    QSoundEffect moveSound_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    GomokuWidget widget;
    widget.show();
    return app.exec();
}
