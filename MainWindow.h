#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>

#include <QCheckBox>
#include <QFileDialog>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QRgb>
#include <QSlider>
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
    cv::Mat _src, _src_gray, _dst;

    QWidget *_toolbar;
    QCheckBox *_inverted;
    QSlider *_slider;
    QWidget *_image;
    QLabel *_label;

    void calculatePercent();

protected:
    void resizeEvent(QResizeEvent *event);
    void displayMat(const cv::Mat &mat, bool isGray = false);

public slots:
    void openfile();

private slots:
    void binarize();

};

#endif // MAINWINDOW_H
