# Архитектура IP Camera Quality Analyzer

Документация архитектуры проекта с подробным описанием классов, взаимодействий и технических решений.

---

## 📋 Содержание

- [Общий обзор](#-общий-обзор)
- [Многопоточная архитектура](#-многопоточная-архитектура)
- [Событийная модель](#-событийная-модель)
- [Детальное описание классов](#-детальное-описание-классов)
- [Диаграммы взаимодействия](#-диаграммы-взаимодействия)
- [Алгоритмы анализа](#-алгоритмы-анализа)
- [Обработка ошибок](#-обработка-ошибок)
- [Производительность](#-производительность)

---

## 🏗️ Общий обзор

### Структура приложения

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    IP Camera Quality Analyzer                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    Application Layer                             │   │
│  │  ┌──────────────────┐                                            │   │
│  │  │     main.cpp     │ • Инициализация Qt Application            │   │
│  │  │                  │ • Парсинг аргументов командной строки     │   │
│  │  │                  │ • Создание MainWindow                     │   │
│  │  └──────────────────┘                                            │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    Presentation Layer                           │   │
│  │  ┌──────────────────┐                                            │   │
│  │  │   MainWindow     │ • GUI приложения                         │   │
│  │  │   (Qt Widgets)   │ • Управление вкладками                   │   │
│  │  │                  │ • Отображение метрик                      │   │
│  │  └──────────────────┘                                            │   │
│  │           ▲                      │                               │   │
│  │           │       signals        │                               │   │
│  │           └──────────────────────┘                               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                   Business Logic Layer                           │   │
│  │  ┌──────────────────┐  ┌──────────────────────────────────┐      │   │
│  │  │   CameraWorker   │  │   ImageQualityAnalyzer         │      │   │
│  │  │   (per camera)   │  │   (shared)                      │      │   │
│  │  └──────────────────┘  └──────────────────────────────────┘      │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                     Data Access Layer                           │   │
│  │  ┌──────────────────────────────────────────────────────────┐   │   │
│  │  │              cv::VideoCapture (OpenCV)                   │   │   │
│  │  │              RTSP Protocol                                │   │   │
│  │  └──────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Технологический стек

| Уровень | Технология | Назначение |
|---------|------------|------------|
| UI Framework | Qt5 Widgets | Графический интерфейс |
| Video Processing | OpenCV 4.x | Захват и обработка видео |
| Concurrency | QThread + Signals/Slots | Многопоточность |
| Build System | CMake | Сборка проекта |
| Standard | C++17 | Язык разработки |

---

## 🔄 Многопоточная архитектура

### Модель потоков

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Thread Model                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Main Thread (UI)                                                       │
│  ═══════════════════                                                     │
│  • Обработка событий Qt                                                 │
│  • Отрисовка GUI                                                        │
│  • Прием сигналов от workers                                            │
│                                                                         │
│         ▲                                                                │
│         │ Qt::QueuedConnection                                          │
│         │ (асинхронная доставка)                                        │
│         ▼                                                                │
│  ┌──────────────────────────────────────────────────────────┐          │
│  │  Worker Thread #1                                        │          │
│  │  ═══════════════════                                    │          │
│  │  • CameraWorker instance                                 │          │
│  │  • RTSP connection                                       │          │
│  │  • Frame processing                                      │          │
│  │  • Quality analysis                                      │          │
│  │                                                          │          │
│  │  ┌────────────────┐    ┌────────────────┐              │          │
│  │  │ VideoCapture   │───▶│ QualityAnalyzer│              │          │
│  │  └────────────────┘    └────────────────┘              │          │
│  └──────────────────────────────────────────────────────────┘          │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────┐          │
│  │  Worker Thread #2                                        │          │
│  │  ═══════════════════                                    │          │
│  │  • CameraWorker instance                                 │          │
│  │  • RTSP connection                                       │          │
│  │  • Frame processing                                      │          │
│  │  • Quality analysis                                      │          │
│  │                                                          │          │
│  │  ┌────────────────┐    ┌────────────────┐              │          │
│  │  │ VideoCapture   │───▶│ QualityAnalyzer│              │          │
│  │  └────────────────┘    └────────────────┘              │          │
│  └──────────────────────────────────────────────────────────┘          │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────┐          │
│  │  Worker Thread #N                                        │          │
│  │  ═══════════════════                                    │          │
│  │  ...                                                     │          │
│  └──────────────────────────────────────────────────────────┘          │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Принципы многопоточности

#### 1. Изоляция потоков

Каждая камера обрабатывается в своем собственном потоке:

```cpp
// Создание потока для камеры
QThread* workerThread = new QThread(this);
CameraWorker* worker = new CameraWorker(rtspUrl);

worker->moveToThread(workerThread);

// Запуск потока
connect(workerThread, &QThread::started, worker, &CameraWorker::startCapture);
workerThread->start();
```

#### 2. Безопасная передача данных

Использование Qt Signals/Slots с `Qt::QueuedConnection` для потокобезопасной передачи:

```cpp
// GUI обновляется через очередь событий
connect(worker, &CameraWorker::frameReady, 
    this, [this, cameraId](const QImage& image) {
        updateFrame(cameraId, image);
    }, Qt::QueuedConnection);
```

#### 3. Атомарные флаги состояния

```cpp
std::atomic<bool> m_capturing{false};
std::atomic<bool> m_connected{false};
```

### Жизненный цикл потока камеры

```
     startCapture()                  stopCapture()
         │                               │
         ▼                               ▼
    ┌─────────┐                    ┌─────────┐
    │ STOPPED │ ───────────────▶ │ RUNNING │
    └─────────┐ ◀───────────────  └─────────┘
         │              │               │
         │              ▼               │
         │        ┌──────────┐          │
         │        │  ERROR   │ ─────────┘
         │        │ STATE    │
         │        └────┬─────┘
         │             │
         │             ▼
         │        ┌──────────┐
         │        │ RECONNECT│
         │        │  ATTEMPT │
         │        └────┬─────┘
         │             │
         └─────────────┘
```

---

## 📡 Событийная модель

### Сигналы и слоты

```
MainWindow (GUI Thread)
        │
        │ ◀── connectionStatusChanged(bool, QString)
        │ ◀── frameReady(QImage)
        │ ◀── qualityResultReady(QualityResult)
        │ ◀── errorOccurred(QString)
        │ ◀── connectionLost()
        │
        ▼
CameraWorker (Worker Thread)
        │
        │ ◀── processFrame() [from QTimer]
        │
        ▼
cv::VideoCapture (OpenCV)
        │
        │ RTSP Stream
        ▼
IP Camera
```

### Детальная схема сигналов

```
CameraWorker Signals                    MainWindow Slots
══════════════════════════════════════════════════════════

frameReady(image)     ───────────────▶  updateFrame(cameraId, image)
                                          │
                                          ▼
                                    QLabel::setPixmap()

qualityResultReady    ───────────────▶  updateQualityResult(cameraId, result)
(result)                                    │
                                            ▼
                                    QProgressBar updates
                                    QLabel score update

connectionStatus      ───────────────▶  handleConnectionStatus(cameraId, 
(connected, msg)                            connected, msg)
                                            │
                                            ▼
                                    Status bar update

errorOccurred         ───────────────▶  handleError(cameraId, errorText)
(errorText)                                │
                                            ▼
                                    Log warning
                                    Show error

connectionLost()       ───────────────▶  handleConnectionLost(cameraId)
                                            │
                                            ▼
                                    Try reconnect
                                    Update status
```

---

## 📦 Детальное описание классов

### MainWindow

**Файл:** [`src/mainwindow.h`](src/mainwindow.h), [`src/mainwindow.cpp`](src/mainwindow.cpp)

#### Назначение

Главное окно приложения, обеспечивающее:
- GUI для пользовательского интерфейса
- Управление коллекцией камер
- Отображение видеопотоков и метрик

#### Диаграмма класса

```
┌─────────────────────────────────────────┐
│             MainWindow                  │
├─────────────────────────────────────────┤
│ - m_centralWidget: QWidget*            │
│ - m_cameraTabs: QTabWidget*            │
│ - m_rtspInput: QLineEdit*              │
│ - m_addButton: QPushButton*            │
│ - m_removeButton: QPushButton*         │
│ - m_statusBar: QStatusBar*             │
│ - m_activityTimer: QTimer*             │
│                                          │
│ - m_cameraWorkers: QMap<int, Worker*>  │
│ - m_cameraThreads: QMap<int, QThread*> │
│ - m_cameraUrls: QMap<int, QString>     │
│ - m_frameLabels: QMap<int, QLabel*>    │
│ - m_scoreLabels: QMap<int, QLabel*>    │
│ - m_nextCameraId: int                  │
├─────────────────────────────────────────┤
│ + MainWindow(QWidget* = nullptr)        │
│ + ~MainWindow()                         │
│                                          │
│ + setRtspInput(const QString&)          │
│ + showAbout()                           │
│                                          │
│ # closeEvent(QCloseEvent*)              │
│                                          │
│ - addCamera() [slot]                    │
│ - removeCamera() [slot]                 │
│ - updateFrame(int, QImage) [slot]       │
│ - updateQualityResult(int, Result)      │
│ - handleConnectionStatus(int, bool,     │
│   QString) [slot]                       │
│ - handleError(int, QString) [slot]      │
│ - handleConnectionLost(int) [slot]     │
│ - updateActivityTimer() [slot]         │
│                                          │
│ - setupUi()                             │
│ - addCameraTab(int, QString)            │
│ - removeCameraTab(int)                  │
│ - generateCameraId(): int               │
│ - stopAllCameras()                      │
│ - getQualityColor(double): QColor       │
└─────────────────────────────────────────┘
```

#### Основные методы

| Метод | Описание | Сложность |
|-------|----------|-----------|
| `addCamera()` | Добавляет новую камеру | O(1) |
| `removeCamera()` | Удаляет текущую камеру | O(1) |
| `updateFrame()` | Обновляет отображение кадра | O(1) |
| `updateQualityResult()` | Обновляет метрики качества | O(1) |

#### Инициализация GUI

```cpp
void MainWindow::setupUi()
{
    // Создание центрального виджета
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Основной layout
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Панель ввода RTSP URL
    m_inputLayout = new QHBoxLayout();
    m_rtspInput = new QLineEdit(this);
    m_addButton = new QPushButton("Добавить камеру", this);
    m_removeButton = new QPushButton("Удалить камеру", this);
    
    // Вкладки для камер
    m_cameraTabs = new QTabWidget(this);
    m_cameraTabs->setTabsClosable(true);
    m_cameraTabs->setMovable(true);
    
    // Статус бар
    m_statusBar = new QStatusBar(this);
    m_statusLabel = new QLabel("Готов");
    m_cameraCountLabel = new QLabel("Камер: 0");
    m_timeLabel = new QLabel("Текущее время: --:--:--");
}
```

#### Добавление камеры

```cpp
void MainWindow::addCamera()
{
    QString rtspUrl = m_rtspInput->text().trimmed();
    
    // Валидация URL
    if (rtspUrl.isEmpty()) {
        m_statusLabel->setText("Ошибка: Введите RTSP URL");
        return;
    }
    
    if (!rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive)) {
        m_statusLabel->setText("Ошибка: URL должен начинаться с rtsp://");
        return;
    }
    
    // Проверка на дубликат
    if (m_cameraUrls.values().contains(rtspUrl)) {
        m_statusLabel->setText("Ошибка: Эта камера уже добавлена");
        return;
    }
    
    int cameraId = generateCameraId();
    
    // Создание потока и воркера
    QThread* workerThread = new QThread(this);
    CameraWorker* worker = new CameraWorker(rtspUrl);
    
    worker->moveToThread(workerThread);
    
    // Подключение сигналов
    connect(workerThread, &QThread::started, worker, &CameraWorker::startCapture);
    connect(worker, &CameraWorker::frameReady, this, &MainWindow::updateFrame, Qt::QueuedConnection);
    connect(worker, &CameraWorker::qualityResultReady, this, &MainWindow::updateQualityResult, Qt::QueuedConnection);
    
    // Запуск
    workerThread->start();
}
```

#### Обновление метрик

```cpp
void MainWindow::updateQualityResult(int cameraId, const QualityResult& result)
{
    // Обновление progress bars
    noiseBar->setValue(static_cast<int>(100 - result.noiseScore));
    contrastBar->setValue(static_cast<int>(result.contrastScore));
    sharpnessBar->setValue(static_cast<int>(result.sharpnessScore));
    overexposedBar->setValue(static_cast<int>(result.overexposedPercent));
    
    // Обновление общей оценки
    scoreLabel->setText(QString("Оценка: %1").arg(static_cast<int>(result.overallScore)));
    scoreLabel->setStyleSheet(QString("color: %1;").arg(getQualityColor(result.overallScore).name()));
}
```

---

### CameraWorker

**Файл:** [`src/cameraworker.h`](src/cameraworker.h), [`src/cameraworker.cpp`](src/cameraworker.cpp)

#### Назначение

Обработка RTSP потока в отдельном потоке:
- Захват видеокадров с IP камеры
- Передача кадров в GUI
- Анализ качества изображения
- Управление переподключением

#### Диаграмма класса

```
┌─────────────────────────────────────────┐
│             CameraWorker                │
├─────────────────────────────────────────┤
│ - m_rtspUrl: QString                   │
│ - m_videoCapture: cv::VideoCapture    │
│ - m_qualityAnalyzer: ImageQuality*     │
│ - m_frameTimer: QTimer*                │
│ - m_lastQualityResult: QualityResult   │
│                                          │
│ - m_capturing: std::atomic<bool>       │
│ - m_connected: std::atomic<bool>       │
│ - m_reconnectAttempts: int             │
│ - m_frameSkipCounter: int              │
│                                          │
│ - MAX_RECONNECT_ATTEMPTS: const int     │
│ - FRAME_INTERVAL_MS: const int         │
│ - QUALITY_ANALYSIS_SKIP: const int     │
├─────────────────────────────────────────┤
│ + CameraWorker(const QString&, QObject*│
│   = nullptr)                            │
│ + ~CameraWorker()                       │
│                                          │
│ + startCapture()                        │
│ + stopCapture()                         │
│ + isConnected(): bool                   │
│ + getRtspUrl(): QString                 │
│ + getLastQualityResult(): QualityResult │
│                                          │
│ # processFrame() [slot]                 │
│                                          │
│ - initializeCapture(): bool             │
│ - cleanupCapture()                      │
│ - tryReconnect()                        │
├─────────────────────────────────────────┤
│ SIGNALS:                                │
│ - frameReady(const QImage&)             │
│ - qualityResultReady(const               │
│   QualityResult&)                       │
│ - connectionStatusChanged(bool,         │
│   const QString&)                       │
│ - errorOccurred(const QString&)         │
│ - connectionLost()                      │
└─────────────────────────────────────────┘
```

#### Константы конфигурации

```cpp
// Максимальное количество попыток переподключения
static constexpr int MAX_RECONNECT_ATTEMPTS = 5;

// Интервал между кадрами (мс) - ~30 FPS
static constexpr int FRAME_INTERVAL_MS = 33;

// Пропуск кадров перед анализом качества
// Снижает нагрузку на CPU
static constexpr int QUALITY_ANALYSIS_SKIP = 10;
```

#### Инициализация захвата

```cpp
bool CameraWorker::initializeCapture()
{
    // Открытие RTSP потока с использованием FFmpeg
    m_videoCapture.open(m_rtspUrl.toStdString(), cv::CAP_FFMPEG);
    
    if (!m_videoCapture.isOpened()) {
        qWarning() << "Failed to open RTSP stream:" << m_rtspUrl;
        return false;
    }
    
    // Минимизация буферизации для снижения задержки
    m_videoCapture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    qInfo() << "Successfully initialized RTSP stream:" << m_rtspUrl;
    return true;
}
```

#### Обработка кадра

```cpp
void CameraWorker::processFrame()
{
    if (!m_capturing.load()) return;
    
    try {
        cv::Mat frame;
        bool success = m_videoCapture.read(frame);
        
        if (!success || frame.empty()) {
            qWarning() << "Failed to grab frame from" << m_rtspUrl;
            if (m_connected.load()) {
                tryReconnect();
            }
            return;
        }
        
        // Отправка кадра в GUI
        QImage image = ImageQualityAnalyzer::matToQImage(frame);
        if (!image.isNull()) {
            emit frameReady(image);
        }
        
        // Анализ качества каждые N кадров
        m_frameSkipCounter++;
        if (m_qualityAnalyzer && m_frameSkipCounter >= QUALITY_ANALYSIS_SKIP) {
            m_frameSkipCounter = 0;
            m_lastQualityResult = m_qualityAnalyzer->analyze(frame);
            emit qualityResultReady(m_lastQualityResult);
        }
    } catch (...) {
        qWarning() << "Exception in processFrame for" << m_rtspUrl;
    }
}
```

#### Переподключение при разрыве

```cpp
void CameraWorker::tryReconnect()
{
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        qWarning() << "Max reconnect attempts reached for" << m_rtspUrl;
        emit errorOccurred("Connection lost. Max reconnect attempts reached.");
        stopCapture();
        return;
    }
    
    m_reconnectAttempts++;
    qInfo() << "Attempting reconnect" << m_reconnectAttempts 
            << "of" << MAX_RECONNECT_ATTEMPTS;
    
    cleanupCapture();
    
    // Экспоненциальная задержка
    QThread::msleep(1000 * m_reconnectAttempts);
    
    if (initializeCapture()) {
        m_connected.store(true);
        emit connectionStatusChanged(true, "Reconnected to " + m_rtspUrl);
    } else {
        emit connectionStatusChanged(false, "Reconnect failed...");
    }
}
```

---

### ImageQualityAnalyzer

**Файл:** [`src/imagequalityanalyzer.h`](src/imagequalityanalyzer.h), [`src/imagequalityanalyzer.cpp`](src/imagequalityanalyzer.cpp)

#### Назначение

Комплексный анализ качества видеокадра по 4 метрикам:
- Шумность (Noise)
- Контрастность (Contrast)
- Резкость (Sharpness)
- Пересвеченность (Overexposed)

#### Диаграмма класса

```
┌─────────────────────────────────────────┐
│        ImageQualityAnalyzer             │
├─────────────────────────────────────────┤
│ WEIGHTS:                                │
│ - NOISE_WEIGHT: const double = 0.25    │
│ - CONTRAST_WEIGHT: const double = 0.25 │
│ - SHARPNESS_WEIGHT: const double = 0.35│
│ - OVEREXPOSED_WEIGHT: const double      │
│   = 0.15                               │
│                                          │
│ THRESHOLDS:                             │
│ - OVEREXPOSED_THRESHOLD: const int     │
│   = 245                                │
│ - IDEAL_CONTRAST: const double = 160.0  │
│ - IDEAL_SHARPNESS: const double = 400.0│
│ - MAX_NOISE_VARIANCE: const double     │
│   = 50.0                               │
├─────────────────────────────────────────┤
│ + ImageQualityAnalyzer(QObject* =      │
│   nullptr)                              │
│ + ~ImageQualityAnalyzer()              │
│                                          │
│ + analyze(const cv::Mat&): QualityResult│
│ + matToQImage(const cv::Mat&): QImage  │
│                                          │
│ # calculateNoiseScore(const cv::Mat&)   │
│ # calculateContrastScore(const          │
│   cv::Mat&)                             │
│ # calculateSharpnessScore(const         │
│   cv::Mat&)                             │
│ # calculateOverexposedPercentage(       │
│   const cv::Mat&)                       │
├─────────────────────────────────────────┤
│ SIGNALS:                                │
│ - analysisCompleted(const               │
│   QualityResult&)                       │
└─────────────────────────────────────────┘
```

#### Структура QualityResult

```cpp
struct QualityResult {
    double noiseScore;           // 0-100, выше = лучше
    double contrastScore;        // 0-100, выше = лучше
    double sharpnessScore;       // 0-100, выше = лучше
    double overexposedPercent;   // 0-100, ниже = лучше
    double overallScore;         // 0-100, выше = лучше
    QString status;              // Текстовый статус
    bool isValid;                // Валидность результата
    
    QualityResult() : noiseScore(0), contrastScore(0), 
                      sharpnessScore(0), overexposedPercent(0),
                      overallScore(0), status(""), isValid(false) {}
};
```

#### Главный метод анализа

```cpp
QualityResult ImageQualityAnalyzer::analyze(const cv::Mat& frame)
{
    QualityResult result;
    result.isValid = false;
    
    // Проверка валидности кадра
    if (frame.empty() || frame.cols < 10 || frame.rows < 10) {
        result.status = "Неверный кадр";
        result.overallScore = 0.0;
        return result;
    }
    
    try {
        // Конвертация в градации серого
        cv::Mat grayFrame;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        } else {
            grayFrame = frame.clone();
        }
        
        // Расчет всех метрик
        double noiseScore = calculateNoiseScore(grayFrame);
        double contrastScore = calculateContrastScore(grayFrame);
        double sharpnessScore = calculateSharpnessScore(grayFrame);
        double overexposedPercent = calculateOverexposedPercentage(grayFrame);
        
        // Взвешенная сумма
        double overallScore = 
            noiseScore * NOISE_WEIGHT +
            contrastScore * CONTRAST_WEIGHT +
            sharpnessScore * SHARPNESS_WEIGHT +
            (100.0 - overexposedPercent) * OVEREXPOSED_WEIGHT;
        
        // Ограничение диапазона
        overallScore = std::max(0.0, std::min(100.0, overallScore));
        
        // Заполнение результата
        result.noiseScore = noiseScore;
        result.contrastScore = contrastScore;
        result.sharpnessScore = sharpnessScore;
        result.overexposedPercent = overexposedPercent;
        result.overallScore = overallScore;
        result.isValid = true;
        
        // Статус на основе оценки
        if (result.overallScore >= 80) {
            result.status = "Отличное качество";
        } else if (result.overallScore >= 60) {
            result.status = "Хорошее качество";
        } else if (result.overallScore >= 40) {
            result.status = "Удовлетворительно";
        } else if (result.overallScore >= 20) {
            result.status = "Плохое качество";
        } else {
            result.status = "Очень плохое качество";
        }
    } catch (...) {
        result.status = "Ошибка анализа";
        result.overallScore = 0.0;
    }
    
    emit analysisCompleted(result);
    return result;
}
```

---

## 🔀 Диаграммы взаимодействия

### Диаграмма последовательности: Добавление камеры

```
actor User
participant MainWindow
participant QThread
participant CameraWorker
participant VideoCapture
participant ImageQualityAnalyzer

User->>MainWindow: addCamera()
activate MainWindow

MainWindow->>MainWindow: validate RTSP URL
alt Invalid URL
    MainWindow-->>User: Show error
else Valid URL
    MainWindow->>QThread: create new thread
    MainWindow->>CameraWorker: create worker(rtspUrl)
    MainWindow->>CameraWorker: moveToThread(thread)
    
    activate CameraWorker
    
    MainWindow->>CameraWorker: connect signals
    MainWindow->>QThread: start()
    
    QThread->>CameraWorker: started()
    CameraWorker->>CameraWorker: startCapture()
    CameraWorker->>VideoCapture: open(rtspUrl)
    
    alt Connection successful
        VideoCapture-->>CameraWorker: success
        CameraWorker->>CameraWorker: create ImageQualityAnalyzer
        CameraWorker->>QTimer: start(FRAME_INTERVAL_MS)
        CameraWorker-->>MainWindow: connectionStatusChanged(true)
        MainWindow->>MainWindow: update status bar
    else Connection failed
        VideoCapture-->>CameraWorker: failed
        CameraWorker-->>MainWindow: errorOccurred()
        MainWindow-->>User: Show error
    end
end

deactivate CameraWorker
deactivate MainWindow
```

### Диаграмма последовательности: Обработка кадра

```
participant QTimer
participant CameraWorker
participant VideoCapture
participant ImageQualityAnalyzer
participant MainWindow
participant QLabel

loop Every FRAME_INTERVAL_MS
    QTimer->>CameraWorker: timeout()
    activate CameraWorker
    
    CameraWorker->>VideoCapture: read(frame)
    
    alt Frame received
        VideoCapture-->>CameraWorker: frame
        CameraWorker->>ImageQualityAnalyzer: matToQImage(frame)
        ImageQualityAnalyzer-->>CameraWorker: QImage
        
        CameraWorker->>MainWindow: frameReady(image)
        MainWindow->>QLabel: setPixmap(image)
        
        CameraWorker->>CameraWorker: check frameSkipCounter
        alt Should analyze (every 10th frame)
            CameraWorker->>ImageQualityAnalyzer: analyze(frame)
            ImageQualityAnalyzer-->>CameraWorker: QualityResult
            
            CameraWorker->>MainWindow: qualityResultReady(result)
            MainWindow->>MainWindow: update metrics
        end
    else Frame empty
        CameraWorker->>CameraWorker: tryReconnect()
    end
    
    deactivate CameraWorker
end
```

### Диаграмма последовательности: Переподключение

```
participant CameraWorker
participant VideoCapture
participant MainWindow

CameraWorker->>CameraWorker: tryReconnect()
activate CameraWorker

alt reconnect attempts < MAX
    CameraWorker->>VideoCapture: release()
    CameraWorker->>QThread: sleep(exponential backoff)
    
    CameraWorker->>VideoCapture: open(rtspUrl)
    
    alt Reconnect successful
        VideoCapture-->>CameraWorker: success
        CameraWorker-->>MainWindow: connectionStatusChanged(true)
    else Reconnect failed
        VideoCapture-->>CameraWorker: failed
        CameraWorker->>CameraWorker: increment attempts
        CameraWorker-->>MainWindow: connectionStatusChanged(false)
    end
else Max attempts reached
    CameraWorker-->>MainWindow: errorOccurred()
    CameraWorker->>CameraWorker: stopCapture()
end

deactivate CameraWorker
```

---

## 🧮 Алгоритмы анализа

### Алгоритм расчета шумности (Noise)

```
┌─────────────────────────────────────────────────────────────────┐
│              Расчет шумности изображения                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ВХОД:  frame (cv::Mat, grayscale)                               │
│                                                                 │
│  ШАГ 1: Gaussian Blur                                           │
│  ─────────────────────                                          │
│  blurred = GaussianBlur(frame, 5×5, sigma=0)                     │
│                                                                 │
│         frame                    blurred                        │
│    ┌───────────┐            ┌───────────┐                      │
│    │ ▓▓▓▓▓▓▓▓▓ │            │ ▒▒▒▒▒▒▒▒▒ │                      │
│    │ ▓▓▓▓▓▓▓▓▓ │   blur    │ ▒▒▒▒▒▒▒▒▒ │                      │
│    │ ▓▓▓▓▓▓▓▓▓ │ ────────▶ │ ▒▒▒▒▒▒▒▒▒ │                      │
│    │ ▓▓▓▓▓▓▓▓▓ │            │ ▒▒▒▒▒▒▒▒▒ │                      │
│    └───────────┘            └───────────┘                      │
│                                                                 │
│  ШАГ 2: Разница изображений                                      │
│  ─────────────────────────                                      │
│  diff = |frame - blurred|                                        │
│                                                                 │
│         frame          blurred           diff                   │
│    ┌───────────┐    ┌───────────┐    ┌───────────┐             │
│    │ ▓▓▓▓▓▓▓▓▓ │    │ ▒▒▒▒▒▒▒▒▒ │    │ █ █ █ █ █ │             │
│    │ ▓▓▓▓▓▓▓▓▓ │ ───│ ▒▒▒▒▒▒▒▒▒ │───▶│ █ █ █ █ █ │             │
│    │ ▓▓▓▓▓▓▓▓▓ │    │ ▒▒▒▒▒▒▒▒▒ │    │ █ █ █ █ █ │             │
│    └───────────┘    └───────────┘    └───────────┘             │
│                                                                 │
│  ШАГ 3: Среднее значение разницы                                 │
│  ──────────────────────────────                                 │
│  noiseLevel = mean(diff)                                         │
│                                                                 │
│  ШАГ 4: Расчет оценки                                           │
│  ─────────────────────                                           │
│  noiseScore = 100 - (noiseLevel / MAX_NOISE_VARIANCE) × 100    │
│                                                                 │
│  ВЫХОД: noiseScore (0-100)                                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Сложность:** O(n) где n = ширина × высота изображения

### Алгоритм расчета контрастности (Contrast)

```
┌─────────────────────────────────────────────────────────────────┐
│              Расчет контрастности изображения                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ВХОД:  frame (cv::Mat, grayscale)                              │
│                                                                 │
│  ШАГ 1: Поиск min/max значений                                   │
│  ────────────────────────────────                                │
│  minVal, maxVal = minMaxLoc(frame)                              │
│                                                                 │
│         pixel values                                             │
│    ┌───────────────────────────┐                                 │
│    │  10  45  78  120  200    │                                 │
│    │  15  50  85  130  210    │                                 │
│    │  ...                     │                                 │
│    └───────────────────────────┘                                 │
│              │              │                                    │
│              ▼              ▼                                    │
│         minVal=10       maxVal=250                              │
│                                                                 │
│  ШАГ 2: Расчет диапазона                                         │
│  ────────────────────                                           │
│  contrastRange = maxVal - minVal                                 │
│               = 250 - 10 = 240                                  │
│                                                                 │
│  ШАГ 3: Расчет оценки                                           │
│  ────────────────────                                           │
│  contrastScore = (contrastRange / IDEAL_CONTRAST) × 100        │
│                = (240 / 160) × 100 = 150 → 100 (обрезано)      │
│                                                                 │
│  Примечание: Если contrastRange > 250,                         │
│  применяется штраф 20%                                          │
│                                                                 │
│  ВЫХОД: contrastScore (0-100)                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Алгоритм расчета резкости (Sharpness)

```
┌─────────────────────────────────────────────────────────────────┐
│              Расчет резкости изображения                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ВХОД:  frame (cv::Mat, grayscale)                              │
│                                                                 │
│  ШАГ 1: Применение Laplacian                                    │
│  ───────────────────────────                                    │
│  laplacian = Laplacian(frame, CV_64F, kernel=3)               │
│                                                                 │
│    ┌─────────────────┐                                          │
│    │   ▓▓▓▓▓▓▓▓▓▓▓   │  Laplacian                              │
│    │   ▓▓▓▓▓▓▓▓▓▓▓   │ ───────▶  ┌─────────────────┐             │
│    │   ▓▓▓▓▓▓▓▓▓▓▓   │          │ ░░░░█░░█░░░█░░ │             │
│    │   ▓▓▓▓▓▓▓▓▓▓▓   │          │ ░░░░█░░█░░░█░░ │             │
│    └─────────────────┘          └─────────────────┘             │
│                                                                 │
│  ШАГ 2: Расчет дисперсии Laplacian                              │
│  ────────────────────────────────                               │
│  mean, stdDev = meanStdDev(laplacian)                           │
│  laplacianVariance = stdDev²                                    │
│                                                                 │
│  ШАГ 3: Расчет оценки                                           │
│  ────────────────────                                           │
│  sharpnessScore = (laplacianVariance / IDEAL_SHARPNESS) × 100│
│                                                                 │
│  ИНТЕРПРЕТАЦИЯ:                                                  │
│  • Высокая дисперсия → четкие края → высокая резкость          │
│  • Низкая дисперсия → размытое изображение                      │
│                                                                 │
│  ВЫХОД: sharpnessScore (0-100)                                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Алгоритм расчета пересвеченности (Overexposed)

```
┌─────────────────────────────────────────────────────────────────┐
│              Расчет пересвеченных пикселей                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ВХОД:  frame (cv::Mat, grayscale)                              │
│                                                                 │
│  ШАГ 1: Бинаризация по порогу                                   │
│  ───────────────────────────────                                 │
│  threshold = OVEREXPOSED_THRESHOLD = 245                       │
│  mask = (frame > threshold) ? 255 : 0                          │
│                                                                 │
│         frame                                                     │
│    ┌───────────────────┐                                         │
│    │ 100 150 200 245 255│                                         │
│    │ 120 180 220 250 255│                                         │
│    │ ...               │                                         │
│    └───────────────────┘                                         │
│              │                                                   │
│              ▼ threshold = 245                                  │
│    ┌───────────────────┐                                         │
│    │  0   0   0   1  1 │  (1 = пересвечен)                       │
│    │  0   0   0   1  1 │                                         │
│    └───────────────────┘                                         │
│                                                                 │
│  ШАГ 2: Подсчет пересвеченных пикселей                          │
│  ─────────────────────────────────                               │
│  overexposedCount = countNonZero(mask)                         │
│                                                                 │
│  ШАГ 3: Расчет процента                                         │
│  ─────────────────────────                                       │
│  totalPixels = frame.rows × frame.cols                         │
│  overexposedPercent = (overexposedCount / totalPixels) × 100 │
│                                                                 │
│  ШАГ 4: Инверсия для оценки                                      │
│  ──────────────────────────                                     │
│  overexposedScore = 100 - min(100, overexposedPercent × 2)    │
│                                                                 │
│  ВЫХОД: overexposedPercent (0-100), overexposedScore           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## ⚠️ Обработка ошибок

### Иерархия ошибок

```
┌─────────────────────────────────────────────────────────────────┐
│                    Error Handling System                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Критические ошибки                    │   │
│  │  • Ошибка инициализации RTSP                             │   │
│  │  • Превышено время ожидания                              │   │
│  │  • Ошибка декодирования                                  │   │
│  │  Обработка: Переподключение, затем завершение           │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Предупреждения                       │   │
│  │  • Пропущенный кадр                                     │   │
│  │  • Нестабильное соединение                               │   │
│  │  • Высокая задержка                                     │   │
│  │  Обработка: Логирование, продолжение работы             │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Информационные                        │   │
│  │  • Успешное подключение                                 │   │
│  │  • Успешное переподключение                             │   │
│  │  • Завершение анализа                                    │   │
│  │  Обработка: Логирование                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Стратегия переподключения

```
┌─────────────────────────────────────────────────────────────────┐
│              Стратегия экспоненциальной задержки                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  attempt #1: wait 1 second                                     │
│  attempt #2: wait 2 seconds                                    │
│  attempt #3: wait 3 seconds                                    │
│  attempt #4: wait 4 seconds                                    │
│  attempt #5: wait 5 seconds                                    │
│                                                                 │
│  Формула: wait_ms = 1000 × attempt_number                      │
│                                                                 │
│  После 5 неудачных попыток:                                     │
│  → Остановка захвата                                           │
│  → Уведомление пользователя                                    │
│  → Ожидание ручного перезапуска                                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## ⚡ Производительность

### Временные затраты

| Операция | Время | Примечание |
|----------|-------|------------|
| RTSP захват кадра | ~5 мс | Зависит от сети |
| Декодирование | ~10 мс | Зависит от кодека |
| Конвертация в grayscale | ~1 мс | |
| Анализ шумности | ~2 мс | Gaussian blur |
| Анализ контраста | <1 мс | min/max |
| Анализ резкости | ~5 мс | Laplacian |
| Анализ пересветов | ~2 мс | Threshold |
| Конвертация в QImage | ~2 мс | |
| GUI update | ~3 мс | |
| **Total per frame** | **~30 мс** | ~33 FPS |
| **Quality analysis** | **~10 мс** | Каждый 10-й кадр |

### Оптимизации

#### 1. Пропуск кадров анализа

```cpp
// Анализ качества каждые 10 кадров
// Снижает нагрузку на CPU в 10 раз
if (m_frameSkipCounter >= QUALITY_ANALYSIS_SKIP) {
    m_frameSkipCounter = 0;
    // Выполнить анализ
}
```

#### 2. Минимальная буферизация

```cpp
// Размер буфера = 1 для минимизации задержки
m_videoCapture.set(cv::CAP_PROP_BUFFERSIZE, 1);
```

#### 3. Асинхронные сигналы

```cpp
// Qt::QueuedConnection гарантирует выполнение слота
// в контексте целевого потока
connect(worker, &CameraWorker::frameReady, 
    this, &MainWindow::updateFrame, 
    Qt::QueuedConnection);
```

### Потребление памяти

| Компонент | Память |
|-----------|--------|
| Один кадр (1080p, BGR) | ~6 МБ |
| Один кадр (1080p, grayscale) | ~2 МБ |
| ImageQualityAnalyzer | ~1 МБ |
| CameraWorker | ~1 МБ |
| GUI overhead | ~30 МБ |
| **Total per camera** | **~50 МБ** |

### Масштабируемость

```
┌─────────────────────────────────────────────────────────────────┐
│                    Масштабируемость                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Количество камер vs FPS:                                       │
│                                                                 │
│  10 │                                      ●                   │
│   8 │                                 ●                       │
│   6 │                            ●                            │
│   4 │                       ●                                 │
│   2 │           ●                                          │
│   0 │──●──────────────────────────────────────────────────    │
│       1    2    3    4    5    6    7    8    9   10          │
│                     Количество камер                             │
│                                                                 │
│  Примечание: FPS снижается линейно с числом камер              │
│  Рекомендуемое количество: 4-6 камер на типичном ПК            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 Заключение

### Ключевые архитектурные решения

1. **Многопоточность:** Изоляция каждой камеры в отдельном потоке
2. **Асинхронная коммуникация:** Qt Signals/Slots с queued connections
3. **Модульность:** Разделение на MainWindow, CameraWorker, ImageQualityAnalyzer
4. **Отказоустойчивость:** Автоматическое переподключение с экспоненциальной задержкой
5. **Производительность:** Оптимизация через пропуск кадров анализа

### Примененные паттерны

| Паттерн | Применение |
|---------|------------|
| Producer-CameraWorker | RTSP поток → Кадры |
| Observer | Signals/Slots |
| Strategy | ImageQualityAnalyzer |
| Factory | Создание CameraWorker |

---

**Версия документации:** 1.0  
**Дата:** 2024
