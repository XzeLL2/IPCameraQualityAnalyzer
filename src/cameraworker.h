#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include <atomic>
#include "imagequalityanalyzer.h"
#include <opencv2/opencv.hpp>

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(const QString& rtspUrl, QObject *parent = nullptr);
    ~CameraWorker();

    void startCapture();
    void stopCapture();
    bool isConnected() const;
    QString getRtspUrl() const;
    ImageQualityAnalyzer::QualityResult getLastQualityResult() const;

signals:
    void frameReady(const QImage& image);
    void qualityResultReady(const ImageQualityAnalyzer::QualityResult& result);
    void connectionStatusChanged(bool connected, const QString& message);
    void errorOccurred(const QString& errorText);
    void connectionLost();

public slots:
    void processFrame();

private:
    bool initializeCapture();
    void cleanupCapture();
    void tryReconnect();

    QString m_rtspUrl;
    cv::VideoCapture m_videoCapture;
    std::atomic<bool> m_capturing{false};
    std::atomic<bool> m_connected{false};
    QTimer* m_frameTimer;
    ImageQualityAnalyzer* m_qualityAnalyzer;
    ImageQualityAnalyzer::QualityResult m_lastQualityResult;
    int m_reconnectAttempts;
    int m_frameSkipCounter;  // Счётчик для пропуска кадров анализа
    
    const int MAX_RECONNECT_ATTEMPTS = 5;
    const int FRAME_INTERVAL_MS = 33;  // ~30 FPS
    const int QUALITY_ANALYSIS_SKIP = 10;  // Анализ качества каждые 10 кадров
};

#endif // CAMERAWORKER_H
