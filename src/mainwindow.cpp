#include "mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QMetaType>

#include "imagequalityanalyzer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_inputLayout(nullptr)
    , m_rtspInput(nullptr)
    , m_addButton(nullptr)
    , m_removeButton(nullptr)
    , m_cameraTabs(nullptr)
    , m_statusBar(nullptr)
    , m_statusLabel(nullptr)
    , m_cameraCountLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_activityTimer(nullptr)
    , m_nextCameraId(1)
{
    // Регистрируем метатип для передачи между потоками
    qRegisterMetaType<ImageQualityAnalyzer::QualityResult>("ImageQualityAnalyzer::QualityResult");
    
    setWindowTitle("Анализатор качества видеопотока с IP камер");
    resize(1200, 800);
    setupUi();

    m_activityTimer = new QTimer(this);
    connect(m_activityTimer, &QTimer::timeout, this, &MainWindow::updateActivityTimer);
    m_activityTimer->start(1000);
    m_statusLabel->setText("Готов. Введите RTSP URL для добавления камеры.");
}

MainWindow::~MainWindow()
{
    stopAllCameras();
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // Input panel
    m_inputLayout = new QHBoxLayout();
    QLabel* inputLabel = new QLabel("RTSP URL:", this);
    inputLabel->setFixedWidth(80);
    m_rtspInput = new QLineEdit(this);
    m_rtspInput->setPlaceholderText("rtsp://пользователь:пароль@192.168.1.1:554/поток");
    m_rtspInput->setMinimumWidth(400);
    m_addButton = new QPushButton("Добавить камеру", this);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addCamera);
    m_removeButton = new QPushButton("Удалить камеру", this);
    connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::removeCamera);
    m_removeButton->setEnabled(false);
    m_inputLayout->addWidget(inputLabel);
    m_inputLayout->addWidget(m_rtspInput);
    m_inputLayout->addWidget(m_addButton);
    m_inputLayout->addWidget(m_removeButton);
    m_inputLayout->addStretch();
    m_mainLayout->addLayout(m_inputLayout);

    // Camera tabs
    m_cameraTabs = new QTabWidget(this);
    m_cameraTabs->setTabsClosable(true);
    m_cameraTabs->setMovable(true);
    connect(m_cameraTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::removeCamera);
    
    // Instructions tab
    QLabel* emptyLabel = new QLabel(
        "<html><center>"
        "<h2>Камеры не добавлены</h2>"
        "<p>Введите RTSP URL выше и нажмите 'Добавить камеру'.</p>"
        "<p>Пример: rtsp://192.168.1.1:554/stream</p>"
        "</center></html>", this
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setMinimumSize(400, 300);
    emptyLabel->setObjectName("Instructions");
    m_cameraTabs->addTab(emptyLabel, "Инструкция");
    
    m_mainLayout->addWidget(m_cameraTabs);

    // Toolbar - only "About" action
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    addToolBar(toolbar);
    
    QAction* aboutAction = new QAction("О программе", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    toolbar->addAction(aboutAction);

    // Status bar
    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
    m_statusLabel = new QLabel("Готов", this);
    m_statusLabel->setMinimumWidth(300);
    m_statusBar->addWidget(m_statusLabel, 1);
    m_cameraCountLabel = new QLabel("Камер: 0", this);
    m_cameraCountLabel->setMinimumWidth(100);
    m_statusBar->addPermanentWidget(m_cameraCountLabel);
    m_timeLabel = new QLabel("Текущее время: --:--:--", this);
    m_timeLabel->setMinimumWidth(150);
    m_statusBar->addPermanentWidget(m_timeLabel);
}

void MainWindow::addCamera()
{
    QString rtspUrl = m_rtspInput->text().trimmed();
    
    if (rtspUrl.isEmpty()) {
        m_statusLabel->setText("Ошибка: Введите RTSP URL");
        m_rtspInput->setFocus();
        return;
    }
    
    if (!rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive)) {
        m_statusLabel->setText("Ошибка: URL должен начинаться с rtsp://");
        m_rtspInput->setFocus();
        return;
    }
    
    if (m_cameraUrls.values().contains(rtspUrl)) {
        m_statusLabel->setText("Ошибка: Эта камера уже добавлена");
        return;
    }
    
    int cameraId = generateCameraId();
    
    // Remove instructions tab if it exists
    if (m_cameraTabs->count() == 1) {
        QWidget* firstTab = m_cameraTabs->widget(0);
        if (firstTab->objectName() == "Instructions") {
            m_cameraTabs->removeTab(0);
        }
    }
    
    // Create thread and worker
    QThread* workerThread = new QThread(this);
    m_cameraThreads[cameraId] = workerThread;
    
    CameraWorker* worker = new CameraWorker(rtspUrl);
    m_cameraWorkers[cameraId] = worker;
    m_cameraUrls[cameraId] = rtspUrl;
    
    worker->moveToThread(workerThread);
    
    connect(workerThread, &QThread::started, worker, &CameraWorker::startCapture);
    connect(workerThread, &QThread::finished, worker, &CameraWorker::deleteLater);
    connect(worker, &CameraWorker::frameReady, this, [this, cameraId](const QImage& image) {
        updateFrame(cameraId, image);
    }, Qt::QueuedConnection);
    connect(worker, &CameraWorker::qualityResultReady, this, [this, cameraId](const ImageQualityAnalyzer::QualityResult& result) {
        updateQualityResult(cameraId, result);
    }, Qt::QueuedConnection);
    connect(worker, &CameraWorker::connectionStatusChanged, this, [this, cameraId](bool connected, const QString& message) {
        handleConnectionStatus(cameraId, connected, message);
    }, Qt::QueuedConnection);
    connect(worker, &CameraWorker::errorOccurred, this, [this, cameraId](const QString& errorText) {
        handleError(cameraId, errorText);
    }, Qt::QueuedConnection);
    connect(worker, &CameraWorker::connectionLost, this, [this, cameraId]() {
        handleConnectionLost(cameraId);
    }, Qt::QueuedConnection);
    
    addCameraTab(cameraId, rtspUrl);
    
    workerThread->start();
    m_rtspInput->clear();
    m_statusLabel->setText("Добавление камеры: " + rtspUrl);
    m_removeButton->setEnabled(true);
    
    qInfo() << "Added camera with ID:" << cameraId << "URL:" << rtspUrl;
}

void MainWindow::removeCamera()
{
    int currentIndex = m_cameraTabs->currentIndex();
    if (currentIndex < 0) {
        m_statusLabel->setText("Камера не выбрана");
        return;
    }
    
    QList<int> cameraIds = m_cameraWorkers.keys();
    if (currentIndex >= cameraIds.size()) {
        m_statusLabel->setText("Неверный выбор камеры");
        return;
    }
    
    int cameraId = cameraIds[currentIndex];
    QString url = m_cameraUrls[cameraId];
    
    // Stop thread
    if (m_cameraThreads.contains(cameraId)) {
        QThread* thread = m_cameraThreads[cameraId];
        thread->quit();
        if (!thread->wait(5000)) {
            thread->terminate();
            thread->wait();
        }
        m_cameraThreads.remove(cameraId);
    }
    
    // Remove tab
    m_cameraTabs->removeTab(currentIndex);
    
    // Clean up maps
    m_cameraWorkers.remove(cameraId);
    m_cameraUrls.remove(cameraId);
    m_frameLabels.remove(cameraId);
    m_scoreLabels.remove(cameraId);
    
    if (m_cameraWorkers.isEmpty()) {
        m_removeButton->setEnabled(false);
        m_statusLabel->setText("Все камеры удалены");
    } else {
        m_statusLabel->setText("Камера удалена: " + url);
    }
    
    m_cameraCountLabel->setText(QString("Камер: %1").arg(m_cameraWorkers.size()));
    
    qInfo() << "Removed camera ID:" << cameraId << "URL:" << url;
}

void MainWindow::addCameraTab(int cameraId, const QString& rtspUrl)
{
    QVBoxLayout* tabLayout = new QVBoxLayout();
    tabLayout->setSpacing(5);
    
    // Video label
    QLabel* frameLabel = new QLabel(this);
    frameLabel->setAlignment(Qt::AlignCenter);
    frameLabel->setMinimumSize(640, 480);
    frameLabel->setStyleSheet("background-color: #1a1a1a; border: 1px solid #333;");
    frameLabel->setText("Подключение...");
    frameLabel->setObjectName(QString("frameLabel_%1").arg(cameraId));
    m_frameLabels[cameraId] = frameLabel;
    tabLayout->addWidget(frameLabel, 1);
    
    // Metrics group
    QGroupBox* metricsGroup = new QGroupBox("Метрики качества", this);
    QGridLayout* metricsLayout = new QGridLayout();
    
    int row = 0;
    QLabel* noiseLabel = new QLabel("Шум:", this);
    QProgressBar* noiseBar = new QProgressBar(this);
    noiseBar->setRange(0, 100);
    noiseBar->setValue(0);
    noiseBar->setObjectName(QString("noiseBar_%1").arg(cameraId));
    
    QLabel* contrastLabel = new QLabel("Контраст:", this);
    QProgressBar* contrastBar = new QProgressBar(this);
    contrastBar->setRange(0, 100);
    contrastBar->setValue(0);
    contrastBar->setObjectName(QString("contrastBar_%1").arg(cameraId));
    
    QLabel* sharpnessLabel = new QLabel("Резкость:", this);
    QProgressBar* sharpnessBar = new QProgressBar(this);
    sharpnessBar->setRange(0, 100);
    sharpnessBar->setValue(0);
    sharpnessBar->setObjectName(QString("sharpnessBar_%1").arg(cameraId));
    
    QLabel* overexposedLabel = new QLabel("Пересвет:", this);
    QProgressBar* overexposedBar = new QProgressBar(this);
    overexposedBar->setRange(0, 100);
    overexposedBar->setValue(0);
    overexposedBar->setObjectName(QString("overexposedBar_%1").arg(cameraId));
    
    metricsLayout->addWidget(noiseLabel, row, 0);
    metricsLayout->addWidget(noiseBar, row++, 1);
    metricsLayout->addWidget(contrastLabel, row, 0);
    metricsLayout->addWidget(contrastBar, row++, 1);
    metricsLayout->addWidget(sharpnessLabel, row, 0);
    metricsLayout->addWidget(sharpnessBar, row++, 1);
    metricsLayout->addWidget(overexposedLabel, row, 0);
    metricsLayout->addWidget(overexposedBar, row++, 1);
    
    metricsGroup->setLayout(metricsLayout);
    tabLayout->addWidget(metricsGroup);
    
    // Score label
    QLabel* scoreLabel = new QLabel("Оценка: --", this);
    scoreLabel->setAlignment(Qt::AlignCenter);
    QFont scoreFont = scoreLabel->font();
    scoreFont.setPointSize(24);
    scoreFont.setBold(true);
    scoreLabel->setFont(scoreFont);
    scoreLabel->setObjectName(QString("scoreLabel_%1").arg(cameraId));
    m_scoreLabels[cameraId] = scoreLabel;
    tabLayout->addWidget(scoreLabel);
    
    // Status label
    QLabel* statusLabel = new QLabel("Статус: Подключение...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setObjectName(QString("statusLabel_%1").arg(cameraId));
    tabLayout->addWidget(statusLabel);
    
    // Tab widget
    QWidget* tabWidget = new QWidget(this);
    tabWidget->setLayout(tabLayout);
    
    QString cameraName = rtspUrl;
    if (cameraName.length() > 30) {
        cameraName = cameraName.left(27) + "...";
    }
    
    m_cameraTabs->addTab(tabWidget, cameraName);
    m_cameraTabs->setCurrentIndex(m_cameraTabs->count() - 1);
    m_cameraCountLabel->setText(QString("Камер: %1").arg(m_cameraWorkers.size()));
    
    tabWidget->setProperty("cameraId", cameraId);
}

void MainWindow::updateFrame(int cameraId, const QImage& image)
{
    if (!m_frameLabels.contains(cameraId)) return;
    
    QLabel* frameLabel = m_frameLabels[cameraId];
    if (!frameLabel) return;
    
    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap = pixmap.scaled(
        frameLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    frameLabel->setPixmap(pixmap);
}

void MainWindow::updateQualityResult(int cameraId, const ImageQualityAnalyzer::QualityResult& result)
{
    QList<int> cameraIds = m_cameraWorkers.keys();
    int tabIndex = cameraIds.indexOf(cameraId);
    if (tabIndex < 0) return;
    
    QWidget* tabWidget = m_cameraTabs->widget(tabIndex);
    if (!tabWidget) return;
    
    QProgressBar* noiseBar = tabWidget->findChild<QProgressBar*>(QString("noiseBar_%1").arg(cameraId));
    QProgressBar* contrastBar = tabWidget->findChild<QProgressBar*>(QString("contrastBar_%1").arg(cameraId));
    QProgressBar* sharpnessBar = tabWidget->findChild<QProgressBar*>(QString("sharpnessBar_%1").arg(cameraId));
    QProgressBar* overexposedBar = tabWidget->findChild<QProgressBar*>(QString("overexposedBar_%1").arg(cameraId));
    QLabel* scoreLabel = tabWidget->findChild<QLabel*>(QString("scoreLabel_%1").arg(cameraId));
    
    // Шум: инвертируем отображение (больше шума = больший процент = плохо)
    int noiseDisplay = static_cast<int>(100 - result.noiseScore);
    if (noiseBar) {
        noiseBar->setValue(noiseDisplay);
        // Цвет: зеленый = мало шума (хорошо), красный = много шума (плохо)
        noiseBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }")
            .arg(getQualityColor(100 - noiseDisplay).name()));
    }
    
    // Контраст: выше = лучше, зеленый при хорошем качестве
    if (contrastBar) {
        contrastBar->setValue(static_cast<int>(result.contrastScore));
        contrastBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }")
            .arg(getQualityColor(result.contrastScore).name()));
    }
    
    // Резкость: выше = лучше, зеленый при хорошем качестве
    if (sharpnessBar) {
        sharpnessBar->setValue(static_cast<int>(result.sharpnessScore));
        sharpnessBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }")
            .arg(getQualityColor(result.sharpnessScore).name()));
    }
    
    // Пересвет: показываем процент пересвеченных пикселей (больше = хуже)
    int overexposedDisplay = static_cast<int>(result.overexposedPercent);  // Процент пересвета
    if (overexposedBar) {
        overexposedBar->setValue(overexposedDisplay);
        // Инвертированный цвет: зеленый = мало пересвета (хорошо), красный = много (плохо)
        overexposedBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }")
            .arg(getQualityColor(100 - overexposedDisplay).name()));
    }
    
    if (scoreLabel) {
        scoreLabel->setText(QString("Оценка: %1").arg(static_cast<int>(result.overallScore)));
        scoreLabel->setStyleSheet(QString("color: %1;").arg(getQualityColor(result.overallScore).name()));
    }
}

void MainWindow::handleConnectionStatus(int cameraId, bool connected, const QString& message)
{
    QList<int> cameraIds = m_cameraWorkers.keys();
    int tabIndex = cameraIds.indexOf(cameraId);
    if (tabIndex < 0) return;
    
    QWidget* tabWidget = m_cameraTabs->widget(tabIndex);
    if (!tabWidget) return;
    
    QLabel* statusLabel = tabWidget->findChild<QLabel*>(QString("statusLabel_%1").arg(cameraId));
    if (statusLabel) {
        statusLabel->setText(QString("Статус: %1").arg(message));
        statusLabel->setStyleSheet(QString("color: %1;").arg(connected ? "#00aa00" : "#ff0000"));
    }
    
    m_statusLabel->setText(message);
}

void MainWindow::handleError(int cameraId, const QString& errorText)
{
    qWarning() << "Camera" << cameraId << "error:" << errorText;
    m_statusLabel->setText(QString("Ошибка: %1").arg(errorText));
}

void MainWindow::handleConnectionLost(int cameraId)
{
    qWarning() << "Connection lost for camera" << cameraId;
    
    QList<int> cameraIds = m_cameraWorkers.keys();
    int tabIndex = cameraIds.indexOf(cameraId);
    if (tabIndex >= 0) {
        QWidget* tabWidget = m_cameraTabs->widget(tabIndex);
        if (tabWidget) {
            QLabel* statusLabel = tabWidget->findChild<QLabel*>(QString("statusLabel_%1").arg(cameraId));
            if (statusLabel) {
                statusLabel->setText("Статус: Переподключение...");
                statusLabel->setStyleSheet("color: #ffaa00;");
            }
        }
    }
}

void MainWindow::updateActivityTimer()
{
    QTime currentTime = QTime::currentTime();
    QString timeString = currentTime.toString("HH:mm:ss");
    m_timeLabel->setText(QString("Текущее время: %1").arg(timeString));
}

int MainWindow::generateCameraId()
{
    return m_nextCameraId++;
}

void MainWindow::stopAllCameras()
{
    for (auto it = m_cameraWorkers.begin(); it != m_cameraWorkers.end(); ++it) {
        CameraWorker* worker = it.value();
        if (worker) {
            worker->stopCapture();
        }
    }
    
    for (auto it = m_cameraThreads.begin(); it != m_cameraThreads.end(); ++it) {
        QThread* thread = it.value();
        if (thread) {
            thread->quit();
            if (!thread->wait(5000)) {
                thread->terminate();
                thread->wait();
            }
        }
    }
    
    m_cameraWorkers.clear();
    m_cameraThreads.clear();
    m_cameraUrls.clear();
    m_frameLabels.clear();
    m_scoreLabels.clear();
}

QColor MainWindow::getQualityColor(double score)
{
    if (score >= 80) {
        return QColor(0, 200, 0);
    } else if (score >= 60) {
        return QColor(220, 200, 0);
    } else if (score >= 40) {
        return QColor(255, 165, 0);
    } else {
        return QColor(255, 0, 0);
    }
}

void MainWindow::setRtspInput(const QString& url)
{
    if (m_rtspInput) {
        m_rtspInput->setText(url);
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "О программе",
        "<html>"
        "<h2>Анализатор качества видеопотока с IP камер</h2>"
        "<p><b>Версия:</b> 0.0.1</p>"
        "<hr>"
        "<p>Анализ качества видео с IP камер по протоколу RTSP.</p>"
        "<p><b>Метрики:</b></p>"
        "<ul>"
        "<li>Шум - уровень шума на изображении</li>"
        "<li>Контраст - диапазон яркости</li>"
        "<li>Резкость - четкость границ</li>"
        "<li>Пересвет - процент переэкспонированных областей</li>"
        "</ul>"
        "<p><b>Итоговая оценка:</b> 0-100 (100 = отличное качество)</p>"
        "<hr>"
        "<p><i>Поддержка нескольких RTSP потоков одновременно.</i></p>"
        "</html>"
    );
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopAllCameras();
    event->accept();
}
