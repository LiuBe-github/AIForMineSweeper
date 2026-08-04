#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLCDNumber>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include "BoardWidget.h"
#include "CustomGameDialog.h"

namespace {
const int kAiServerPort = 8765;
const int kAiStepDelayMs = 300;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("扫雷"));

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(6, 6, 6, 6);
    rootLayout->setSpacing(6);

    // 顶部信息栏：雷数 | 笑脸按钮 | 计时
    auto* topBar = new QHBoxLayout;

    mineCounter_ = new QLCDNumber(6, central);
    mineCounter_->setSegmentStyle(QLCDNumber::Flat);
    mineCounter_->setFixedSize(60, 32);

    resetButton_ = new QPushButton(QStringLiteral("🙂"), central);
    resetButton_->setFixedSize(36, 32);
    resetButton_->setFocusPolicy(Qt::NoFocus);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::newGame);

    timeCounter_ = new QLCDNumber(6, central);
    timeCounter_->setSegmentStyle(QLCDNumber::Flat);
    timeCounter_->setFixedSize(60, 32);

    topBar->addWidget(mineCounter_);
    topBar->addStretch();
    topBar->addWidget(resetButton_);
    topBar->addStretch();
    topBar->addWidget(timeCounter_);
    rootLayout->addLayout(topBar);

    // 棋盘容器
    boardContainer_ = new QWidget(central);
    boardLayout_ = new QGridLayout(boardContainer_);
    boardLayout_->setContentsMargins(0, 0, 0, 0);
    boardLayout_->setSpacing(0);
    rootLayout->addWidget(boardContainer_);

    setCentralWidget(central);

    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::updateClock);

    network_ = new QNetworkAccessManager(this);

    // AI 解局按钮 + 模型选择下拉栏
    auto* aiRow = new QHBoxLayout;
    aiRow->addStretch();
    auto* aiHint = new QLabel(QStringLiteral("AI 解局："), central);
    aiEngineCombo_ = new QComboBox(central);
    aiEngineCombo_->setFocusPolicy(Qt::NoFocus);
    aiEngineCombo_->addItem(QStringLiteral("API Key 模型（DeepSeek）"),
                            QStringLiteral("api"));
    aiEngineCombo_->addItem(QStringLiteral("网页默认模型（内置求解器）"),
                            QStringLiteral("heuristic"));
    aiButton_ = new QPushButton(QStringLiteral("AI帮解"), central);
    aiButton_->setFocusPolicy(Qt::NoFocus);
    connect(aiButton_, &QPushButton::clicked, this, &MainWindow::onAiButtonClicked);
    aiRow->addWidget(aiHint);
    aiRow->addWidget(aiEngineCombo_);
    aiRow->addWidget(aiButton_);
    rootLayout->addLayout(aiRow);

    createMenus();
    buildBoard(9, 9, 10);  // 默认初级
    statusBar()->showMessage(
        QStringLiteral("左键翻开，右键插旗，中键或双击数字快速翻格"));

    // 命令行参数 --ai-solve：启动后自动开始 AI 解局（便于联调测试）
    if (QCoreApplication::arguments().contains(QStringLiteral("--ai-solve")))
        QTimer::singleShot(1500, this, &MainWindow::startAiSolving);
}

MainWindow::~MainWindow() {
    if (aiServerProcess_ && aiServerProcess_->state() != QProcess::NotRunning) {
        aiServerProcess_->terminate();
        aiServerProcess_->waitForFinished(1500);
    }
}

void MainWindow::createMenus() {
    QMenu* gameMenu = menuBar()->addMenu(QStringLiteral("游戏(&G)"));

    QAction* newAct = gameMenu->addAction(QStringLiteral("新游戏(&N)\tF2"));
    newAct->setShortcut(QKeySequence(Qt::Key_F2));
    connect(newAct, &QAction::triggered, this, &MainWindow::newGame);

    gameMenu->addSeparator();

    auto* group = new QActionGroup(this);
    group->setExclusive(true);

    auto addDifficulty = [&](const QString& text, Difficulty d, int rows,
                             int cols, int mines) {
        QAction* act = gameMenu->addAction(text);
        act->setCheckable(true);
        act->setData(static_cast<int>(d));
        group->addAction(act);
        connect(act, &QAction::triggered, this,
                [this, d, rows, cols, mines] {
                    difficulty_ = d;
                    buildBoard(rows, cols, mines);
                });
        return act;
    };

    QAction* beginner =
        addDifficulty(QStringLiteral("初级(&B)  9×9，10 雷"), Beginner, 9, 9, 10);
    addDifficulty(QStringLiteral("中级(&I)  16×16，40 雷"), Intermediate, 16, 16, 40);
    addDifficulty(QStringLiteral("高级(&E)  30×16，99 雷"), Expert, 30, 16, 99);
    beginner->setChecked(true);

    QAction* customAct = gameMenu->addAction(QStringLiteral("自定义(&C)..."));
    connect(customAct, &QAction::triggered, this, &MainWindow::startCustom);

    gameMenu->addSeparator();
    QAction* aiAct = gameMenu->addAction(QStringLiteral("AI 帮解(&A)"));
    connect(aiAct, &QAction::triggered, this, &MainWindow::onAiButtonClicked);

    gameMenu->addSeparator();
    QAction* quitAct = gameMenu->addAction(QStringLiteral("退出(&Q)"));
    quitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    QAction* howAct = helpMenu->addAction(QStringLiteral("玩法说明(&H)"));
    connect(howAct, &QAction::triggered, this, &MainWindow::showHelp);
    QAction* aboutAct = helpMenu->addAction(QStringLiteral("关于(&A)"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::buildBoard(int rows, int cols, int mines) {
    clearBoard();
    field_.reset(rows, cols, mines);
    elapsedSeconds_ = 0;
    started_ = false;
    timer_->stop();
    timeCounter_->display(0);
    setResetFace(QStringLiteral("🙂"));

    board_ = new BoardWidget(&field_, boardContainer_);
    connect(board_, &BoardWidget::leftClicked, this,
            &MainWindow::onCellLeftClicked);
    connect(board_, &BoardWidget::rightClicked, this,
            &MainWindow::onCellRightClicked);
    connect(board_, &BoardWidget::chordRequested, this,
            &MainWindow::onChordRequested);
    boardLayout_->addWidget(board_, 0, 0);

    updateCounters();
    statusBar()->clearMessage();
    adjustSize();
}

void MainWindow::clearBoard() {
    while (QLayoutItem* item = boardLayout_->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }
    board_ = nullptr;
}

void MainWindow::newGame() {
    switch (difficulty_) {
        case Beginner:
            buildBoard(9, 9, 10);
            break;
        case Intermediate:
            buildBoard(16, 16, 40);
            break;
        case Expert:
            buildBoard(30, 16, 99);
            break;
        case Custom:
            buildBoard(rows(), cols(), field_.mineCount());
            break;
    }
}

void MainWindow::startCustom() {
    CustomGameDialog dlg(this);
    dlg.setValues(rows(), cols(), field_.mineCount());
    if (dlg.exec() == QDialog::Accepted) {
        difficulty_ = Custom;
        buildBoard(dlg.rows(), dlg.cols(), dlg.mines());
    }
}

void MainWindow::onCellLeftClicked(int row, int col) {
    if (field_.isGameOver())
        return;

    field_.reveal(row, col);
    if (!started_ && !field_.isFirstClickPending()) {
        started_ = true;
        timer_->start();
    }

    syncAllCells();
    updateCounters();
    if (field_.isGameOver()) {
        timer_->stop();
        gameOverHandler(field_.isWon());
    }
}

void MainWindow::onCellRightClicked(int row, int col) {
    if (field_.isGameOver())
        return;
    field_.toggleFlag(row, col);
    syncAllCells();
    updateCounters();
}

void MainWindow::onChordRequested(int row, int col) {
    if (field_.isGameOver())
        return;
    field_.chord(row, col);
    syncAllCells();
    updateCounters();
    if (field_.isGameOver()) {
        timer_->stop();
        gameOverHandler(field_.isWon());
    }
}

void MainWindow::updateClock() {
    ++elapsedSeconds_;
    timeCounter_->display(qMin(elapsedSeconds_, 999));
}

void MainWindow::syncAllCells() {
    if (board_)
        board_->refresh(field_.takeDirty());
}

void MainWindow::updateCounters() {
    const int remaining = field_.mineCount() - field_.flaggedCount();
    mineCounter_->display(qBound(-999999, remaining, 999999));
}

void MainWindow::gameOverHandler(bool won) {
    if (won) {
        setResetFace(QStringLiteral("😎"));
        statusBar()->showMessage(QStringLiteral("恭喜，你赢了！"), 5000);
    } else {
        setResetFace(QStringLiteral("😵"));
        statusBar()->showMessage(QStringLiteral("踩到地雷了，再来一局吧！"), 5000);
    }
}

void MainWindow::setResetFace(const QString& face) {
    resetButton_->setText(face);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F2) {
        newGame();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::showHelp() {
    QMessageBox::information(
        this, QStringLiteral("玩法说明"),
        QStringLiteral(
            "扫雷玩法：\n\n"
            "1. 左键点击格子将其翻开，首次点击一定安全。\n"
            "2. 格子上的数字表示周围 8 格中的地雷数量。\n"
            "3. 右键在格子上插旗/取消旗，标记可能有雷的位置。\n"
            "4. 数字周围旗子数等于数字时，中键或左键点击该数字可快速翻开周围格子。\n"
            "5. 翻开所有非地雷格子即获胜，踩到地雷则失败。\n\n"
            "难度：初级 9×9/10 雷，中级 16×16/40 雷，高级 30×16/99 雷。"));
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this, QStringLiteral("关于"),
        QStringLiteral(
            "<b>扫雷 MineSweeper</b><br><br>"
            "使用 C++ 与 Qt 5 (Widgets) 实现。<br>"
            "支持初级/中级/高级/自定义难度、计时、雷数统计。"));
}

// ---------------------------------------------------------------------------
// AI 解局
// ---------------------------------------------------------------------------

void MainWindow::onAiButtonClicked() {
    if (aiSolving_) {
        stopAiSolving(QStringLiteral("已停止 AI 解局"));
        return;
    }
    startAiSolving();
}

void MainWindow::startAiSolving() {
    if (aiSolving_)
        return;
    // 若上一局已经结束，先开一局新游戏再开始求解
    if (field_.isGameOver())
        buildBoard(rows(), cols(), field_.mineCount());
    aiSolving_ = true;
    aiSteps_ = 0;
    aiAttempts_ = 1;
    aiRetries_ = 0;
    aiButton_->setText(QStringLiteral("停止"));
    statusBar()->showMessage(
        QStringLiteral("正在连接 AI 服务 (127.0.0.1:%1)...").arg(kAiServerPort));
    checkServerAndStart();
}

void MainWindow::checkServerAndStart() {
    QNetworkRequest req(
        QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(kAiServerPort)));
    req.setTransferTimeout(1500);
    QNetworkReply* reply = network_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
        if (!aiSolving_)
            return;

        if (ok) {
            statusBar()->showMessage(QStringLiteral("AI 服务已连接，开始解局..."));
            stepAiSolve();
            return;
        }

        // 服务未启动：尝试自动启动 python server.py
        if (!aiServerProcess_ ||
            aiServerProcess_->state() == QProcess::NotRunning) {
            const QString script = QDir::cleanPath(
                QDir(QCoreApplication::applicationDirPath())
                    .filePath(QStringLiteral("../../AIForMineSweeper/server.py")));
            if (QFileInfo::exists(script)) {
                aiServerProcess_ = new QProcess(this);
                aiServerProcess_->start(
                    QStringLiteral("python"),
                    QStringList() << QDir::toNativeSeparators(script));
                statusBar()->showMessage(QStringLiteral("正在启动 AI 服务..."));
                QTimer::singleShot(2500, this, [this] {
                    if (aiSolving_)
                        checkServerAndStart();
                });
                return;
            }
        }

        stopAiSolving(QStringLiteral(
            "无法连接 AI 服务，请先运行: python AIForMineSweeper\\server.py"));
    });
}

void MainWindow::stepAiSolve() {
    if (!aiSolving_ || field_.isGameOver())
        return;
    sendBoardToAi(QString());
}

void MainWindow::sendBoardToAi(const QString& feedback) {
    if (!aiSolving_)
        return;

    QJsonObject payload = boardToJson();
    payload.insert(QStringLiteral("engine"),
                   aiEngineCombo_->currentData().toString());
    if (!feedback.isEmpty()) {
        payload.insert(QStringLiteral("feedback"), feedback);
        payload.insert(QStringLiteral("invalid_attempts"), aiRetries_);
    }

    QNetworkRequest req(
        QUrl(QStringLiteral("http://127.0.0.1:%1/solve").arg(kAiServerPort)));
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    req.setTransferTimeout(180000);

    QElapsedTimer roundTrip;
    roundTrip.start();
    QNetworkReply* reply = network_->post(
        req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, roundTrip] {
        reply->deleteLater();
        if (!aiSolving_)
            return;
        const qint64 latencyMs = roundTrip.elapsed();
        if (reply->error() != QNetworkReply::NoError) {
            // 优先展示服务端返回的具体错误（4xx/5xx 的 body 里有 {"error": ...}）
            const QJsonObject errObj =
                QJsonDocument::fromJson(reply->readAll()).object();
            const QString serverErr =
                errObj.value(QStringLiteral("error")).toString();
            if (!serverErr.isEmpty()) {
                stopAiSolving(QStringLiteral("AI 服务返回错误：%1")
                                  .arg(serverErr));
            } else {
                stopAiSolving(QStringLiteral("AI 服务请求失败：%1")
                                  .arg(reply->errorString()));
            }
            return;
        }

        const QJsonObject obj =
            QJsonDocument::fromJson(reply->readAll()).object();
        // 累计本次请求的网络往返延迟，并记录实际使用的引擎/模型
        aiNetworkLatencyMs_ += latencyMs;
        const QString respEngine =
            obj.value(QStringLiteral("engine")).toString();
        const QString respModel =
            obj.value(QStringLiteral("model")).toString();
        if (!respEngine.isEmpty())
            aiEngineUsed_ = respEngine;
        if (!respModel.isEmpty())
            aiModelUsed_ = respModel;
        if (obj.contains(QStringLiteral("error"))) {
            stopAiSolving(QStringLiteral("AI 服务返回错误：%1")
                              .arg(obj.value(QStringLiteral("error")).toString()));
            return;
        }

        const QString action = obj.value(QStringLiteral("action")).toString();
        const int row = obj.value(QStringLiteral("row")).toInt(-1);
        const int col = obj.value(QStringLiteral("col")).toInt(-1);

        QString feedback;
        if (!executeAiMove(action, row, col, feedback)) {
            ++aiRetries_;
            if (aiRetries_ > 3) {
                stopAiSolving(QStringLiteral("AI 连续多次给出无效指令，已停止：%1")
                                  .arg(feedback));
                return;
            }
            QTimer::singleShot(300, this,
                               [this, feedback] { sendBoardToAi(feedback); });
            return;
        }

        aiRetries_ = 0;
        ++aiSteps_;

        // 与手动点击一致：第一次真正翻开后启动计时
        if (!started_ && !field_.isFirstClickPending()) {
            started_ = true;
            timer_->start();
        }

        syncAllCells();
        updateCounters();

        if (field_.isGameOver()) {
            if (field_.isWon()) {
                finishAiRun(true);
            } else {
                // 踩雷后自动重开一局，继续求解直到通关
                ++aiAttempts_;
                buildBoard(rows(), cols(), field_.mineCount());
                statusBar()->showMessage(
                    QStringLiteral("AI 踩雷了，第 %1 次自动重开...")
                        .arg(aiAttempts_));
                QTimer::singleShot(1200, this, [this] {
                    if (aiSolving_)
                        stepAiSolve();
                });
            }
            return;
        }

        // 稍作停顿，让玩家看到 AI 逐步解局的过程
        QTimer::singleShot(kAiStepDelayMs, this, [this] {
            if (aiSolving_)
                stepAiSolve();
        });
    });
}

bool MainWindow::executeAiMove(const QString& action, int row, int col,
                               QString& feedback) {
    if (!field_.inBounds(row, col)) {
        feedback = QStringLiteral("坐标 (%1,%2) 超出棋盘。").arg(row).arg(col);
        return false;
    }

    const Cell& cell = field_.cell(row, col);
    if (action == QLatin1String("reveal")) {
        if (cell.state != CellState::Hidden) {
            feedback = QStringLiteral("格子 (%1,%2) 不是隐藏格，不能翻开。")
                           .arg(row)
                           .arg(col);
            return false;
        }
        field_.reveal(row, col);
        return true;
    }
    if (action == QLatin1String("flag")) {
        if (cell.state == CellState::Revealed) {
            feedback = QStringLiteral("格子 (%1,%2) 已翻开，不能插旗。")
                           .arg(row)
                           .arg(col);
            return false;
        }
        field_.toggleFlag(row, col);
        return true;
    }
    if (action == QLatin1String("chord")) {
        if (cell.state != CellState::Revealed || cell.adjacent == 0) {
            feedback = QStringLiteral("格子 (%1,%2) 不是带数字的已翻开格。")
                           .arg(row)
                           .arg(col);
            return false;
        }
        field_.chord(row, col);
        return true;
    }
    feedback = QStringLiteral("未知动作：%1").arg(action);
    return false;
}

QJsonObject MainWindow::boardToJson() const {
    QJsonObject obj;
    obj.insert(QStringLiteral("rows"), field_.rows());
    obj.insert(QStringLiteral("cols"), field_.cols());
    obj.insert(QStringLiteral("mines"), field_.mineCount());
    obj.insert(QStringLiteral("flagged"), field_.flaggedCount());
    obj.insert(QStringLiteral("revealed"), field_.revealedCount());
    obj.insert(QStringLiteral("first_click_done"),
               !field_.isFirstClickPending());

    QJsonArray cells;
    for (int r = 0; r < field_.rows(); ++r) {
        for (int c = 0; c < field_.cols(); ++c) {
            const Cell& cell = field_.cell(r, c);
            QJsonObject co;
            if (cell.state == CellState::Revealed) {
                co.insert(QStringLiteral("state"), QStringLiteral("revealed"));
                co.insert(QStringLiteral("adjacent"), cell.adjacent);
            } else if (cell.state == CellState::Flagged) {
                co.insert(QStringLiteral("state"), QStringLiteral("flagged"));
            } else {
                co.insert(QStringLiteral("state"), QStringLiteral("hidden"));
            }
            cells.append(co);
        }
    }
    obj.insert(QStringLiteral("cells"), cells);
    return obj;
}

void MainWindow::finishAiRun(bool won) {
    aiSolving_ = false;
    aiButton_->setText(QStringLiteral("AI帮解"));
    timer_->stop();
    gameOverHandler(won);

    if (won) {
        recordAiResult();
        statusBar()->showMessage(
            QStringLiteral("AI 通关成功！用时 %1 秒，共 %2 步，尝试 %3 次，结果已记录。")
                .arg(elapsedSeconds_)
                .arg(aiSteps_)
                .arg(aiAttempts_),
            10000);
    }
}

void MainWindow::recordAiResult() {
    const QString dir = QDir::cleanPath(
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("../../AIForMineSweeper")));
    const QString path = dir + QStringLiteral("/solve_records.txt");

    QFile file(path);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        statusBar()->showMessage(
            QStringLiteral("无法写入记录文件：%1").arg(path), 8000);
        return;
    }

    const QString ts =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    QString engineLabel;
    if (aiEngineUsed_ == QLatin1String("heuristic")) {
        engineLabel = QStringLiteral("网页默认模型（内置求解器）");
    } else if (aiEngineUsed_ == QLatin1String("api")) {
        engineLabel = QStringLiteral("API Key 模型（DeepSeek）");
    } else {
        // 兜底：以下拉框当前选择为准
        engineLabel = (aiEngineCombo_->currentData().toString()
                       == QLatin1String("api"))
                          ? QStringLiteral("API Key 模型（DeepSeek）")
                          : QStringLiteral("网页默认模型（内置求解器）");
    }
    const QString modelLabel = aiModelUsed_.isEmpty()
                                   ? QStringLiteral("—")
                                   : aiModelUsed_;
    const int adjustedSec =
        qMax(0, elapsedSeconds_ - static_cast<int>(aiNetworkLatencyMs_ / 1000));
    const QString record =
        QStringLiteral("========================================\n"
                       "时间: %1\n"
                       "难度: %2\n"
                       "棋盘: %3x%4，%5 雷\n"
                       "引擎: %6\n"
                       "模型: %7\n"
                       "总用时(含网络等待): %8 秒\n"
                       "网络延迟合计: %9 毫秒\n"
                       "通关用时(去网络延迟): %10 秒\n"
                       "AI 步数: %11\n"
                       "尝试次数: %12\n"
                       "结果: 通关\n"
                       "========================================\n")
            .arg(ts)
            .arg(difficultyName())
            .arg(rows())
            .arg(cols())
            .arg(field_.mineCount())
            .arg(engineLabel)
            .arg(modelLabel)
            .arg(elapsedSeconds_)
            .arg(aiNetworkLatencyMs_)
            .arg(adjustedSec)
            .arg(aiSteps_)
            .arg(aiAttempts_);
    file.write(record.toUtf8());
    file.close();
}

void MainWindow::stopAiSolving(const QString& message) {
    aiSolving_ = false;
    aiButton_->setText(QStringLiteral("AI帮解"));
    if (!message.isEmpty())
        statusBar()->showMessage(message, 8000);
}

QString MainWindow::difficultyName() const {
    switch (difficulty_) {
        case Beginner:
            return QStringLiteral("初级");
        case Intermediate:
            return QStringLiteral("中级");
        case Expert:
            return QStringLiteral("高级");
        case Custom:
            return QStringLiteral("自定义");
    }
    return QStringLiteral("未知");
}
