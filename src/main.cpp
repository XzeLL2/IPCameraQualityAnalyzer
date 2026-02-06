#include <QApplication>
#include <QMainWindow>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMetaType>
#include <iostream>

#include "mainwindow.h"
#include "imagequalityanalyzer.h"

// Регистрация метатипа для передачи между потоками
Q_DECLARE_METATYPE(ImageQualityAnalyzer::QualityResult)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QCoreApplication::setApplicationName("IP Camera Video Quality Analyzer");
    QCoreApplication::setApplicationVersion("0.0.1");
    QCoreApplication::setOrganizationName("Danila Ivlev");
    QCoreApplication::setOrganizationDomain("danila-ivlev");
    
    QApplication::setApplicationDisplayName("Анализатор качества видеопотока с IP камер");
    QApplication::setWindowIcon(QIcon::fromTheme("camera-video"));
    
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Анализатор качества видеопотока с IP камер по протоколу RTSP.\n"
        "Метрики: Шум, Контраст, Резкость, Пересвет.\n"
        "Итоговая оценка: 0-100 (100 = отличное качество)"
    );
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption cameraOption(
        QStringList() << "c" << "camera",
        "Add camera with specified RTSP URL",
        "rtsp_url"
    );
    parser.addOption(cameraOption);
    
    QCommandLineOption debugOption(
        QStringList() << "d" << "debug",
        "Enable debug mode with verbose output"
    );
    parser.addOption(debugOption);
    
    parser.process(app);
    
    bool debugMode = parser.isSet(debugOption);
    if (debugMode) {
        qDebug() << "Debug mode enabled";
    }
    
    MainWindow mainWindow;
    mainWindow.show();
    
    if (parser.isSet(cameraOption)) {
        QString rtspUrl = parser.value(cameraOption);
        if (debugMode) {
            qDebug() << "Adding camera from command line:" << rtspUrl;
        }
        QTimer::singleShot(100, [ &mainWindow, rtspUrl ]() {
            mainWindow.setRtspInput(rtspUrl);
        });
    }
    
    int result = app.exec();
    
    return result;
}
