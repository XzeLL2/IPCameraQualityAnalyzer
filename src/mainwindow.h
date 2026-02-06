#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QMap>
#include <QGroupBox>
#include <QProgressBar>
#include <QTabWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QCloseEvent>
#include <QImage>
#include <QColor>
#include <QMessageBox>
#include <QTime>

#include "cameraworker.h"
#include "imagequalityanalyzer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setRtspInput(const QString& url);
    void showAbout();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void addCamera();
    void removeCamera();
    void updateFrame(int cameraId, const QImage& image);
    void updateQualityResult(int cameraId, const ImageQualityAnalyzer::QualityResult& result);
    void handleConnectionStatus(int cameraId, bool connected, const QString& message);
    void handleError(int cameraId, const QString& errorText);
    void handleConnectionLost(int cameraId);
    void updateActivityTimer();

private:
    void setupUi();
    void addCameraTab(int cameraId, const QString& rtspUrl);
    void removeCameraTab(int cameraId);
    int generateCameraId();
    void stopAllCameras();
    QColor getQualityColor(double score);

    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_inputLayout;
    QLineEdit* m_rtspInput;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QTabWidget* m_cameraTabs;
    QStatusBar* m_statusBar;
    QLabel* m_statusLabel;
    QLabel* m_cameraCountLabel;
    QLabel* m_timeLabel;
    QTimer* m_activityTimer;

    QMap<int, CameraWorker*> m_cameraWorkers;
    QMap<int, QThread*> m_cameraThreads;
    QMap<int, QString> m_cameraUrls;
    QMap<int, QLabel*> m_frameLabels;
    QMap<int, QLabel*> m_scoreLabels;

    int m_nextCameraId;
};

#endif // MAINWINDOW_H
