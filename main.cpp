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
#include <QDirIterator>
#include <QLabel>
#include <QMovie>

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
        btext=false;
        view->setBackgroundBrush(Qt::black);  // or any QColor

        for (int i = 0; i < 6; ++i) {
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
                tickSlideshow();
             break;
            case Qt::Key_Down:
                tickSlideshow();
               break;
            case Qt::Key_Space:
                tickSlideshow();
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
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
         //   qDebug() << "Left mouse button pressed";
             tickSlideshow();
        } else if (event->button() == Qt::RightButton) {
         //   qDebug() << "Right mouse button pressed";
             tickSlideshow();
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
            if (slideshowMode == SixPane)
                loadSixPane();
            else if (slideshowMode == FourPane)
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

    void loadImagesFromFolder(const QString& folderPath, const QString& startImage = QString()) {
       // QDir dir(folderPath);
        QStringList filters = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif" };

        images.clear();
        QDir dir(folderPath);
    QDirIterator it(folderPath, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString relative = dir.relativeFilePath(filePath);  // Strips base path
            images << relative;  // e.g., just "cat.jpg"
        }

        this->folderPath = folderPath;

        // Optionally set currentIndex based on the start image
        if (!startImage.isEmpty()) {
            currentIndex = images.indexOf(startImage);
        } else {
            currentIndex = 0;
        }

        loadImage(currentIndex);
    }

    void dropEvent(QDropEvent *event) override {
        QList<QUrl> urls = event->mimeData()->urls();
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            if (urls.isEmpty()) return;

            QString path = urls.first().toLocalFile();
            QFileInfo info(path);

            if (info.isDir()) {
                loadImagesFromFolder(path);
            } else if (info.isFile()) {
                loadImagesFromFolder(info.absolutePath(), info.fileName());
            }
            event->acceptProposedAction();
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
    void startSlideshowSix() {
        slideshowMode = SixPane;
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
        if (slideshowMode == SixPane)
            loadSixPane();
        else if (slideshowMode == FourPane)
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
    bool btext;
    QList<QLabel*> gifLabels;

    enum Mode { Single, FourPane, SixPane };

    Mode slideshowMode;

    void setupMenu() {
        QMenu *fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open Image...", this, &ImageViewer::openImage);

        QMenu *slideshowMenu = menuBar()->addMenu("Slideshow");
        slideshowMenu->addAction("Start Slideshow (Single)", this, &ImageViewer::startSlideshowSingle);
        slideshowMenu->addAction("Start Slideshow (4-Pane)", this, &ImageViewer::startSlideshowFour);
        slideshowMenu->addAction("Start Slideshow (6-Pane)", this, &ImageViewer::startSlideshowSix);
        slideshowMenu->addAction("Stop Slideshow", this, &ImageViewer::stopSlideshow);

        QMenu *viewMenu = menuBar()->addMenu("View");
        viewMenu->addAction("Toggle Fullscreen (F)", this, &ImageViewer::toggleFullscreen);
        viewMenu->addAction("text (F5)", this, &ImageViewer::toggletext);
    }

    void toggletext() {
        btext=!btext;
    }

    void loadImagesFromFile(const QString &imagePath) {
        if (!QFileInfo(imagePath).exists()) return;

        QDir dir = QFileInfo(imagePath).absoluteDir();
        QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
        images = dir.entryList(filters, QDir::Files, QDir::Name);
        folderPath = dir.absolutePath();
        currentIndex = images.indexOf(QFileInfo(imagePath).fileName());

        QTimer::singleShot(50, [this]() {
             if (slideshowMode == SixPane)
                 loadSixPane();
            else if (slideshowMode == FourPane)
                loadFourPane();
            else
                loadImage(currentIndex);
        });
    }

    bool isGif(const QString& filePath) {
        return filePath.endsWith(".gif", Qt::CaseInsensitive);
    }

    void loadImage(int index) {
        if (index < 0 || index >= images.size()) return;
        QString imagePath = folderPath + "/" + images[index];
        QPixmap pix(imagePath);
        if (pix.isNull()) return;

        QSize scaledSize = view->viewport()->size();
      //  pix = pix.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Show only the first pixmap item
        for (int i = 0; i < pixmapItems.size(); ++i) {
            pixmapItems[i]->setVisible(i == 0);
        }

        QPixmap scaled = pix.scaled(scaledSize.width(), scaledSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Create a new pixmap to paint image + text shadow
        QPixmap composed(scaled.size());
        composed.fill(Qt::transparent);

        QPainter painter(&composed);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.drawPixmap(0, 0, scaled);
     //   if (0) {
        if (isGif(imagePath)) {
            QMovie* movie = new QMovie(imagePath);
            if (!movie->isValid()) {
                delete movie;
             //   continue;
            }

            movie->setScaledSize(scaledSize);

            QLabel* label = new QLabel;
            label->setMovie(movie);
            label->setGeometry(scaledSize.width(), scaledSize.height(), scaledSize.width(), scaledSize.height());
            label->setParent(view);  // or layout
            label->show();
            label->setVisible(true);
            movie->start();

            gifLabels.append(label);  // manage them for cleanup
        } else {
        // Draw the scaled image
        if(btext){
                 // Optionally draw translucent black rect behind text for readability
                 QRect textRect(0, composed.height() - 30, composed.width(), 30);
                 painter.setBrush(QColor(0, 0, 0, 100)); // translucent black
                 painter.setPen(Qt::NoPen);
                 painter.drawRect(textRect);

                 // Prepare font and shadow
                 QString fileName = QFileInfo(imagePath).fileName();

                 QFont font = painter.font();
                 font.setBold(true);
                 font.setPointSize(14);
                 painter.setFont(font);

                 QPoint shadowOffset(2, 2);

                 // Draw shadow text
                 painter.setPen(QColor(0, 0, 0, 160));
                 painter.drawText(textRect.translated(shadowOffset), Qt::AlignCenter | Qt::AlignVCenter, fileName);

                 // Draw main text
                 painter.setPen(Qt::white);
                 painter.drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter, fileName);

                 painter.end();
      }
        pixmapItems[0]->setPixmap(composed);
        pixmapItems[0]->setPos(0, 0);
        scene->setSceneRect(pixmapItems[0]->boundingRect());
}

        animations[0]->stop();
        opacityEffects[0]->setOpacity(0.0);
        animations[0]->start();
    }

    void loadSixPane() {
        slideshowMode = SixPane;
        if (images.size() < 6) return;

        QSet<int> indexes;
        while (indexes.size() < 6)
            indexes.insert(QRandomGenerator::global()->bounded(images.size()));

        int i = 0;
        QSize viewportSize = view->viewport()->size();
        int w = viewportSize.width() / 3;
        int h = viewportSize.height() / 2;

        for (int index : indexes) {
            QString path = folderPath + "/" + images[index];
            QPixmap pix(path);
            if (pix.isNull()) continue;

            QPixmap scaled = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Create a new pixmap to paint image + text shadow
            QPixmap composed(scaled.size());
            composed.fill(Qt::transparent);

            QPainter painter(&composed);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);

            // Draw the scaled image
            painter.drawPixmap(0, 0, scaled);
   if(btext){
            // Optionally draw translucent black rect behind text for readability
            QRect textRect(0, composed.height() - 30, composed.width(), 30);
            painter.setBrush(QColor(0, 0, 0, 100)); // translucent black
            painter.setPen(Qt::NoPen);
            painter.drawRect(textRect);

            // Prepare font and shadow
            QString fileName = QFileInfo(path).fileName();

            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(14);
            painter.setFont(font);

            QPoint shadowOffset(2, 2);

            // Draw shadow text
            painter.setPen(QColor(0, 0, 0, 160));
            painter.drawText(textRect.translated(shadowOffset), Qt::AlignCenter | Qt::AlignVCenter, fileName);

            // Draw main text
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter, fileName);

            painter.end();
 }
            int row = i / 3;
            int col = i % 3;

            pixmapItems[i]->setPixmap(composed);
            pixmapItems[i]->setPos(col * w, row * h);
            pixmapItems[i]->setVisible(true);

            animations[i]->stop();
            opacityEffects[i]->setOpacity(0.0);
            animations[i]->start();
            ++i;
        }

        for (; i < pixmapItems.size(); ++i) {
            pixmapItems[i]->setVisible(false);
        }

        scene->setSceneRect(0, 0, viewportSize.width(), viewportSize.height());
    }


    void loadFourPane() {
               slideshowMode = FourPane;
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

            // --- Create a new pixmap with the same size to paint on ---
            QPixmap composed(scaled.size());
            composed.fill(Qt::transparent); // transparent background

            QPainter painter(&composed);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);

            // Draw the scaled image first
            painter.drawPixmap(0, 0, scaled);
if(btext){
            // Prepare the drop shadow text
            QString fileName = QFileInfo(path).fileName();

            // Draw shadow
            painter.setPen(QPen(QColor(0, 0, 0, 160)));  // semi-transparent black shadow
            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(14);
            painter.setFont(font);

            int textMargin = 8;
            QPoint shadowOffset(2, 2);

            QRect textRect(0, composed.height() - 30, composed.width(), 30);

            // Draw shadow text slightly offset
            painter.drawText(textRect.translated(shadowOffset), Qt::AlignCenter | Qt::AlignVCenter, fileName);

            // Draw main white text
            painter.setPen(QPen(Qt::white));
            painter.drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter, fileName);

            painter.end();
}
            int row = i / 2;
            int col = i % 2;

            pixmapItems[i]->setPixmap(composed);
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
