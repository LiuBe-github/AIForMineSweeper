#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "MineField.h"

class QGridLayout;
class QComboBox;
class QLCDNumber;
class QNetworkAccessManager;
class QProcess;
class QPushButton;
class QTimer;
class QWidget;
class CellButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void newGame();
    void startCustom();
    void onCellLeftClicked(int row, int col);
    void onCellRightClicked(int row, int col);
    void onChordRequested(int row, int col);
    void updateClock();
    void showHelp();
    void showAbout();
    void onAiButtonClicked();
    void startAiSolving();

private:
    enum Difficulty { Beginner, Intermediate, Expert, Custom };

    void createMenus();
    void buildBoard(int rows, int cols, int mines);
    void clearBoard();
    void syncAllCells();
    void updateCounters();
    void gameOverHandler(bool won);
    void setResetFace(const QString& face);
    void checkServerAndStart();
    void stepAiSolve();
    void sendBoardToAi(const QString& feedback = QString());
    bool executeAiMove(const QString& action, int row, int col, QString& feedback);
    QJsonObject boardToJson() const;
    void finishAiRun(bool won);
    void recordAiResult();
    void stopAiSolving(const QString& message = QString());
    QString difficultyName() const;
    int rows() const { return field_.rows(); }
    int cols() const { return field_.cols(); }

    Difficulty difficulty_ = Beginner;
    MineField field_;
    QVector<CellButton*> cells_;

    QGridLayout* boardLayout_ = nullptr;
    QWidget* boardContainer_ = nullptr;
    QLCDNumber* mineCounter_ = nullptr;
    QLCDNumber* timeCounter_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QPushButton* aiButton_ = nullptr;
    QComboBox* aiEngineCombo_ = nullptr;
    QTimer* timer_ = nullptr;
    QNetworkAccessManager* network_ = nullptr;
    QProcess* aiServerProcess_ = nullptr;

    int elapsedSeconds_ = 0;
    bool started_ = false;
    bool aiSolving_ = false;
    int aiSteps_ = 0;
    int aiAttempts_ = 1;
    int aiRetries_ = 0;
    qint64 aiNetworkLatencyMs_ = 0;
    QString aiEngineUsed_;
    QString aiModelUsed_;
};

#endif  // MAINWINDOW_H
