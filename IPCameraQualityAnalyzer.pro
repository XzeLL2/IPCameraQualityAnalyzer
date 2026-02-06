#-------------------------------------------------
#
# Проектный файл Qt для IP Camera Quality Analyzer
# Совместимость: Qt 5.x на Debian 12
#
# Использование:
#   qmake IPCameraQualityAnalyzer.pro
#   make
#
# Для отладки:
#   qmake CONFIG+=debug IPCameraQualityAnalyzer.pro
#   make
#
#-------------------------------------------------

# Имя проекта
QT += core gui widgets

# Версия Qt (для совместимости)
QT_VERSION = $$QT_VERSION

# Имя приложения и версия
TARGET = IPCameraQualityAnalyzer
TEMPLATE = app

# Версия приложения
VERSION = 1.0.0

# Определения для версии
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Формы UI (Qt Designer)
FORMS += src/mainwindow.ui

# Исходные файлы (SOURCES)
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/cameraworker.cpp \
    src/imagequalityanalyzer.cpp

# Заголовочные файлы (HEADERS)
HEADERS += \
    src/mainwindow.h \
    src/cameraworker.h \
    src/imagequalityanalyzer.h

# Ресурсы (если есть)
# RESOURCES += resources.qrc

# Пути к заголовочным файлам
INCLUDEPATH += \
    $$PWD/src

# Конфигурация OpenCV
# Попытка автоматически найти OpenCV через pkg-config
packagesExist(opencv4) {
    message("Found OpenCV 4")
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
} else:packagesExist(opencv) {
    message("Found OpenCV (legacy)")
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv
} else {
    message("OpenCV not found via pkg-config, using manual paths")
    
    # Ручные пути для OpenCV (настройте при необходимости)
    INCLUDEPATH += /usr/include/opencv4
    INCLUDEPATH += /usr/include/opencv
    LIBS += -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui
}

# Настройки компиляции
CONFIG += c++17

# Режим отладки или релиза
CONFIG(debug, debug|release) {
    message("Debug build")
    DEFINES += QT_DEBUG
} else {
    message("Release build")
}

# Оптимизация
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native
QMAKE_LFLAGS_RELEASE += -Wl,-O1

# Включение дополнительных предупреждений
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# Мета-данные приложения
QMAKE_TARGET_COMPANY = Custom Solutions
QMAKE_TARGET_PRODUCT = IP Camera Quality Analyzer
QMAKE_TARGET_DESCRIPTION = Real-time video quality analyzer for IP cameras

# Иконка приложения (опционально)
# win32:RC_ICONS += appicon.ico
# unix:!macx:ICON = appicon.png

# Установка deployment (опционально)
unix:!macx {
    target.path = /usr/local/bin
    INSTALLS += target
    
    # Создание desktop файла
    desktop.files += IPCameraQualityAnalyzer.desktop
    desktop.path = /usr/share/applications
    INSTALLS += desktop
}

# Создание .desktop файла при сборке
desktop_file.path = $$PWD
desktop_file.name = IPCameraQualityAnalyzer.desktop

# Формирование .desktop файла
QMAKE_POST_LINK += echo \"[Desktop Entry]\" > IPCameraQualityAnalyzer.desktop && \
    echo \"Type=Application\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Name=IP Camera Quality Analyzer\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Comment=Real-time video quality analyzer for IP cameras\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Exec=$$PWD/$$TARGET\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Icon=camera-video\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Terminal=false\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"Categories=Qt;AudioVideo;Video;\" >> IPCameraQualityAnalyzer.desktop && \
    echo \"StartupNotify=true\" >> IPCameraQualityAnalyzer.desktop

# Удаление .desktop файла при очистке
QMAKE_CLEAN += IPCameraQualityAnalyzer.desktop

# Описание для make help
QMAKE_HELP_MESSAGE = \
    \"IP Camera Quality Analyzer - Analyze video quality from IP cameras\" \
    \"\" \
    \"Metrics: Noise, Contrast, Sharpness, Overexposed pixels\" \
    \"Overall score: 0-100 (100 = excellent quality)\" \
    \"\" \
    \"Usage:\" \
    \"  $$TARGET\" \
    \"  $$TARGET --camera rtsp://192.168.1.1:554/stream\"
