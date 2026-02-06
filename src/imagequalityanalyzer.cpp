#include "imagequalityanalyzer.h"
#include <QDebug>
#include <cmath>

ImageQualityAnalyzer::ImageQualityAnalyzer(QObject *parent)
    : QObject(parent)
{
}

ImageQualityAnalyzer::~ImageQualityAnalyzer()
{
}

ImageQualityAnalyzer::QualityResult ImageQualityAnalyzer::analyze(const cv::Mat& frame)
{
    QualityResult result;
    result.isValid = false;

    if (frame.empty() || frame.cols < 10 || frame.rows < 10) {
        result.status = "Неверный кадр";
        result.overallScore = 0.0;
        return result;
    }

    try {
        cv::Mat grayFrame;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        } else {
            grayFrame = frame.clone();
        }

        double noiseScore = calculateNoiseScore(grayFrame);
        double contrastScore = calculateContrastScore(grayFrame);
        double sharpnessScore = calculateSharpnessScore(grayFrame);
        double overexposedPercent = calculateOverexposedPercentage(grayFrame);

        double overallScore = 
            noiseScore * NOISE_WEIGHT +
            contrastScore * CONTRAST_WEIGHT +
            sharpnessScore * SHARPNESS_WEIGHT +
            (100.0 - overexposedPercent) * OVEREXPOSED_WEIGHT;

        if (overallScore > 100.0) overallScore = 100.0;
        if (overallScore < 0.0) overallScore = 0.0;

        result.noiseScore = noiseScore;
        result.contrastScore = contrastScore;
        result.sharpnessScore = sharpnessScore;
        result.overexposedPercent = overexposedPercent;
        result.overallScore = overallScore;
        result.isValid = true;

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

double ImageQualityAnalyzer::calculateNoiseScore(const cv::Mat& frame)
{
    // Отладочная информация для диагностики
    qDebug() << "[Noise] Frame channels:" << frame.channels();
    qDebug() << "[Noise] Frame size:" << frame.cols << "x" << frame.rows;
    qDebug() << "[Noise] Frame type:" << frame.type();
    
    cv::Mat blurred;
    cv::GaussianBlur(frame, blurred, cv::Size(5, 5), 0);
    
    cv::Mat diff;
    cv::absdiff(frame, blurred, diff);
    
    cv::Scalar meanDiff = cv::mean(diff);
    double noiseLevel = meanDiff[0];
    
    qDebug() << "[Noise] Raw noise level:" << noiseLevel;
    qDebug() << "[Noise] MAX_NOISE_VARIANCE:" << MAX_NOISE_VARIANCE;
    
    double noiseScore = 100.0 - (noiseLevel / MAX_NOISE_VARIANCE) * 100.0;
    if (noiseScore < 0.0) noiseScore = 0.0;
    if (noiseScore > 100.0) noiseScore = 100.0;
    
    qDebug() << "[Noise] Calculated score:" << noiseScore;
    
    return noiseScore;
}

double ImageQualityAnalyzer::calculateContrastScore(const cv::Mat& frame)
{
    double minVal, maxVal;
    cv::minMaxLoc(frame, &minVal, &maxVal);
    
    double contrastRange = maxVal - minVal;
    double contrastScore = (contrastRange / IDEAL_CONTRAST) * 100.0;
    
    if (contrastScore > 100.0) contrastScore = 100.0;
    if (contrastScore < 0.0) contrastScore = 0.0;
    
    if (contrastRange > 250) {
        contrastScore = contrastScore * 0.8;
    }
    
    return contrastScore;
}

double ImageQualityAnalyzer::calculateSharpnessScore(const cv::Mat& frame)
{
    cv::Mat laplacian;
    cv::Laplacian(frame, laplacian, CV_64F, 3);
    
    cv::Scalar mean, stdDev;
    cv::meanStdDev(laplacian, mean, stdDev);
    
    double laplacianVariance = stdDev[0] * stdDev[0];
    double sharpnessScore = (laplacianVariance / IDEAL_SHARPNESS) * 100.0;
    
    if (sharpnessScore > 100.0) sharpnessScore = 100.0;
    if (sharpnessScore < 0.0) sharpnessScore = 0.0;
    
    return sharpnessScore;
}

double ImageQualityAnalyzer::calculateOverexposedPercentage(const cv::Mat& frame)
{
    cv::Mat overexposedMask;
    cv::threshold(frame, overexposedMask, OVEREXPOSED_THRESHOLD, 255, cv::THRESH_BINARY);
    
    int overexposedCount = cv::countNonZero(overexposedMask);
    double totalPixels = frame.rows * frame.cols;
    double overexposedPercent = (overexposedCount / totalPixels) * 100.0;
    
    if (overexposedPercent > 100.0) overexposedPercent = 100.0;
    if (overexposedPercent < 0.0) overexposedPercent = 0.0;
    
    return overexposedPercent;
}

QImage ImageQualityAnalyzer::matToQImage(const cv::Mat& mat)
{
    try {
        cv::Mat rgbMat;
        QImage qImage;
        
        if (mat.channels() == 3) {
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
            qImage = QImage(
                rgbMat.data,
                rgbMat.cols,
                rgbMat.rows,
                static_cast<int>(rgbMat.step),
                QImage::Format_RGB888
            ).copy();
        } else if (mat.channels() == 1) {
            qImage = QImage(
                mat.data,
                mat.cols,
                mat.rows,
                static_cast<int>(mat.step),
                QImage::Format_Grayscale8
            ).copy();
        } else {
            qImage = QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_ARGB32).copy();
        }
        
        return qImage;
    } catch (...) {
        qWarning() << "Exception in matToQImage";
        return QImage();
    }
}
