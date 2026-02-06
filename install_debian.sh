#!/bin/bash

# Скрипт установки зависимостей для Debian 12
# и сборки проекта IP Camera Quality Analyzer

set -e

echo "=========================================="
echo "Установка зависимостей для Debian 12"
echo "=========================================="

# Обновление пакетов
echo "[1/4] Обновление пакетов..."
sudo apt update

# Установка Qt5
echo "[2/4] Установка Qt5 и зависимостей..."
sudo apt install -y \
    qtbase5-dev \
    build-essential \
    cmake \
    pkg-config \
    qtbase5-dev-tools

# Установка OpenCV
echo "[3/4] Установка OpenCV..."
sudo apt install -y \
    libopencv-dev \
    libopencv-highgui-dev \
    libopencv-videoio-dev \
    libopencv-core-dev

# Проверка установки
echo "[4/4] Проверка установки..."
echo "Qt5 version:"
qmake --version

echo ""
echo "OpenCV version:"
pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv

echo ""
echo "=========================================="
echo "Установка завершена!"
echo "=========================================="
echo ""
echo "Для сборки проекта выполните:"
echo "  cd ~/ip_camera_quality"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
echo ""
echo "Для запуска:"
echo "  ./IPCameraQualityAnalyzer"
echo ""
echo "Или с RTSP URL:"
echo "  ./IPCameraQualityAnalyzer --camera rtsp://ваш_ip:554/stream"
