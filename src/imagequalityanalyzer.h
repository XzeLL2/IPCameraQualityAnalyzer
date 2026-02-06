#ifndef IMAGEQUALITYANALYZER_H
#define IMAGEQUALITYANALYZER_H

#include <opencv2/opencv.hpp>
#include <QObject>
#include <QImage>
#include <QDebug>

/**
 * @class ImageQualityAnalyzer
 * @brief Класс для анализа качества изображения с видеокамеры
 * 
 * Данный класс выполняет комплексный анализ качества видеокадра по следующим параметрам:
 * 1. Шумность (Noise) - измеряется через отклонение от размытого изображения
 * 2. Контрастность (Contrast) - измеряется через разницу между max и min яркостью
 * 3. Резкость (Sharpness) - измеряется через анализ градиентов и краев
 * 4. Пересвеченные пиксели (Overexposed) - процент пикселей выше порога яркости
 * 
 * Итоговая оценка вычисляется как взвешенная сумма всех параметров (0-100 баллов)
 */
class ImageQualityAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit ImageQualityAnalyzer(QObject *parent = nullptr);
    ~ImageQualityAnalyzer();

    /**
     * @brief Структура для хранения результатов анализа качества изображения
     */
    struct QualityResult {
        double noiseScore;           // Оценка шумности (0-100), выше = лучше
        double contrastScore;        // Оценка контрастности (0-100), выше = лучше
        double sharpnessScore;       // Оценка резкости (0-100), выше = лучше
        double overexposedPercent;   // Процент пересвеченных пикселей (0-100), ниже = лучше
        double overallScore;         // Общая оценка качества (0-100), выше = лучше
        QString status;              // Текстовое описание статуса
        bool isValid;                // Флаг валидности результата
        
        QualityResult() : noiseScore(0), contrastScore(0), sharpnessScore(0),
                          overexposedPercent(0), overallScore(0), status(""), isValid(false) {}
    };

    /**
     * @brief Анализирует качество изображения и возвращает оценку
     * @param frame Кадр изображения в формате OpenCV (cv::Mat)
     * @return QualityResult Результат анализа качества
     */
    QualityResult analyze(const cv::Mat& frame);

    /**
     * @brief Конвертирует cv::Mat в QImage для отображения в GUI
     * @param mat Исходное изображение OpenCV
     * @return QImage Изображение для Qt
     */
    static QImage matToQImage(const cv::Mat& mat);

signals:
    void analysisCompleted(const QualityResult& result);

private:
    /**
     * @brief Вычисляет оценку шумности изображения
     */
    double calculateNoiseScore(const cv::Mat& frame);

    /**
     * @brief Вычисляет оценку контрастности изображения
     */
    double calculateContrastScore(const cv::Mat& frame);

    /**
     * @brief Вычисляет оценку резкости изображения
     */
    double calculateSharpnessScore(const cv::Mat& frame);

    /**
     * @brief Вычисляет процент пересвеченных пикселей
     */
    double calculateOverexposedPercentage(const cv::Mat& frame);

    // Константы для весовых коэффициентов
    const double NOISE_WEIGHT = 0.25;
    const double CONTRAST_WEIGHT = 0.25;
    const double SHARPNESS_WEIGHT = 0.35;
    const double OVEREXPOSED_WEIGHT = 0.15;

    // Пороговые значения
    const int OVEREXPOSED_THRESHOLD = 245;
    const double IDEAL_CONTRAST = 160.0;
    const double IDEAL_SHARPNESS = 400.0;
    const double MAX_NOISE_VARIANCE = 50.0;
};

#endif // IMAGEQUALITYANALYZER_H
