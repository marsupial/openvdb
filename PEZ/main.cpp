//
//  main.cpp
//  pork
//
//  Created by jermainedupris on 6/18/19.
//  Copyright © 2019 jermainedupris. All rights reserved.
//

#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QColor>
#include <QtGui/QClipboard>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QListView>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QAction>

#include <QtCore/QStringListModel>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>

#include <string>
#include <set>
#include <dirent.h>

static void setStyle(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(255, 168, 40));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    //darkPalette.setColor(menubar->backgroundRole(), Qt::black);
    //darkPalette.setColor(menubar->foregroundRole(), Qt::white);
    
    app.setPalette(darkPalette);
    
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }"
                      //"QSplitter::handle { background:darkGray; }"
                      "QSplitter::handle:horizontal { width:4px; }"
                      "QSplitter::handle:vertical { height:4px; }"
                      "QMenu { background-color: rgb(53,53,53); }"
                      "QMenu::item:selected { background: rgb(255, 168, 40); }"
                      "QGraphicsView { background-color: rgb(53,53,53); border: 0 }"
                      "QComboBox { font-family: \"Verdana\"; font-size: 11pt; }"
                      //"QGraphicsView { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:1 rgb(53,53,53), stop:0 rgb(75,75,75)); }"
                      //"QMenuBar { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 lightgray, stop:1 darkgray);}"
                      );
}

using StringList = std::vector<std::string>;

class DataModel {
public:
    virtual ~DataModel() {}
    virtual StringList items() = 0;
    virtual StringList items(std::string what) = 0;
};
using DataModelData = std::unique_ptr<DataModel>;

class DirectoryData : public DataModel {
    std::string mRoot;

    StringList subDirectories(const std::string& directory) const {
        DIR *dir = opendir(directory.c_str());
        if (!dir)
            return {};
        
        StringList subd;
        struct dirent *entry = readdir(dir);
        while (entry != NULL) {
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.')
                subd.emplace_back(entry->d_name);
            entry = readdir(dir);
        }
        closedir(dir);
        return subd;
    }
public:
    DirectoryData(std::string root)
        : mRoot(std::move(root)) {
        
    }
    ~DirectoryData() {}

    StringList items(std::string directory) override {
        return subDirectories(mRoot+"/"+directory);
    }

    StringList items() override {
        return subDirectories(mRoot);
    }

};


class ProductSelection : public QWidget {
    using SelectionChanged = std::function<void(const QItemSelection&, const QItemSelection&)>;

    DataModelData    mModel;
    QListView        *mList;
    QComboBox        *mCombo;
    SelectionChanged mSelectionChanged;

    void buildSubList(std::string from) {
        if (auto* sel = mList->selectionModel())
            sel->selectionChanged({}, sel->selection());

        QStringListModel* model = new QStringListModel(mList);
        QStringList items;
        for (auto&& item : mModel->items(from))
            items.append(QString::fromStdString(std::move(item)));
        model->setStringList(items);
        mList->setModel(model);
        if (mSelectionChanged) {
            connect(mList->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, mSelectionChanged);
        }
    }

public:
    ProductSelection(DataModelData   data,
                     std::string     shot = {},
                     QWidget         *parent = nullptr,
                     Qt::WindowFlags f = Qt::WindowFlags())
        : QWidget(parent, f)
        , mModel(std::move(data))
    {
        QLayout* layout = new QVBoxLayout(this);
        mCombo = new QComboBox(this);
        mList = new QListView(this);
        mList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mList->setEditTriggers(QAbstractItemView::NoEditTriggers);

        int index = -1, idx = 0;
        for (auto&& item : mModel->items()) {
            if (item == shot)
                index = idx;
            mCombo->addItem(QString::fromStdString(std::move(item)));
            ++idx;
        }
        mCombo->setCurrentIndex(index);

        connect(mCombo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
                this, [this](const QString &cur) { buildSubList(cur.toStdString()); });

        layout->addWidget(mCombo);
        layout->addWidget(mList);
    }

    void setFunction(SelectionChanged changed) {
        mSelectionChanged = std::move(changed);
    }
};

class ProductInfo : public QFrame {
public:
    using QFrame::QFrame;
    virtual ~ProductInfo() {
        printf("~ProductInfo\n");
    }
};

struct FrameRange {
    int begin, end;
    int frame(float perecent) const {
        return begin + (end-begin) * perecent;
    }
};

class Viewport : public QFrame {
    float      mFrame = 0.f;
    FrameRange mRange = { 1001, 2001 };
    int        mTimelineGutter = 15;
    unsigned   mTimeScrub : 1;
    unsigned   mKeyScrub  : 1;
public:
    Viewport(QWidget         *parent = nullptr,
             Qt::WindowFlags f = Qt::WindowFlags())
        : QFrame(parent, f)
        , mTimeScrub(false) {
            setMouseTracking(true);
    }
    virtual ~Viewport() {
        printf("~Viewport\n");
    }

    void paintEvent(QPaintEvent *event) override {
        QFrame::paintEvent(event);
        QPainter paint(this);

        const QColor& h = palette().highlight().color();
        const QColor color(h.red(), h.green(), h.blue(), 160);
        paint.setBrush(QBrush(color));

        paint.setPen(Qt::NoPen);
        QRect rect(0, height()-4, mFrame * width(), 4);
        paint.drawRoundedRect(rect, 2, 2);
        if (!mTimeScrub)
            return;

        rect = QRect(std::min(width()-40, std::max(rect.right()-20,0)), rect.top()-15, 40, 15);
        paint.setBrush(Qt::NoBrush);
#if 0
        paint.setPen(color);
        paint.drawRoundedRect(rect, 2, 2);
#endif
        paint.setPen(Qt::white);
        paint.drawText(rect, Qt::AlignCenter, QString("%1").arg(mRange.frame(mFrame)));
    }
    void mousePressEvent(QMouseEvent* event) override {
        if (mKeyScrub || event->y() > (height() - mTimelineGutter))
            mTimeScrub = true;
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (mTimeScrub) {
            mFrame = std::max(std::min(float(event->x()) / float(width()), 1.f), 0.f);
            update();
        }
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (mTimeScrub) {
            mTimeScrub = false;
            update();
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_K) {
            mKeyScrub = true;
            event->accept();
            return;
        }
        event->setAccepted(false);
    }
    void keyReleaseEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_K) {
            mKeyScrub = false;
            return;
        }
        event->setAccepted(false);
    }

    void enterEvent(QEvent * event) override {
        QFrame::enterEvent(event);
        setFocus();
    }
    void leaveEvent(QEvent * event) override {
        QFrame::leaveEvent(event);
        static_cast<QWidget*>(parent())->setFocus();
    }
};

class ProductBrowser : public QFrame {
    QSplitter        *mSplitter;
    ProductSelection *mSelection;
    Viewport         *mViewport;
    QTabWidget       *mProductInfo;
    QSplitter        *mBottomSplitter;
    std::map<std::string, int> mTabViews;

    struct DrawBorder {
        QTimer  mTimer;
        uint8_t mAlpha = 255;
        bool animate() {
            const uint8_t prev = mAlpha;
            if (mAlpha <= 5) {
                mTimer.stop();
                mAlpha = 0;
            } else if (mAlpha != 255)
                mAlpha -= 5;
            return mAlpha != prev;
        }
        bool setAnimated(bool animate) {
            const uint8_t prevVal = mAlpha;
            mAlpha = animate ? 254 : 255;
            return prevVal != mAlpha;
        }
        bool done() const {
            return mAlpha == 0;
        }
    };
    std::unique_ptr<DrawBorder> mDrawBorder;

    void update() {
        QFrame::update();
    }

    bool setDropBorder(bool v, bool animate = false) {
        const bool timer = v || animate;
        const bool makeNew = bool(mDrawBorder) != timer;
        if (makeNew) {
            mDrawBorder.reset(timer ? new DrawBorder : nullptr);
            if (!mDrawBorder) {
                update();
                return false;
            }

            mDrawBorder->mTimer.start(30);
            connect(&mDrawBorder->mTimer, &QTimer::timeout, this, [this]() {
                if (!mDrawBorder || mDrawBorder->animate()) {
                    if (mDrawBorder && mDrawBorder->done())
                        mDrawBorder.reset();
                    update();
                }
            });
        }
        if (mDrawBorder) {
            if (mDrawBorder->setAnimated(animate) || makeNew)
                update();
        }
        return bool(mDrawBorder);
    }

    bool checkMimeData(const QDropEvent* e) {
        const QMimeData* mimeData = e->mimeData();
        if (!mimeData)
            return setDropBorder(false);
        for (auto& f : mimeData->formats())
            printf("f: %s\n", f.toStdString().c_str());
        printf("TEXT: %s\n", mimeData->text().toStdString().c_str());

        return setDropBorder(true);
    }
public:
    ProductBrowser(DataModelData   data,
                   std::string     shot = {},
                   QWidget         *parent = nullptr,
                   Qt::WindowFlags f = Qt::WindowFlags())
        : QFrame(parent, f)
        , mSelection(new ProductSelection(std::move(data), std::move(shot), this))
        , mViewport(new Viewport(this))
        , mProductInfo(new QTabWidget(this))
    {
        mSelection->setFunction([&](const QItemSelection &selected, const QItemSelection &deselected) {
            if (!deselected.empty()) {
                std::set<int> ordered;
                for (auto&& i : deselected.indexes()) {
                    auto itr = mTabViews.find(i.data().toString().toStdString());
                    assert(itr != mTabViews.end());
                    ordered.emplace(itr->second);
                    mTabViews.erase(itr);
                }
                for (auto i = ordered.rbegin(), e = ordered.rend(); i != e; ++i) {
                    const int key = *i;
                    ProductInfo* info = static_cast<ProductInfo*>(mProductInfo->widget(key));
                    mProductInfo->removeTab(key);
                    info->setParent(nullptr);
                    delete info;
                    for (auto&& i : mTabViews) {
                        if (i.second > key)
                            --i.second;
                    }
                }
            }
            for (auto&& i : selected.indexes()) {
                const std::string tab = i.data().toString().toStdString();
                mTabViews[tab] = mProductInfo->addTab(new ProductInfo(mProductInfo), QString::fromStdString(tab));
            }
            const bool visible = mProductInfo->count() != 0;
            mProductInfo->setVisible(visible);
            mViewport->setVisible(visible);
            mProductInfo->update();
        });

        mProductInfo->setTabPosition(QTabWidget::South);
        mProductInfo->setStyleSheet("QTabBar::tab { height: 16px; font-size: 8pt; }");

        mSelection->layout()->setContentsMargins(0, 0, 0, 0);

        mProductInfo->hide();
        mViewport->hide();

        mSplitter = new QSplitter(Qt::Orientation::Horizontal, this);
        mSplitter->addWidget(mSelection);
        mSplitter->addWidget(mViewport);
        mSplitter->setSizes(QList<int>() << 200 << 800);

        mBottomSplitter = new QSplitter(Qt::Orientation::Vertical, this);
        mBottomSplitter->addWidget(mSplitter);
        mBottomSplitter->addWidget(mProductInfo);
        mBottomSplitter->setSizes(QList<int>() << 500 << 300);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(mBottomSplitter);

        setAcceptDrops(true);
        setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    }

    void dragEnterEvent(QDragEnterEvent *event) override {
        if (checkMimeData(event))
            event->acceptProposedAction();
    }
    void dragMoveEvent(QDragMoveEvent *event) override {
        if (checkMimeData(event))
            event->acceptProposedAction();
    }
    void dragLeaveEvent(QDragLeaveEvent *event) override {
        setDropBorder(false);
    }
    void dropEvent(QDropEvent *event) override {
        if (checkMimeData(event)) {
            event->acceptProposedAction();
        }
        setDropBorder(false, true);
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->matches(QKeySequence::Paste)) {
            setDropBorder(true, true);
            event->accept();
            return;
        }
        event->setAccepted(false);
    }

    void paintEvent(QPaintEvent *event) override {
        QFrame::paintEvent(event);
        if (!mDrawBorder)
            return;

        QPainter paint(this);
        const QColor& h = palette().highlight().color();
        paint.setPen(QPen(QColor(h.red(), h.green(), h.blue(), mDrawBorder->mAlpha), 3));
        paint.setBrush(Qt::NoBrush);
        paint.drawRect(rect());
    }

    QSize sizeHint() const override {
        return { 1000, 800 };
    }
};

int main(int argc, const char * argv[]) {
    QApplication app(argc, (char**)argv, 0);
    setStyle(app);

    ProductBrowser widget(std::unique_ptr<DataModel>(new DirectoryData("/Volumes/Feets/Developer/build")));
    widget.show();
    app.exec();

    return 0;
}
