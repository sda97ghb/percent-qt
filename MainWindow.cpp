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

    _inverted = new QCheckBox("Inverted", this);
    _inverted->move(width() - _inverted->width(), 3);
    connect(_inverted, SIGNAL(toggled(bool)), SLOT(binarize()));

    _slider = new QSlider(Qt::Horizontal, this);
    _slider->setRange(0, 255);
    _slider->setValue(230);
    connect(_slider, SIGNAL(valueChanged(int)), SLOT(binarize()));

    _image = new QWidget(this);

    _label = new QLabel("Object is %% from the image.", this);

    openfile();
}

void
MainWindow::displayMat(const cv::Mat &mat, bool isGray)
{
    QImage image(mat.cols, mat.rows, QImage::Format_RGB32);
    for (int i = 0; i < mat.rows; ++ i)
        for (int j = 0; j < mat.cols; ++ j)
            if (isGray) {
                uchar intensity = mat.at<uchar>(i, j);
                image.setPixel(j, i, qRgb(intensity, intensity, intensity));
            } else {
                cv::Vec3b intensity = mat.at<cv::Vec3b>(i, j);
                uchar blue  = intensity.val[0];
                uchar green = intensity.val[1];
                uchar red   = intensity.val[2];
                image.setPixel(j, i, qRgb(red, green, blue));
            }
    QPalette palette = this->palette();
    palette.setBrush(backgroundRole(), QBrush(image));
    _image->setPalette(palette);
    _image->setAutoFillBackground(true);
    int height = _toolbar->height() + _slider->height() + image.height() + _label->height();
    _image->resize(image.width(), image.height());
    resize(image.width(), height);
}

void
MainWindow::binarize()
{
    int threshold_value = _slider->value();
    if (threshold_value < 0 || threshold_value > 255)
        return;
    int threshold_type = _inverted->checkState() == Qt::Checked ? 1 : 0; // 0: Binary, 1: Binary Inverted
    int const max_BINARY_value = 255;

    threshold(_src_gray, _dst, threshold_value, max_BINARY_value, threshold_type);

    displayMat(_dst, true);

    calculatePercent();
}

void
MainWindow::resizeEvent(QResizeEvent *event)
{
    _slider->resize(width(), _slider->height());
    _toolbar->resize(width(), _toolbar->height());
    _label->resize(width(), _label->height());

    _toolbar->move(0, 0);
    _inverted->move(width() - _inverted->width() - 2, 3);
    _slider->move(0, _toolbar->height());
    _image->move(0, _toolbar->height() + _slider->height());
    _label->move(0, _toolbar->height() + _slider->height() + _image->height());
}

void
MainWindow::openfile()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Select an image file",
                                                    "/home/dmitry/percent/");
    if (filename.isEmpty()) {
        exit(EXIT_SUCCESS);
    }

    /// Load an image
    _src = cv::imread(filename.toStdString());

    /// Convert the image to Gray
    cvtColor(_src, _src_gray, CV_BGR2GRAY );

    binarize();
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
    _label->setText(QString("Object is %1% from the image.")
                    .arg(percent));
}
