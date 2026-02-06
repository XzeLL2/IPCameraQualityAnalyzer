# Установка на Debian 12

## Быстрая установка (автоматический скрипт)

```bash
# Скачайте скрипт установки
chmod +x install_debian.sh
./install_debian.sh
```

## Ручная установка

### 1. Установка Qt5

```bash
# Обновление пакетов
sudo apt update

# Установка Qt5 и инструментов сборки
sudo apt install -y qt5-default build-essential cmake pkg-config
```

### 2. Установка OpenCV

```bash
# Установка OpenCV из репозитория
sudo apt install -y libopencv-dev libopencv-highgui-dev libopencv-videoio-dev
```

### 3. Проверка установки

```bash
# Проверка Qt5
qmake --version

# Проверка OpenCV
pkg-config --modversion opencv4
```

## Сборка проекта

```bash
# Перейдите в директорию проекта
cd ~/ip_camera_quality

# Создайте директорию сборки
mkdir build && cd build

# Настройте проект
cmake ..

# Соберите проект
make -j$(nproc)

# Запустите
./IPCameraQualityAnalyzer
```

## Возможные проблемы

### Qt5 не найден

Если Qt5 не устанавливается через `apt`:
```bash
# Попробуйте установить компоненты отдельно
sudo apt install -y qtbase5-dev qtchooser
```

### OpenCV модули не найдены

```bash
# Проверьте установленные пакеты
dpkg -l | grep opencv

# Установите необходимые пакеты
sudo apt install -y libopencv-core-dev libopencv-imgproc-dev libopencv-video-dev libopencv-videoio-dev
```

### Ошибка CMake "Could not find Qt5"

Укажите путь к Qt5 вручную:
```bash
cmake .. -DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5
```

## Запуск с RTSP камеры

```bash
# Пример запуска
./IPCameraQualityAnalyzer --camera rtsp://admin:password@192.168.1.100:554/stream
```

## Создание .desktop файла (опционально)

```ini
[Desktop Entry]
Type=Application
Name=IP Camera Quality Analyzer
Comment=Analyze video quality from IP cameras
Exec=/home/user/ip_camera_quality/build/IPCameraQualityAnalyzer
Icon=camera-video
Terminal=false
Categories=Qt;AudioVideo;Video;
```
