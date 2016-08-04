#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent, Qt::Dialog)
{
    setWindowTitle("Binarization");
    setMinimumSize(220, 50);

    _toolbar = new QWidget(this);
    _toolbar->resize(width(), 25);

    QPushButton *openFileButton = new QPushButton("&Open", _toolbar);
    openFileButton->move(2, 2);
    connect(openFileButton, SIGNAL(clicked(bool)), SLOT(openfile()));

    _gaussian = new QCheckBox("&Gaussian", this);
    _gaussian->setChecked(true);
    _gaussian->move(width() - _gaussian->width(), 3);
    connect(_gaussian, SIGNAL(toggled(bool)), SLOT(binarize()));

    _cSlider = new QSlider(Qt::Horizontal, this);
    _cSlider->setRange(THRESH_RANGE_MIN, THRESH_RANGE_MAX);
    _cSlider->setValue(0);
    connect(_cSlider, SIGNAL(valueChanged(int)), SLOT(binarize()));

    _blockSize = new QSpinBox(_toolbar);
    _blockSize->setRange(3, 99);
    _blockSize->setSingleStep(2);
    _blockSize->setValue(7);
    _blockSize->move(_gaussian->x() - _blockSize->width(), 2);
    connect(_blockSize, SIGNAL(valueChanged(int)), SLOT(binarize()));

    _image = new QWidget(this);

    _percentInfo = new QLabel("Object is %% from the image.", this);

    openfile();
}

void
MainWindow::displayMatrix(const cv::Mat &matrix, bool isGray)
{
    QImage image(matrix.cols, matrix.rows, QImage::Format_RGB32);
    for (int i = 0; i < matrix.rows; ++ i)
        for (int j = 0; j < matrix.cols; ++ j)
            if (isGray) {
                uchar intensity = matrix.at<uchar>(i, j);
                image.setPixel(j, i, qRgb(intensity, intensity, intensity));
            } else {
                cv::Vec3b intensity = matrix.at<cv::Vec3b>(i, j);
                uchar blue  = intensity.val[0];
                uchar green = intensity.val[1];
                uchar red   = intensity.val[2];
                image.setPixel(j, i, qRgb(red, green, blue));
            }
    QPalette palette = this->palette();
    palette.setBrush(backgroundRole(), QBrush(image));
    _image->setPalette(palette);
    _image->setAutoFillBackground(true);
    int height = _toolbar->height() + _cSlider->height() + image.height() + _percentInfo->height();
    _image->resize(image.width(), image.height());
    resize(image.width(), height);
}

void
MainWindow::resizeEvent(QResizeEvent *)
{
    _cSlider->resize(width(), _cSlider->height());
    _toolbar->resize(width(), _toolbar->height());
    _percentInfo->resize(width(), _percentInfo->height());

    _toolbar->move(0, 0);
    _gaussian->move(width() - _gaussian->width() - 2, 3);
    _blockSize->move(_gaussian->x() - _blockSize->width() - 4, 2);
    _cSlider->move(0, _toolbar->height());
    _image->move(0, _toolbar->height() + _cSlider->height());
    _percentInfo->move(0, _toolbar->height() + _cSlider->height() + _image->height());
}

void
MainWindow::openfile()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Select an image file",
                                                    "/home/dmitry/percent/");
    if (filename.isEmpty()) {
        QMessageBox::critical(this, "Error", "Can't open file.\n"
                                             "Application will be closed.");
        exit(EXIT_SUCCESS);
    }

    try {
        /// Load an image
        _src = cv::imread(filename.toStdString());

        /// Convert the image to Gray
        cvtColor(_src, _src_gray, CV_BGR2GRAY );
    }
    catch (cv::Exception &exception) {
        QMessageBox::critical(this, "Error", "File is not an image.\n"
                                             "Application will be closed.");
        abort();
    }

    binarize();
}

void
MainWindow::binarize()
{
    auto adaptiveMethod = _gaussian->isChecked() ?
                cv::ADAPTIVE_THRESH_GAUSSIAN_C : cv::ADAPTIVE_THRESH_MEAN_C;

//    threshold(_src_gray, _dst, _slider->value(), 255, cv::THRESH_BINARY);
    cv::adaptiveThreshold(_src_gray, _dst, 255,
                          adaptiveMethod,
                          cv::THRESH_BINARY,
                          _blockSize->value(), _cSlider->value());

    displayMatrix(_dst, true);

    calculatePercent();
}

void
MainWindow::calculatePercent()
{
    float whitePixelsCount = 0.0f;
    float blackPixelsCount = 0.0f;
    for (int i = 0; i < _dst.rows; ++ i)
        for (int j = 0; j < _dst.cols; ++ j) {
            uchar intensity = _dst.at<uchar>(i, j);
            if (intensity == static_cast<uchar>(0))
                blackPixelsCount += 1.0;
            else if (intensity == static_cast<uchar>(255))
                whitePixelsCount += 1.0;
        }
    float percent = whitePixelsCount > blackPixelsCount ?
                    blackPixelsCount : whitePixelsCount;
    percent = 100.0f * percent / (whitePixelsCount + blackPixelsCount);
    _percentInfo->setText(QString("Object is %1% from the image.").arg(percent));
}
