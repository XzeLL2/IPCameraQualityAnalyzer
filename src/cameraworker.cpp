#include "cameraworker.h"
#include <QDebug>
#include <QThread>

CameraWorker::CameraWorker(const QString& rtspUrl, QObject *parent)
    : QObject(parent)
    , m_rtspUrl(rtspUrl)
    , m_frameTimer(nullptr)
    , m_qualityAnalyzer(nullptr)
    , m_reconnectAttempts(0)
    , m_frameSkipCounter(0)
{
    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &CameraWorker::processFrame, Qt::QueuedConnection);
}

CameraWorker::~CameraWorker()
{
    stopCapture();
}

void CameraWorker::startCapture()
{
    if (m_capturing.load()) {
        qWarning() << "Capture already in progress for" << m_rtspUrl;
        return;
    }

    if (!initializeCapture()) {
        emit errorOccurred("Failed to initialize RTSP connection: " + m_rtspUrl);
        return;
    }

    m_capturing.store(true);
    m_connected.store(true);
    m_reconnectAttempts = 0;

    // Создаем анализатор качества при запуске
    if (!m_qualityAnalyzer) {
        m_qualityAnalyzer = new ImageQualityAnalyzer(this);
    }

    m_frameTimer->start(FRAME_INTERVAL_MS);
    emit connectionStatusChanged(true, "Connected to " + m_rtspUrl);
    qInfo() << "Started capture for" << m_rtspUrl;
}

void CameraWorker::stopCapture()
{
    if (!m_capturing.load()) {
        return;
    }

    m_frameTimer->stop();
    m_capturing.store(false);
    m_connected.store(false);
    cleanupCapture();

    emit connectionStatusChanged(false, "Disconnected from " + m_rtspUrl);
    qInfo() << "Stopped capture for" << m_rtspUrl;
}

bool CameraWorker::initializeCapture()
{
    m_videoCapture.open(m_rtspUrl.toStdString(), cv::CAP_FFMPEG);
    
    if (!m_videoCapture.isOpened()) {
        qWarning() << "Failed to open RTSP stream:" << m_rtspUrl;
        return false;
    }
    
    // Отключаем буферизацию для уменьшения задержки
    m_videoCapture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    qInfo() << "Successfully initialized RTSP stream:" << m_rtspUrl;
    return true;
}

void CameraWorker::cleanupCapture()
{
    if (m_videoCapture.isOpened()) {
        m_videoCapture.release();
        qInfo() << "Released video capture for" << m_rtspUrl;
    }
}

void CameraWorker::processFrame()
{
    if (!m_capturing.load()) {
        return;
    }

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

        // Отладочная информация о кадре
        qDebug() << "[CameraWorker] Frame received from" << m_rtspUrl;
        qDebug() << "[CameraWorker] Frame size:" << frame.cols << "x" << frame.rows;
        qDebug() << "[CameraWorker] Frame channels:" << frame.channels();
        qDebug() << "[CameraWorker] Frame type:" << frame.type();
        
        m_reconnectAttempts = 0;

        // Всегда отображаем кадр, даже если качество плохое
        QImage image = ImageQualityAnalyzer::matToQImage(frame);
        if (!image.isNull()) {
            qDebug() << "[CameraWorker] QImage created successfully, size:" << image.size();
            emit frameReady(image);
        } else {
            qWarning() << "[CameraWorker] Failed to convert frame to QImage";
        }

        // Анализ качества выполняется каждые N кадров для уменьшения задержки
        m_frameSkipCounter++;
        if (m_qualityAnalyzer && m_frameSkipCounter >= QUALITY_ANALYSIS_SKIP) {
            m_frameSkipCounter = 0;
            qDebug() << "[CameraWorker] Starting quality analysis for" << m_rtspUrl;
            m_lastQualityResult = m_qualityAnalyzer->analyze(frame);
            qDebug() << "[CameraWorker] Quality result - Valid:" << m_lastQualityResult.isValid 
                     << "Score:" << m_lastQualityResult.overallScore;
            emit qualityResultReady(m_lastQualityResult);
        }
    } catch (...) {
        qWarning() << "Exception in processFrame for" << m_rtspUrl;
    }
}

void CameraWorker::tryReconnect()
{
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        qWarning() << "Max reconnect attempts reached for" << m_rtspUrl;
        emit errorOccurred("Connection lost. Max reconnect attempts reached.");
        stopCapture();
        return;
    }

    m_reconnectAttempts++;
    qInfo() << "Attempting reconnect" << m_reconnectAttempts << "of" << MAX_RECONNECT_ATTEMPTS;

    cleanupCapture();
    QThread::msleep(1000 * m_reconnectAttempts);

    if (initializeCapture()) {
        m_connected.store(true);
        emit connectionStatusChanged(true, "Reconnected to " + m_rtspUrl);
        qInfo() << "Successfully reconnected to" << m_rtspUrl;
    } else {
        qWarning() << "Reconnect attempt failed for" << m_rtspUrl;
        emit connectionStatusChanged(false, "Reconnect failed...");
    }
}

bool CameraWorker::isConnected() const
{
    return m_connected.load();
}

QString CameraWorker::getRtspUrl() const
{
    return m_rtspUrl;
}

ImageQualityAnalyzer::QualityResult CameraWorker::getLastQualityResult() const
{
    return m_lastQualityResult;
}
