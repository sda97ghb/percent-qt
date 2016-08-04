#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>

#include <QCheckBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRgb>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private:
    const int THRESH_RANGE_MIN = -100;
    const int THRESH_RANGE_MAX =  100;

    cv::Mat _src, _src_gray, _dst;

    QWidget *_toolbar;
    QCheckBox *_gaussian;
    QSlider *_cSlider;
    QSpinBox *_blockSize;
    QWidget *_image;
    QLabel *_percentInfo;

    void calculatePercent();

protected:
    void resizeEvent(QResizeEvent *);
    void displayMatrix(const cv::Mat &matrix, bool isGray = false);

public slots:
    void openfile();

private slots:
    void binarize();

};

#endif // MAINWINDOW_H
