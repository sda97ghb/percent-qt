#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) :
    QWidget(parent, Qt::Dialog),
    _isFileOpen(false)
{
    setWindowTitle("Binarization");
    setMinimumSize(220, 50);

    _toolbar = new QWidget(this);
    _toolbar->resize(width(), 25);

    QPushButton* openFileButton = new QPushButton("&Open", _toolbar);
    openFileButton->move(2, 2);
    connect(openFileButton, SIGNAL(clicked(bool)), SLOT(openfile()));

//    _gaussian = new QCheckBox("&Gaussian", this);
//    _gaussian->setChecked(true);
//    _gaussian->move(width() - _gaussian->width(), 3);
//    connect(_gaussian, SIGNAL(toggled(bool)), SLOT(binarize()));

    _cSlider = new QSlider(Qt::Horizontal, this);
    _cSlider->setRange(THRESH_RANGE_MIN, THRESH_RANGE_MAX);
    _cSlider->setValue(C_SLIDER_START_VALUE);
    connect(_cSlider, SIGNAL(valueChanged(int)), SLOT(binarize()));

//    _blockSize = new QSpinBox(_toolbar);
//    _blockSize->setRange(BLOCK_SIZE_MIN, BLOCK_SIZE_MAX);
//    _blockSize->setSingleStep(2);
//    _blockSize->setValue(BLOCK_SIZE_START_VALUE);
//    _blockSize->move(_gaussian->x() width() - _blockSize->width(), 2);
//    connect(_blockSize, SIGNAL(valueChanged(int)), SLOT(binarize()));

    _image = new QWidget(this);

    _percentInfo = new QLabel("Object is %% from the image.", this);

    openfile();
}

void
MainWindow::displayMatrix(const cv::Mat& matrix, bool isGray)
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
    int height = _toolbar->height() + _cSlider->height() +
            image.height() + _percentInfo->height();
    _image->resize(image.width(), image.height());
    resize(image.width(), height);
}

void
MainWindow::resizeEvent(QResizeEvent*)
{
    _cSlider->resize(width(), _cSlider->height());
    _toolbar->resize(width(), _toolbar->height());
    _percentInfo->resize(width(), _percentInfo->height());

    _toolbar->move(0, 0);
//    _gaussian->move(width() - _gaussian->width() - 2, 3);
//    _blockSize->move(_gaussian->x() - _blockSize->width() - 4, 2);
    _cSlider->move(0, _toolbar->height());
    _image->move(0, _toolbar->height() + _cSlider->height());
    _percentInfo->move(0, _toolbar->height() + _cSlider->height() + _image->height());
}

void
MainWindow::openfile()
{
    while (true) {
        QString filename = QFileDialog::getOpenFileName(this,
                                                      "Select an image file",
                                                      "/home/dmitry/percent/");
        if (filename.isEmpty()) {
            if (_isFileOpen)
                return;
            else
                exit(EXIT_FAILURE);
        }

        try {
            /// Load an image
            _src = cv::imread(filename.toStdString());
            _isFileOpen = true;

            resizeImageToFit();

            /// Convert the image to Gray
            cvtColor(_src, _src_gray, CV_BGR2GRAY );

            fixLightness();
        }
        catch (cv::Exception&) {
            QMessageBox::critical(this, "Error", "File is not an image.");
            continue;
        }
        break;
    }

    binarize();
}

void
MainWindow::binarize()
{
//    auto adaptiveMethod = _gaussian->isChecked() ?
//                cv::ADAPTIVE_THRESH_GAUSSIAN_C : cv::ADAPTIVE_THRESH_MEAN_C;

    threshold(_src_gray, _dst, _cSlider->value(), 255, cv::THRESH_BINARY);
//    cv::adaptiveThreshold(_src_gray, _dst, 255,
//                          adaptiveMethod,
//                          cv::THRESH_BINARY,
//                          _blockSize->value(), _cSlider->value());

    deleteTooSmallClusters();

    displayMatrix(_dst, true);

    calculatePercent();
}

void
MainWindow::resizeImageToFit()
{
    double height = _src.size().height;
    double width = _src.size().width;

    if (width <= MAX_IMAGE_WIDTH && height <= MAX_IMAGE_HEIGHT)
        return;

    auto max = [] (double x, double y) { return x > y ? x : y; };
    double scaleFactor = 1.0 / max(width  / MAX_IMAGE_WIDTH,
                                   height / MAX_IMAGE_HEIGHT);
    cv::resize(_src, _src, cv::Size(), scaleFactor, scaleFactor,
               cv::INTER_CUBIC);
}

void
MainWindow::fixLightness()
{
    int height = _src_gray.size().height;
    int width = _src_gray.size().width;

    cv::Mat m = cv::Mat(height, width, CV_8U);

    // We have grayscale image _src_gray.
    // Calculate lightness for each pixel as average of
    // its neighborhood of radius r.
    for (int i = RADIUS; i < height - RADIUS; ++ i)
        for (int j = RADIUS; j < width - RADIUS; ++ j) {
            int s = 0;
            for (int k = -RADIUS; k <= RADIUS; ++ k)
                for (int l = -RADIUS; l <= RADIUS; ++ l)
                    s += _src_gray.at<uchar>(i + k, j + l);
            s /= (2 * RADIUS + 1) * (2 * RADIUS + 1);
            m.at<uchar>(i, j) = s;
        }

    int avg = 0; // average lightness for whole image
    for (int i = RADIUS; i < height - RADIUS; ++ i)
        for (int j = RADIUS; j < width - RADIUS; ++ j)
            avg += m.at<uchar>(i, j);
    avg /= ((width - 2 * RADIUS) * (height - 2 * RADIUS));

    // Fixed pixel value is original pixel value minus difference of
    // average lightness and pixel's lightness.
    for (int i = RADIUS; i < height - RADIUS; ++ i)
        for (int j = RADIUS; j < width - RADIUS; ++ j) {
            int v = _src_gray.at<uchar>(i, j) + avg - m.at<uchar>(i, j);
            v = v < 0 ? 0 : v;
            v = v > 255 ? 255 : v;
            _src_gray.at<uchar>(i, j) = v;
        }
}

void
MainWindow::deleteTooSmallClusters()
{
    int height = _dst.size().height;
    int width = _dst.size().width;
    for (int i = RADIUS; i < height - RADIUS; ++ i)
        for (int j = RADIUS; j < width - RADIUS; ++ j)
            if (_dst.at<uchar>(i, j) == 0) {
                _cluster.clear();
                getCluster(i, j);
                const int CRITICAL_CLUSTER_SIZE = 50;
                if (_cluster.size() < CRITICAL_CLUSTER_SIZE)
                    deleteCluster();
            }
}

void
MainWindow::getCluster(int i, int j)
{
    cv::Point this_point = cv::Point(i, j);
    bool isIJInCluster = std::find(std::begin(_cluster), std::end(_cluster),
                                   this_point) != std::end(_cluster);
    if (isIJInCluster)
        return;

    _cluster.push_back(this_point);

    for (int k = -1; k <= 1; ++ k)
        for (int l = -1; l <= 1; ++ l) {
            if (k == 0 && l == 0)
                continue;
            int newi = i + k;
            int newj = j + l;
            if (newi < RADIUS || newj < RADIUS ||
                newi >= _dst.size().height - RADIUS ||
                newj >= _dst.size().width - RADIUS)
                continue;
            if (_dst.at<uchar>(newi, newj) != 0)
                continue;
            getCluster(newi, newj);
        }
}

void
MainWindow::deleteCluster()
{
    for (cv::Point p : _cluster)
        _dst.at<uchar>(p.x, p.y) = 255;
    _cluster.clear();
}

void
MainWindow::calculatePercent()
{
    float whitePixelsCount = 0.0f;
    float blackPixelsCount = 0.0f;
    for (int i = RADIUS; i < _dst.rows - RADIUS; ++ i)
        for (int j = RADIUS; j < _dst.cols - RADIUS; ++ j) {
            uchar intensity = _dst.at<uchar>(i, j);
            if (intensity == static_cast<uchar>(0))
                blackPixelsCount += 1.0;
            else if (intensity == static_cast<uchar>(255))
                whitePixelsCount += 1.0;
        }
    float percent = whitePixelsCount > blackPixelsCount ?
                    blackPixelsCount : whitePixelsCount;
    percent = 100.0f * percent / (whitePixelsCount + blackPixelsCount);
    _percentInfo->setText(QString("Object is %1% from the image.")
                          .arg(percent));
}
