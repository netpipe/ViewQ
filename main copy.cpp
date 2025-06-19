#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsOpacityEffect>
#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QFileDialog>
#include <QMenuBar>
#include <QTimer>
#include <QDir>
#include <QDropEvent>
#include <QMimeData>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QSet>
#include <QDebug>

// Custom view to handle drag-and-drop directly
class DropGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    using QGraphicsView::QGraphicsView;

signals:
    void imageDropped(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent* event) override {
        if (event->mimeData()->hasUrls()) {
            const QList<QUrl> urls = event->mimeData()->urls();
            if (!urls.isEmpty()) {
                QString path = urls.first().toLocalFile();
                qDebug() << "Dropped file:" << path;
                emit imageDropped(path);
            }
        }
        event->acceptProposedAction(); // <-- THIS IS CRUCIAL!
    }

};


class ClickablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ClickablePixmapItem(int index)
        : QGraphicsPixmapItem(), imageIndex(index), textItem(nullptr) {
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptDrops(false);
    }

    int imageIndex;
    QString fileName;
    QGraphicsTextItem* textItem;

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        emit clicked(imageIndex);
        QGraphicsPixmapItem::mousePressEvent(event);
    }
};

class ImageViewer : public QMainWindow {
    Q_OBJECT

public:
    ImageViewer() {
        setWindowTitle("Qt Image Viewer");

        view = new DropGraphicsView(this);
       // view->setAcceptDrops(true);
        view->setRenderHint(QPainter::Antialiasing);
        view->setAlignment(Qt::AlignCenter);
        view->setBackgroundBrush(Qt::black);
        view->setFrameShape(QFrame::NoFrame);
        setCentralWidget(view);
        view->installEventFilter(this);
       // view->setAcceptDrops(true);  // view = new DropGraphicsView(this);
        view->setDragMode(QGraphicsView::ScrollHandDrag);



     //   connect(view, &DropGraphicsView::imageDropped, [](const QString& path){
     //       qDebug() << "Dropped file:" << path;
     //   });

        connect(view, &DropGraphicsView::imageDropped, this, &ImageViewer::loadFromFile);

        scene = new QGraphicsScene(this);
        view->setScene(scene);

        for (int i = 0; i < 6; ++i) {
            auto* item = new ClickablePixmapItem(i);
            connect(item, &ClickablePixmapItem::clicked, this, &ImageViewer::onItemClicked);
            auto* effect = new QGraphicsOpacityEffect();
            item->setGraphicsEffect(effect);
            auto* anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(800);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            scene->addItem(item);
            pixmapItems.append(item);
            opacityEffects.append(effect);
            animations.append(anim);
        }

        setupMenu();

        slideshowTimer = new QTimer(this);
        connect(slideshowTimer, &QTimer::timeout, this, &ImageViewer::tickSlideshow);
    }

protected:
    enum Mode { Single, FourPane, SixPane };
    Mode slideshowMode = Single;

    DropGraphicsView* view;
    QGraphicsScene* scene;
    QList<ClickablePixmapItem*> pixmapItems;
    QList<QGraphicsOpacityEffect*> opacityEffects;
    QList<QPropertyAnimation*> animations;

    QTimer* slideshowTimer;
    QString folderPath;
    QStringList images;
    int currentIndex = 0;
    bool slideshowRunning = false;

    void setupMenu() {
        QMenu* fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open Image", this, &ImageViewer::openImage);
        fileMenu->addAction("Quit", qApp, &QApplication::quit);

        QMenu* slideshowMenu = menuBar()->addMenu("Slideshow");
        slideshowMenu->addAction("Start Slideshow (Single)", this, &ImageViewer::startSlideshowSingle);
        slideshowMenu->addAction("Start Slideshow (4-Pane)", this, &ImageViewer::startSlideshowFour);
        slideshowMenu->addAction("Start Slideshow (6-Pane)", this, &ImageViewer::startSlideshowSix);

        QMenu* viewMenu = menuBar()->addMenu("View");
        viewMenu->addAction("Toggle Fullscreen", this, &ImageViewer::toggleFullscreen);
    }

    void openImage() {
        QString file = QFileDialog::getOpenFileName(this, "Open Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!file.isEmpty())
            loadFromFile(file);
    }

    void loadFromFile(const QString& file) {
        QFileInfo info(file);
        folderPath = info.absolutePath();
        images = QDir(folderPath).entryList(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp", QDir::Files);
        currentIndex = images.indexOf(info.fileName());
        slideshowRunning = false;
        slideshowTimer->stop();
        slideshowMode = Single;
        loadImage(currentIndex);
    }

    bool eventFilter(QObject* obj, QEvent* event) {
        if (obj == view) {
            if (event->type() == QEvent::DragEnter) {
                auto* dragEvent = static_cast<QDragEnterEvent*>(event);
                if (dragEvent->mimeData()->hasUrls()) {
                    dragEvent->acceptProposedAction();
                    qDebug() << "eventFilter: dragEnter";
                    return true;
                }
            } else if (event->type() == QEvent::Drop) {
                auto* dropEvent = static_cast<QDropEvent*>(event);
                if (dropEvent->mimeData()->hasUrls()) {
                    const QList<QUrl> urls = dropEvent->mimeData()->urls();
                    if (!urls.isEmpty()) {
                        QString path = urls.first().toLocalFile();
                        qDebug() << "eventFilter: dropped" << path;
                        loadFromFile(path);
                        dropEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
        }

        return QObject::eventFilter(obj, event); // pass on to base class
    }

    void tickSlideshow() {
        if (slideshowMode == FourPane)
            loadTiled(4, 2);
        else if (slideshowMode == SixPane)
            loadTiled(6, 3);
        else
            nextImage();
    }

    void nextImage() {
        if (images.isEmpty()) return;
        currentIndex = (currentIndex + 1) % images.size();
        loadImage(currentIndex);
    }

    void loadImage(int index) {
        if (index < 0 || index >= images.size()) return;
        QString path = folderPath + "/" + images[index];
        QPixmap pix(path);
        if (pix.isNull()) return;

        QSize scaledSize = view->viewport()->size();
        pix = pix.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        for (int i = 0; i < pixmapItems.size(); ++i)
            pixmapItems[i]->setVisible(i == 0);

        auto* item = pixmapItems[0];
        item->setPixmap(pix);
        item->setPos(0, 0);
        item->imageIndex = index;
        item->fileName = QFileInfo(images[index]).fileName();

        if (item->textItem)
            item->textItem->setVisible(false);

        animations[0]->stop();
        opacityEffects[0]->setOpacity(0.0);
        animations[0]->start();
        scene->setSceneRect(item->boundingRect());
    }

    void loadTiled(int count, int cols) {
        if (images.size() < count) return;
        QSet<int> indexes;
        while (indexes.size() < count)
            indexes.insert(QRandomGenerator::global()->bounded(images.size()));

        int i = 0;
        QSize viewportSize = view->viewport()->size();
        int w = viewportSize.width() / cols;
        int h = viewportSize.height() / ((count + cols - 1) / cols);

        for (int idx : indexes) {
            QString path = folderPath + "/" + images[idx];
            QPixmap pix(path);
            if (pix.isNull()) continue;

            QPixmap scaled = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ClickablePixmapItem* item = pixmapItems[i];
            item->imageIndex = idx;
            item->fileName = QFileInfo(images[idx]).fileName();

            item->setPixmap(scaled);
            item->setPos((i % cols) * w, (i / cols) * h);
            item->setVisible(true);

            if (!item->textItem) {
                item->textItem = new QGraphicsTextItem(item->fileName, item);
                item->textItem->setDefaultTextColor(Qt::white);
                QFont font;
                font.setPointSize(10);
                font.setBold(true);
                item->textItem->setFont(font);

                QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                shadow->setBlurRadius(6);
                shadow->setColor(Qt::black);
                shadow->setOffset(1, 1);
                item->textItem->setGraphicsEffect(shadow);
            } else {
                item->textItem->setPlainText(item->fileName);
            }

            item->textItem->setVisible(true);
            item->textItem->setPos(5, scaled.height() - 20);

            animations[i]->stop();
            opacityEffects[i]->setOpacity(0.0);
            animations[i]->start();
            ++i;
        }

        for (; i < pixmapItems.size(); ++i) {
            pixmapItems[i]->setVisible(false);
            if (pixmapItems[i]->textItem)
                pixmapItems[i]->textItem->setVisible(false);
        }

        scene->setSceneRect(0, 0, viewportSize.width(), viewportSize.height());
    }

    void startSlideshowSingle() {
        slideshowMode = Single;
        slideshowRunning = true;
        slideshowTimer->start(3000);
    }

    void startSlideshowFour() {
        slideshowMode = FourPane;
        slideshowRunning = true;
        slideshowTimer->start(3000);
    }

    void startSlideshowSix() {
        slideshowMode = SixPane;
        slideshowRunning = true;
        slideshowTimer->start(3000);
    }

    void toggleFullscreen() {
        isFullScreen() ? showNormal() : showFullScreen();
    }

    void keyPressEvent(QKeyEvent* event) override {
        switch (event->key()) {
            case Qt::Key_Right: nextImage(); break;
            case Qt::Key_Left: currentIndex = (currentIndex - 1 + images.size()) % images.size(); loadImage(currentIndex); break;
            case Qt::Key_Space: slideshowRunning ? slideshowTimer->stop() : slideshowTimer->start(3000); slideshowRunning = !slideshowRunning; break;
            case Qt::Key_F: toggleFullscreen(); break;
            case Qt::Key_Escape: close(); break;
            default: QMainWindow::keyPressEvent(event); break;
        }
    }

private slots:
    void onItemClicked(int index) {
        slideshowTimer->stop();
        slideshowRunning = false;
        slideshowMode = Single;
        currentIndex = index;
        loadImage(index);
    }
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ImageViewer viewer;
    viewer.resize(1200, 800);
    viewer.show();
    return app.exec();
}
