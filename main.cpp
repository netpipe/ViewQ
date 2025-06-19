#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QMimeData>
#include <QDropEvent>
#include <QDebug>
#include <QRandomGenerator>
#include <QPixmap>

class ImageViewer : public QMainWindow {
    Q_OBJECT

public:
    ImageViewer(QWidget *parent = nullptr)
        : QMainWindow(parent), currentIndex(0), slideshowRunning(false), fullscreen(false), slideshowMode(Single) {
        setWindowTitle("Fancy Image Viewer");
        setMinimumSize(800, 600);
        setAcceptDrops(true);

        scene = new QGraphicsScene(this);
        view = new QGraphicsView(scene, this);
        view->installEventFilter(this);

        view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        view->setAlignment(Qt::AlignCenter);
        view->setAcceptDrops(false);
        setCentralWidget(view);

        for (int i = 0; i < 4; ++i) {
            QGraphicsPixmapItem *item = new QGraphicsPixmapItem();
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(800);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);

            item->setGraphicsEffect(effect);
            pixmapItems.append(item);
            opacityEffects.append(effect);
            animations.append(anim);
            scene->addItem(item);
        }

        slideshowTimer = new QTimer(this);
        connect(slideshowTimer, &QTimer::timeout, this, &ImageViewer::tickSlideshow);

        setupMenu();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_Right:
            if (slideshowMode == FourPane)
                loadFourPane();
            else
                nextImage();

             break;
            case Qt::Key_Down:
              nextImage();
               break;
            case Qt::Key_Space:
                nextImage();
                break;
            case Qt::Key_Left:
                 prevImage();
             break;
            case Qt::Key_Up:
                prevImage();
                break;
            case Qt::Key_Escape:
                if (fullscreen) toggleFullscreen();
                else close();
                break;
            case Qt::Key_F:
                toggleFullscreen();
                break;
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == view && event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            keyPressEvent(keyEvent);
            return true;  // We've handled it
        }
        return QMainWindow::eventFilter(obj, event);
    }

    void resizeEvent(QResizeEvent *event) override {
        QMainWindow::resizeEvent(event);
        if (!images.isEmpty()) {
            if (slideshowMode == FourPane)
                loadFourPane();
            else
                loadImage(currentIndex);
        }
    }

    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            qDebug() << "Dropped file:" << filePath;
            loadImagesFromFile(filePath);
        }
    }

private slots:
    void openImage() {
        QString imagePath = QFileDialog::getOpenFileName(this, "Open Image", QDir::homePath(), "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!imagePath.isEmpty()) {
            loadImagesFromFile(imagePath);
        }
    }

    void startSlideshowSingle() {
        slideshowMode = Single;
        if (!images.isEmpty()) {
            slideshowRunning = true;
            slideshowTimer->start(13000);
        }
    }

    void startSlideshowFour() {
        slideshowMode = FourPane;
        if (!images.isEmpty()) {
            slideshowRunning = true;
            slideshowTimer->start(13000);
        }
    }

    void stopSlideshow() {
        slideshowRunning = false;
        slideshowTimer->stop();
    }

    void toggleFullscreen() {
        if (fullscreen) {
            showNormal();
            fullscreen = false;
        } else {
            showFullScreen();
            fullscreen = true;
        }
    }

    void tickSlideshow() {
        if (slideshowMode == FourPane)
            loadFourPane();
        else
            nextImage();
    }

private:
    QGraphicsView *view;
    QGraphicsScene *scene;

    QVector<QGraphicsPixmapItem*> pixmapItems;
    QVector<QGraphicsOpacityEffect*> opacityEffects;
    QVector<QPropertyAnimation*> animations;

    QTimer *slideshowTimer;
    bool slideshowRunning;
    bool fullscreen;

    QStringList images;
    QString folderPath;
    int currentIndex;

    enum Mode { Single, FourPane };
    Mode slideshowMode;

    void setupMenu() {
        QMenu *fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open Image...", this, &ImageViewer::openImage);

        QMenu *slideshowMenu = menuBar()->addMenu("Slideshow");
        slideshowMenu->addAction("Start Slideshow (Single)", this, &ImageViewer::startSlideshowSingle);
        slideshowMenu->addAction("Start Slideshow (4-Pane)", this, &ImageViewer::startSlideshowFour);
        slideshowMenu->addAction("Stop Slideshow", this, &ImageViewer::stopSlideshow);

        QMenu *viewMenu = menuBar()->addMenu("View");
        viewMenu->addAction("Toggle Fullscreen (F)", this, &ImageViewer::toggleFullscreen);
    }

    void loadImagesFromFile(const QString &imagePath) {
        if (!QFileInfo(imagePath).exists()) return;

        QDir dir = QFileInfo(imagePath).absoluteDir();
        QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
        images = dir.entryList(filters, QDir::Files, QDir::Name);
        folderPath = dir.absolutePath();
        currentIndex = images.indexOf(QFileInfo(imagePath).fileName());

        QTimer::singleShot(50, [this]() {
            if (slideshowMode == FourPane)
                loadFourPane();
            else
                loadImage(currentIndex);
        });
    }

    void loadImage(int index) {
        if (index < 0 || index >= images.size()) return;
        QString imagePath = folderPath + "/" + images[index];
        QPixmap pix(imagePath);
        if (pix.isNull()) return;

        QSize scaledSize = view->viewport()->size();
        pix = pix.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Show only the first pixmap item
        for (int i = 0; i < pixmapItems.size(); ++i) {
            pixmapItems[i]->setVisible(i == 0);
        }

        pixmapItems[0]->setPixmap(pix);
        pixmapItems[0]->setPos(0, 0);
        scene->setSceneRect(pixmapItems[0]->boundingRect());

        animations[0]->stop();
        opacityEffects[0]->setOpacity(0.0);
        animations[0]->start();
    }

    void loadFourPane() {
        if (images.size() < 4) return;

        QSet<int> indexes;
        while (indexes.size() < 4)
            indexes.insert(QRandomGenerator::global()->bounded(images.size()));

        int i = 0;
        QSize viewportSize = view->viewport()->size();
        int w = viewportSize.width() / 2;
        int h = viewportSize.height() / 2;

        for (int index : indexes) {
            QString path = folderPath + "/" + images[index];
            QPixmap pix(path);
            if (pix.isNull()) continue;

            QPixmap scaled = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            int row = i / 2;
            int col = i % 2;

            pixmapItems[i]->setPixmap(scaled);
            pixmapItems[i]->setPos(col * w, row * h);
            pixmapItems[i]->setVisible(true);

            animations[i]->stop();
            opacityEffects[i]->setOpacity(0.0);
            animations[i]->start();
            ++i;
        }

        // Hide any unused items (safety)
        for (; i < pixmapItems.size(); ++i) {
            pixmapItems[i]->setVisible(false);
        }

        scene->setSceneRect(0, 0, viewportSize.width(), viewportSize.height());
    }

    void nextImage() {
        if (images.isEmpty()) return;
        currentIndex = (currentIndex + 1) % images.size();
        loadImage(currentIndex);
    }

    void prevImage() {
        if (images.isEmpty()) return;
        currentIndex = (currentIndex - 1 + images.size()) % images.size();
        loadImage(currentIndex);
    }
};




int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ImageViewer viewer;
    viewer.show();
    return app.exec();
}
#include "main.moc"
