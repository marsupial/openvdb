
#include <QtCore/QElapsedTimer>

#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidgetAction>

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsSceneEvent>
#include <QtWidgets/QGraphicsBlurEffect>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QScrollBar>

#include <QtWidgets/QGraphicsItemAnimation>
#include <QtCore/QTimeLine>

#include <QtOpenGL/QGLWidget>

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>

#include "math/Vector3.h"
#include "Viewport.h"

Q_GUI_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

struct Settings {
    QColor SelectionColor     = QColor(255, 168, 40);
    QColor ConnectionColor    = QColor(42, 130, 218);
    float  ConnectionWidth    = 1.5f;
    float  NewConnectionWidth = 2.f;
    float  HorizontalPad      = 5;
    float  VerticalPad        = 5;
    float  Radius             = 5;
    float  RoundRadius        = 5;
    float  ConnectionCutoff   = 0.2f;
    float  CutWidth           = 1.5f;
    float  SelBlur            = 8;
    float  SelExpansion       = 2;
    float  SelLine            = -3;
    float  AnimationSpeed     = 1;

    int    Snap               = 20;
    bool   LineStyle          = 1;
    bool   Hollow             = 1;
    bool   InvertScaleX       = 1;
    QFont  Font, Title;
    float  Height() const { return QFontMetrics(Font).height() + VerticalPad; }
    float  TitleHeight() const { return QFontMetrics(Title).height() + VerticalPad; }

    QPoint zoomDelta(QPoint P) { if (InvertScaleX) P.setX(-P.x()); return P; }

    void init(QApplication& app) {
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
                          "QSplitter::handle:horizontal { width:6px; }"
                          "QSplitter::handle:vertical { height:6px; }"
                          "QMenu { background-color: rgb(53,53,53); }"
                          "QMenu::item:selected { background: rgb(255, 168, 40); }"
                          "QGraphicsView { background-color: rgb(53,53,53); border: 0 }"
                          //"QGraphicsView { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:1 rgb(53,53,53), stop:0 rgb(75,75,75)); }"
                          //"QMenuBar { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 lightgray, stop:1 darkgray);}"
                          );

        Font = QGraphicsTextItem().font();
        Title = Font;
        Font.setPixelSize(9);
        Font.setWeight(QFont::Thin);
        Snap = Height();
    }
};
Settings sSettings;

template <class T> class NonEmptyList {
    T mList;
public:
    NonEmptyList(T list) : mList(std::move(list)) {}
    operator bool () const { return !mList.empty(); }

    T& operator * () { return mList; }
    const T& operator * () const { return mList; }
    typename T::iterator begin() { return mList.begin(); }
    typename T::iterator end() { return mList.end(); }
    typename T::const_iterator begin() const { return mList.begin(); }
    typename T::const_iterator end() const { return mList.end(); }
};

template <class T> static NonEmptyList<T> listCheck(T list) {
    return NonEmptyList<T>(std::move(list));
}

template <class T>
class Range : public std::pair<T,T> {
public:
    Range(T b, T e) : std::pair<T,T>(b,e) {}

    T begin() const { return this->first; }
    T end() const { return this->second; }
    
    Range<T> resize(int amount) {
        if (amount < 0) return { this->first, this->second - amount };
        return { this->first + amount, this->second };
    }

    typename T::reference operator [] (int i) const { return *(this->first + i); }
};

class Stageview : public QGLWidget {
    Viewport mViewport;
    
    static Viewport::Action translate(unsigned action, const QMouseEvent *event) {
      Qt::KeyboardModifiers mods = event->modifiers();
      if (mods & Qt::ControlModifier)
        action |= Viewport::kCmdKeyModifier;
      if (mods & Qt::AltModifier)
        action |= Viewport::kAltKeyModifier;
      if (mods & Qt::MetaModifier)
        action |= Viewport::kCtlKeyModifier;
      switch (event->button()) {
          case Qt::MouseButton::MiddleButton: action |= Viewport::kMMouseModifier; break;
          case Qt::MouseButton::RightButton:  action |= Viewport::kRMouseModifier; break;
          default: break;
      }
      return (Viewport::Action) action;
    }

protected:
    virtual void mousePressEvent(QMouseEvent *event) {
        mViewport.mouse(event->x(), event->y(), translate(Viewport::kMouseDown, event));
        update();
    }
    virtual void mouseReleaseEvent(QMouseEvent *event) {
        mViewport.mouse(event->x(), event->y(), translate(Viewport::kMouseUp, event));
        update();
    }
    virtual void mouseMoveEvent(QMouseEvent *event) {
        mViewport.mouse(event->x(), event->y(), translate(Viewport::kMouseMove, event));
        update();
    }
    virtual void wheelEvent(QWheelEvent *event) {
        auto delta = sSettings.zoomDelta(event->angleDelta());
        mViewport.mouse(delta.x(), delta.y(), Viewport::kScrollWheel);
        update();
    }

    virtual void initializeGL() {
        mViewport.init(width(),height());
    }
    virtual void resizeGL(int w, int h) {
        mViewport.resize(w, h);
    }
    virtual void paintGL() {
        if (isValid())
            mViewport.render();
    }
public:
    Stageview() {
        QGLFormat format;
        format.setVersion(3, 3);
        format.setProfile(QGLFormat::CoreProfile);
        setFormat(format);
    }
    virtual QSize sizeHint() const { return QSize(1280, 720)*0.7f; }
};

class LevenshteinDistance {
    std::vector<size_t> mCosts;
    QByteArray mStr;
public:
    LevenshteinDistance(const QString& str) : mStr(str.toUtf8()) {}

    size_t operator () (const QByteArray &s1)
    {
      const size_t m(s1.size());
      const size_t n(mStr.size());
     
      if( m==0 ) return n;
      if( n==0 ) return m;

      mCosts.resize(n+1);
      for( size_t k=0; k<=n; k++ ) mCosts[k] = k;
     
      size_t i = 0;
      for ( auto it1 = s1.begin(); it1 != s1.end(); ++it1, ++i )
      {
        mCosts[0] = i+1;
        size_t corner = i;
     
        size_t j = 0;
        for ( auto it2 = mStr.begin(); it2 != mStr.end(); ++it2, ++j )
        {
          size_t upper = mCosts[j+1];
          if( *it1 != *it2 )
          {
            size_t t(upper<corner?upper:corner);
            mCosts[j+1] = (mCosts[j]<t?mCosts[j]:t)+1;
          }
          else
            mCosts[j+1] = corner;

     
          corner = upper;
        }
      }
     
      return mCosts[n];
    }
};


class TabInput;
class TabMenu;
static QElapsedTimer sEventCompress;

class TabMenuBase : public QMenu {
public:
    TabMenuBase(QWidget* parent) : QMenu(parent) {
        QFont F = font();
        F.setPointSize(9);
        setFont(F);
        //setStyleSheet("border-radius: 5px;");
        connect(this, &QMenu::aboutToShow, [&]() { aboutToShow(); });
        sEventCompress.restart();
    }
    void keyPressEvent(QKeyEvent *event) final;
    void mouseReleaseEvent(QMouseEvent *e) final;
    void keyNavEvent(QKeyEvent* e) { QMenu::keyPressEvent(e); }
    void mouseMoveEvent(QMouseEvent* e) final;
    void wheelEvent(QWheelEvent* e) final;
    //bool event(QEvent* e) final;
    //void enterEvent(QEvent* e) final;
    void leaveEvent(QEvent* e) final;

    TabMenu& root();
    TabInput& input();
    void aboutToShow();
    void buildMatches(const std::string&);
    bool finish(QAction* action, const QKeyEvent* e = nullptr);
    void hide() { setVisible(false); }
};

class TabInput : public QLineEdit {
    std::unique_ptr<TabMenuBase> mFiltered;
    unsigned mBlockLeave : 1;

    void showMenu(const QString& after, TabMenuBase* base) {
        TabMenuBase* parent = this->parent();
        if (base && base != mFiltered.get()) {
            while (base != parent) {
                base->setActiveAction(nullptr);
                base->hide();
                base = static_cast<TabMenuBase*>(base->parentWidget());
            }
        }
        //parent->setActiveAction(nullptr);

        if (!mFiltered)
            mFiltered.reset(new TabMenuBase(parent));

        parent->buildMatches(after.toStdString());

        //QByteArray look = after.toUtf8();
        //LevenshteinDistance ldist(after);
        std::pair<size_t, QAction*> match(100,nullptr);

#if 0
        QList<QAction*> filtered;
        for (QAction* action : parent->findChildren<QAction*>()) {
            if (action->menu()) continue;
            if (action->text().toLower().contains(after)) {
                //const size_t dist = ldist(action->text().toUtf8());
                //if (dist < match.first)
                //    match = std::pair<size_t, QAction*>(dist,action);
                if (!match.second) match.second = action;
                filtered.push_back(action);
            }
        }
        QList<QAction*> current = mFiltered->actions();
        auto cItr = current.begin(), cEnd = current.end(),
             fItr = filtered.begin(), fEnd = filtered.end();
         while (cItr < cEnd && fItr != fEnd) {
             if (*cItr != *fItr) break;
             ++cItr;
             ++fItr;
         }
         if (cItr != current.begin()) {
             while (cItr < cEnd) {
                 mFiltered->removeAction(*cItr);
                 ++cItr;
             }
         } else
             mFiltered->clear();

         while (fItr < fEnd) {
             mFiltered->addAction(*fItr);
             ++fItr;
         }
#else
        mFiltered->clear();
        for (QAction* action : parent->findChildren<QAction*>()) {
            if (action->menu()) continue;
            if (action->text().toLower().contains(after)) {
                //const size_t dist = ldist(action->text().toUtf8());
                //if (dist < match.first)
                //    match = std::pair<size_t, QAction*>(dist,action);
                if (!match.second) match.second = action;
                mFiltered->addAction(action);
            }
        }
        if (mFiltered->isEmpty()) {
            mFiltered->hide();
            return;
        }
#endif
/*
        QRect R = mFiltered->actionGeometry(match.second);
        QPoint P(R.x()+R.width()/2, R.y()+R.height()/2);
        QHoverEvent E(QEvent::HoverLeave, P, P);
        //QMouseEvent E(QEvent::MouseEx, P, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(parentWidget(), &E);
*/
        if (match.second)
            mFiltered->setActiveAction(match.second);

        if (!mFiltered->isVisible()) {
            hideParent(parent);
            showMenu(true);
        } else
            mBlockLeave = true;
    }

    TabMenuBase* parent() { return static_cast<TabMenuBase*>(parentWidget()); }
    TabMenuBase* activeMenu() { return filtered() ? mFiltered.get() : parent(); }

  public:
    TabInput(QWidget* parent) : QLineEdit(parent), mBlockLeave(0) {
        setWindowFlags(Qt::FramelessWindowHint);
        setMinimumSize(200, 24);
        setMaximumSize(200, 24);
        setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        setClearButtonEnabled(true);
        setStyleSheet("border-style: none; color: rgb(255, 168, 40); background-color: rgba(0,0,0,0);");
        //addAction(":/resources/search.ico", QLineEdit::LeadingPosition);
    }

    bool filtered() const { return mFiltered && mFiltered->isVisible(); }
    TabMenuBase* filterMenu() const { return mFiltered.get(); }

    bool blockLeave(TabMenuBase* menu) {
      if (!mBlockLeave || menu != mFiltered.get()) return false;
      mBlockLeave = false;
      return true;
    }

    void begin() {
        if (mFiltered) mFiltered->clear();
        setText("");
        setFocus();
    }
    void end() {
        setText("");
        if (mFiltered) {
            mFiltered->hide();
            mFiltered->clear();
            mFiltered.reset();
        }
    }

    void keyPressEvent(QKeyEvent *event, TabMenuBase* base) {
        const QString before = text();
        switch (event->key()) {
            case Qt::Key_Escape:
                if (!before.isEmpty()) {
                    setText("");
                    mFiltered.reset();
                    return;
                }
                return parentWidget()->hide();

            case Qt::Key_Tab:
              parent()->finish(base ? base->activeAction() : nullptr, event);
              return;

            case Qt::Key_Return:
            case Qt::Key_Enter:
              parent()->finish(base ? base->activeAction() : nullptr, event);
              return;

            case Qt::Key_Left:
            case Qt::Key_Right:
                if (base && base == mFiltered.get()) {
                    setFocus();
                    return QLineEdit::keyPressEvent(event);
                }

            case Qt::Key_Up:
            case Qt::Key_Down:
                if (!base) {
                    base = parent();
                    // Jump over us
                    if (!base->activeAction())
                        base->keyNavEvent(event);
                }
                return base->keyNavEvent(event);

            default:
                QLineEdit::keyPressEvent(event);
        }

        QString after = text();
        if (before != after) {
            if (after.isEmpty()) {
                mFiltered.reset();
                return;
            }
            showMenu(after.toLower(), base);
        }
    }

    static QPoint& movePoint(QPoint& P, int x, int y) {
        P.setX(P.x() + x);
        P.setY(P.y() + y);
        return P;
    }

    void hideParent(QMenu* parent) {
        if (parent) {
            if (QAction* sel = parent->activeAction()) {
                if (QMenu* child = sel->menu())
                    child->hide();
                parent->setActiveAction(nullptr);
            }
        }
    }

    void showMenu(bool warp = false) {
        assert(mFiltered && !mFiltered->isEmpty());
        if (warp) warp = !rect().contains(mapFromGlobal(cursor().pos()));
        QPoint P = mapToGlobal(pos());
        mFiltered->move(movePoint(P, width(), 0));
        mFiltered->show();

        if (warp) {
            this->cursor().setPos(movePoint(P, mFiltered->width()/2, 10));
            //P = mFiltered->mapFromGlobal(P);
            //QHoverEvent E(QEvent::HoverEnter, P, P);
            ////QMouseEvent E(QEvent::MouseMove, P, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            //QApplication::sendEvent(mFiltered.get(), &E);
        }
    }

    void keyPressEvent(QKeyEvent *event) final {
        keyPressEvent(event, nullptr);
    }
    void wheelEvent(QWheelEvent* e) final {
        activeMenu()->wheelEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent *e);

    void enterEvent(QEvent *event) final {
        if (mFiltered && mFiltered->isHidden()) {
            TabMenuBase* tm = parent();
            if (tm->activeAction())
                tm->setActiveAction(nullptr);
            if (!mFiltered->isEmpty())
                mFiltered->setActiveAction(mFiltered->actions().front());
            mFiltered->show();
        }
        QLineEdit::enterEvent(event);
    }
    void leaveEvent(QEvent *event) final {
        if (mFiltered && mFiltered->isVisible()) mFiltered->hide();
        QLineEdit::leaveEvent(event);
    }
};

class TabMenu : public TabMenuBase {
    TabInput mLineEdit;
    std::vector<std::unique_ptr<TabMenuBase>> mSubMenus;
    std::unordered_map<TabMenuBase*, std::vector<std::string>> mMenuEntries;
    QAction* mSelected;
    std::unique_ptr<std::string> mLastAction;
    unsigned mTrackFilter : 1;
    unsigned mIgnoreSubMenu : 1;
    unsigned mRepeatLast : 1;
    unsigned mAction : 1;

    friend class TabMenuBase;

    void create(std::unordered_map<TabMenuBase*, std::vector<std::string>>::iterator Itr) {
        assert(Itr != mMenuEntries.end());
        TabMenuBase* menu = Itr->first;
        for (const std::string& item : Itr->second)
            menu->addAction(item.c_str());

        mMenuEntries.erase(Itr);
    }

    void create(TabMenuBase* parent) {
        create(mMenuEntries.find(parent));
    }

    TabMenu(QWidget* parent = nullptr)
      : TabMenuBase(nullptr), mLineEdit(this), mSelected(nullptr),
        mTrackFilter(1), mIgnoreSubMenu(1), mRepeatLast(1), mAction(0) {
        QWidgetAction *widgetAction = new QWidgetAction(parent);
        widgetAction->setDefaultWidget(&mLineEdit);
        addAction(widgetAction);

        //setFocusProxy(mLineEdit.get());
        mLineEdit.setFont(font());
     }
public:
    TabMenu(std::map<std::string, std::vector<std::string>>& items, QWidget* parent = nullptr)
      : TabMenu(parent) {
        for (auto&& menu : items) {
            mSubMenus.emplace_back(new TabMenuBase(this));
            addMenu(mSubMenus.back().get())->setText(menu.first.c_str());
            mMenuEntries[mSubMenus.back().get()].swap(menu.second);
        }
     }
    TabMenu(std::vector<std::string>& items, bool repeatLast = 0, QWidget* parent = nullptr)
      : TabMenu(parent) {
        mRepeatLast = repeatLast;
        for (auto&& item : items) {
            addAction(item.c_str());
        }
     }
 
    std::pair<QAction*, std::pair<QWidget*,QPoint>>
    exec(const char* placholder = "Create...", float offsetX = 0.5f) {
        // First time width is inaccurate!
        const QPoint pos = QCursor::pos();
        QWidget* widget = qApp->widgetAt(pos);

        move(pos.x() - (sizeHint().width() * offsetX),
             pos.y() - mLineEdit.height()*0.5f);

        QAction* prev = mSelected;
        const unsigned prevRepeat = mRepeatLast;
        if (!mRepeatLast || !prev) {
            mLineEdit.setPlaceholderText(placholder);
            mRepeatLast = 0;
        } else
            mLineEdit.setPlaceholderText(mSelected->text());

        mSelected = nullptr;
        mAction = 0;
        mLineEdit.begin();
        QMenu::show();
        QMenu::exec();
        mRepeatLast = prevRepeat;
        if (mRepeatLast && !mSelected && mAction)
            mSelected = prev;
        mLineEdit.end();
        return { mSelected, {widget, widget ? widget->mapFromGlobal(pos) : pos} };
    }
    bool repeatLast() const { return mRepeatLast; }
};

TabMenu& TabMenuBase::root() {
    QWidget* root = this;
    QWidget* parent = root->parentWidget();
    while (parent) {
        root = parent;
        parent = parent->parentWidget();
    }
    assert(root);
    return *static_cast<TabMenu*>(root);
}

TabInput& TabMenuBase::input() {
    return root().mLineEdit;
}

void TabMenuBase::keyPressEvent(QKeyEvent *event) {
    input().keyPressEvent(event, this);
}

bool TabMenuBase::finish(QAction* action, const QKeyEvent* e) {
    TabMenu& r = root();

    if (r.mIgnoreSubMenu && !e && (!action || action->menu()))
        return false;

    r.mSelected = action && !action->menu() ? action : nullptr;
    r.mAction = 1;
    if (r.mRepeatLast) {
        if (e && !r.mLineEdit.text().isEmpty())
            r.mAction = 0;
        else if (!e && !action)
            r.mAction = 0;
    }
    r.hide();
    // r.mLineEdit.end();
    return true;
}

void TabMenuBase::mouseReleaseEvent(QMouseEvent *e) {
    if (finish(QMenu::activeAction()))
        QMenu::mouseReleaseEvent(e);
}

void TabInput::mouseReleaseEvent(QMouseEvent *e) {
    if (static_cast<TabMenu*>(parent())->repeatLast() && text().isEmpty()) {
        QKeyEvent ke(QEvent::KeyRelease,Qt::Key_Enter, Qt::NoModifier);
        parent()->finish(nullptr, &ke);
    }
    QLineEdit::mouseReleaseEvent(e);
}

void TabMenuBase::leaveEvent(QEvent* e) {
    if (!input().blockLeave(this))
        return QMenu::leaveEvent(e);
}

void TabMenuBase::mouseMoveEvent(QMouseEvent* e) {
    QWidget* over = qApp->widgetAt(e->globalPos());
    TabMenuBase* filt = input().filterMenu();
    TabInput& input = this->input();
    if (root().mTrackFilter) {
        if (filt) {
            // Hand off the event when over the filter menu.
            if (over == filt)
                return QMenu::mouseMoveEvent(e);

            switch (filt->isVisible()) {
                case true:
                    // Select a sub-menu in the root
                    if (over == this->parentWidget())
                        filt->hide();
                    return;

                case false:
                    if (over == &input) {
                        // Over the input text, show the hidden filter menu
                        // Make sure to hide any visible sub-menu in the parent.
                        input.hideParent(static_cast<QMenu*>(this->parentWidget()));
                        if (!filt->isEmpty()) {
                            filt->setActiveAction(filt->actions().front());
                            input.showMenu();
                        }
                        return;
                    }
                    break;
            }
        } else if (over == &input)
            input.hideParent(static_cast<QMenu*>(this->parentWidget()));
    }

    if (over != &input && (!filt || filt->isHidden()))
        return QMenu::mouseMoveEvent(e);
}

void TabMenuBase::wheelEvent(QWheelEvent* e)  {
    //return QMenu::wheelEvent(e);

    if (sEventCompress.elapsed() < 50) return;
    sEventCompress.restart();

    if (fabs(e->angleDelta().y()) > 5) {
        QAction* active = activeAction();
        auto all = actions();
        if (e->angleDelta().y() > 0) {
            bool next = !active;
            for (QAction* action : all) {
                if (next) {
                    setActiveAction(action);
                    return;
                }
                next = action == active;
            }
            //setActiveAction(all.front());
        } else {
            QAction* prev = nullptr;
            for (QAction* action : all) {
                if (prev && action == active) {
                    setActiveAction(prev);
                    return;
                }
                prev = action;
            }
            //setActiveAction(all.back());
        }
    }
}

void TabMenuBase::aboutToShow() {
    if (!isEmpty()) return;
    assert(&root() != this);
    root().create(this);
}

void TabMenuBase::buildMatches(const std::string& str) {
    assert(&root() == this);
    TabMenu* tm = static_cast<TabMenu*>(this);
    for (auto itr = tm->mMenuEntries.begin(), end = tm->mMenuEntries.end(); itr != end;) {
        bool found = false;
        for (const std::string& item : itr->second) {
          found =
              std::search(item.begin(), item.end(), str.begin(), str.end(),
              [](char A, char B) -> bool {
                return tolower(A) == tolower(B);
              }) != item.end();
          if (found) {
            tm->create(itr++);
            break;
          }
        }
        if (!found) ++itr;
    }
}

static QColor fColor(float r, float g, float b) {
    return QColor(r * 255.f, g * 255.f, b * 255.f);
}

static void dump(const QPointF& P, const char* label="") {
printf("%s: (%f, %f)\n", label, P.x(), P.y());
}
static void dump(const QMatrix& M, const char* label="") {
printf("%s: (%f, %f, %f, %f, %f, %f)\n", label, M.m11(), M.m12(), M.m21(), M.m22(), M.dx(), M.dy());
}

constexpr static qreal zEpsilon() { return std::numeric_limits<qreal>::epsilon(); }
constexpr static qreal zMin()     { return std::numeric_limits<qreal>::min(); }
constexpr static qreal zMax()     { return std::numeric_limits<qreal>::max(); }

class NodeItem;
enum {
    kConnectionType = QGraphicsItem::UserType + 100,
    kNodeType,
};

static QPointF snapPoint(QPointF P, int snap = sSettings.Snap) {
    auto snapper = [] (float v, int snap) -> float {
        float frac = modf(v/snap, &v);
        v *= snap;
        return frac > 0.5f ? v + snap : v;
    };
    return QPointF(snapper(P.x(), snap), snapper(P.y(), snap));
}

template <typename T = unsigned>
class TaggedInteger {
    std::pair<T, uint8_t> mValue;
public:
    typedef uint8_t flag_type;
    typedef T value_type;
    TaggedInteger(T val = 0, flag_type t = 0) : mValue(val, t) {}

    T operator [] (flag_type i) { return mValue.second == i ? mValue.first : 0; }
    flag_type tag() const { return mValue.second; }

    T value() const { return mValue.first; }
    bool operator == (const TaggedInteger& R) const { return mValue == R.mValue; }
    bool operator != (const TaggedInteger& R) const { return mValue != R.mValue; }

    TaggedInteger operator | (int t ) const {
        assert(t >= 0 && t < std::numeric_limits<flag_type>::max());
        return {mValue.first, flag_type(mValue.second | t)};
    }

    flag_type operator & (int f) const { return mValue.second & f; }
    operator flag_type () const { return mValue.second; }

    void clear() { mValue = {0,0}; }
    TaggedInteger reduce(flag_type F) const { return TaggedInteger(mValue.first, mValue.second & F); }
};
typedef TaggedInteger<uint16_t> NodePiece;
typedef TaggedInteger<uint16_t> PortValue;

namespace std {
  template <> struct hash<PortValue> {
    std::size_t operator()(const PortValue& k) const {
      return size_t(k.value()) | (size_t(k.tag()) << (sizeof(PortValue::value_type)*8));
    }
  };
}

enum {
    kInputTag    = 1,
    kOutputTag   = 2,
    kNameTag     = 4,
    kAreaTag     = 8,
    kTitleTag    = 16,
    kSelectConnectionTag = 32,
    kLabelTag      = kNameTag | kAreaTag,
    kConnectionTag = kInputTag | kOutputTag,
    
    kMorePort = std::numeric_limits<PortValue::value_type>::max(),
    kThisPort = std::numeric_limits<PortValue::value_type>::max()-1,
    
    kTitleArea        = 0,
    kMaximizeButton   = 1,
    kConnectedButton  = 2,
    kMinimizeButton   = 3,
    kBottomArea       = 4,
};


class BlurredImage {
    const QPointF    mOffset;
    QImage   mImage;
    QPainter mPainter;
public:
    BlurredImage(QPointF O, QSize S, const QPaintDevice* device) : mOffset(O), mImage(S*qApp->devicePixelRatio(), QImage::Format_ARGB32_Premultiplied), mPainter(&mImage) {
        mImage.fill(0);
        mPainter.initFrom(device);
        mPainter.setRenderHint(QPainter::Antialiasing, true);
        const float pa = qApp->devicePixelRatio();
        mPainter.scale(pa, pa);
        mPainter.translate(mOffset);
        
    }
    void operator () (QPainter* painter, float blur = sSettings.SelBlur) {
        if (blur != 0.f) qt_blurImage(mImage, blur, true, false);
        //const auto mode = painter->compositionMode();
        //painter->setRenderHint(QPainter::SmoothPixmapTransform);
        //painter->setCompositionMode(QPainter::CompositionMode_Plus);
        painter->drawImage(QRectF(-mOffset, mImage.size()*(1.f/qApp->devicePixelRatio())), mImage);
        //painter->drawImage(-mOffset, mImage);
        //painter->setCompositionMode(mode);
    }
    QPainter* operator -> () { return &mPainter; }
    operator QImage& () { return mImage; }
    operator QPainter* () { return &mPainter; }
};

class NodeConnection : public QGraphicsPathItem {
    struct BoundPort {
        const QGraphicsItem *item;
        PortValue port;
        BoundPort(const QGraphicsItem* i, PortValue p) : item(i), port(p) {}
    };
protected:
    const float& mWidth;
    const QColor& mColor;
    const union { BoundPort mPort0; const QPointF* mP0; };
    const union { BoundPort mPort1; const QPointF* mP1; };
    unsigned mBound : 1;

    void setup(float Z, bool createPath) {
          setZValue(Z);
          setAcceptHoverEvents(true);
          setFlag(QGraphicsItem::ItemIsSelectable, true);
          setFlag(QGraphicsItem::ItemIsMovable, false);
          if (createPath) changed();
    }
public:
    NodeConnection(const QGraphicsItem* A, const QGraphicsItem* B,
                   const PortValue& a, const PortValue& b, const float& w,
                   const QColor& C, qreal Z = zMax(), QGraphicsItem* P = nullptr,
                   bool createPath = true)
      : mWidth(w), mColor(C), mPort0({A,a}), mPort1({B,b}), mBound(1), QGraphicsPathItem(P) {
          setup(Z, createPath);
      }
    NodeConnection(const QPointF& a, const QPointF& b, const float& w,
                   const QColor& C, qreal Z = zMax(), QGraphicsItem* P = nullptr,
                   bool createPath = true)
      : mWidth(w), mColor(C),  mP0(&a), mP1(&b), mBound(0), QGraphicsPathItem(P) {
          setup(Z, createPath);
    }

    enum { Type = kConnectionType };
    int type() const final { return Type; }

    QRectF boundingRect() const final {
        qreal skip = sSettings.Radius * 1.5f;
        QRectF B = QGraphicsPathItem::boundingRect();
        return B.adjusted(-skip, -skip, skip, skip);
    }

    void drawPort(QPainter *painter, const QPointF& P, bool restore = true) {
        painter->drawEllipse(P, 2.5f, 2.5f);
        if (sSettings.Hollow) {
            painter->setPen(QPen(Qt::NoPen));
            painter->setBrush(QBrush(pen().color()));
            painter->drawEllipse(P, 1.f,1.f);
            if (restore) {
                painter->setPen(pen());
                painter->setBrush(Qt::NoBrush);
            }
        }
    }

    std::pair<QPointF, QPointF> toScene() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) final {
        painter->setClipRect(option->exposedRect.marginsAdded(QMarginsF(1,1,1,1)));
        QStyleOptionGraphicsItem unSelect;
        if (option->state & QStyle::State_Selected) {
            unSelect = *option;
            unSelect.state = unSelect.state & ~QStyle::State_Selected;
            option = &unSelect;
        }
        QGraphicsPathItem::paint(painter, &unSelect, widget);
        if (!sSettings.Hollow) painter->setBrush(QBrush(pen().color(),Qt::SolidPattern));
        auto P = toScene();
        drawPort(painter, P.first);
        drawPort(painter, P.second, false);
    }

    const QPointF& outputPt() const { assert(!mBound); return *mP0; }
    const QPointF& inputPt() const { assert(!mBound); return *mP1; }

    PortValue outputPort() const { assert(mBound); return mPort0.port; }
    PortValue inputPort() const { assert(mBound); return mPort1.port; }

    const QGraphicsItem* output() const { return mBound ? mPort0.item : nullptr; }
    const QGraphicsItem* input() const { return mBound ? mPort1.item : nullptr; }
    
    void setPen(const QColor& C, float width) {
        QPen Pen = QPen(C);
        Pen.setWidth(width);
        Pen.setCapStyle(Qt::RoundCap);
        QGraphicsPathItem::setPen(Pen);
    }
    void setPen(const QColor& C) { setPen(C, mWidth); }
    void setPen() { setPen(mColor, isSelected() ? sSettings.NewConnectionWidth : mWidth); }

    static QPainterPath makePath(std::pair<QPointF, QPointF> P) {
        const QPointF &P0 = P.first, &P1 = P.second;
        QPainterPath path(P0);
        if (sSettings.LineStyle == 1) {
            float xDistance = P1.x() - P0.x();
            //double yDistance = P1.y() - P0.y() - 100;

            float defaultOffset = 200;

            float minimum = qMin(defaultOffset, std::abs(xDistance));

            float verticalOffset = 0;

            float ratio1 = 0.5;

            if (xDistance <= 0) {
                verticalOffset = -minimum;
                ratio1 = 1.0;
            }

            //double verticalOffset2 = verticalOffset;
            //if (xDistance <= 0)
            //verticalOffset2 = qMin(defaultOffset, std::abs(yDistance));
            //auto sign = [](double d) { return d > 0.0 ? +1.0 : -1.0; };
            //verticalOffset2 = 0.0;

            QPointF c0(P0.x() + minimum * ratio1, P0.y() + verticalOffset);
            QPointF c1(P1.x() - minimum * ratio1, P1.y() + verticalOffset);
            path.cubicTo(c0, c1, P1);

        } else
            path.lineTo(P1);
        return path;
    }

    void changed() {
        prepareGeometryChange();
        setPen();
        setPath(makePath(toScene()));
    }

    void baked(const QPointF& p0, const QPointF& p1) {
        prepareGeometryChange();
        mP0 = &p0;
        mP1 = &p1;
        mBound = 0;
        setPath(makePath({p0,p1}));
    }

    QPainterPath shape() const final {
        QPainterPathStroker stroker;
        stroker.setWidth(6.f);
        auto P = toScene();
        P.first.setX(P.first.x()+sSettings.Radius*1.5f);
        P.second.setX(P.second.x()-sSettings.Radius*1.5f);
        return stroker.createStroke(makePath(P));
    }
    bool skipMouseEvent(QPointF P, QGraphicsSceneEvent* E) const {
        return false;
        P = mapToScene(P);
        auto Ps = toScene();
        if ((P-Ps.first).manhattanLength() < sSettings.Radius ||
            (P-Ps.second).manhattanLength() < sSettings.Radius) {
            E->ignore();
            return true;
        }
        return false;
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) final {
        if (skipMouseEvent(event->pos(), event)) return;
        setPen(sSettings.SelectionColor, sSettings.NewConnectionWidth);
        QGraphicsPathItem::hoverEnterEvent(event);
    }
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) final {
        if (skipMouseEvent(event->pos(), event)) {
            setPen();
            return;
        }
        QGraphicsPathItem::hoverMoveEvent(event);
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) final {
        setPen();
        QGraphicsPathItem::hoverLeaveEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) final {
        if (skipMouseEvent(event->pos(), event)) return;
        return QGraphicsPathItem::mousePressEvent(event);
    }
/*
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) final {
        if (skipMouseEvent(event->pos(), event)) return;
        if (isSelected()) setPen(mColor, sSettings.NewConnectionWidth);
        return QGraphicsPathItem::mouseReleaseEvent(event);
    }
*/
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) final {
        if (change == ItemSelectedChange) {
            setPen(mColor, value.toBool() ? sSettings.NewConnectionWidth : mWidth);
            //if (!value.toBool()) setPen(mColor, mWidth);
            //update();
        }
        return QGraphicsPathItem::itemChange(change, value);
    }
};

#ifndef NDEBUG

class InstanceTracker {
    size_t mInstances = 0;
    const char* const mName;
public:
    InstanceTracker(const char* name) : mName(name) {}
    ~InstanceTracker() { if (mInstances) printf("%lu remaining %s%s\n", mInstances, mName, mInstances > 1 ? "s" : ""); }
    void operator ++ () { ++mInstances; }
    void operator -- () { --mInstances; }
};
static InstanceTracker sAnimTracker("animations");
class ItemAnimation : public QGraphicsItemAnimation {
#endif

protected:
    static void doIt(QGraphicsItem*) {}
    static float speed() { return sSettings.AnimationSpeed; }
public:
    enum { kDefaultLength = 400 };
    ItemAnimation(QGraphicsItem* item, unsigned millisec) {
        QTimeLine* anim = new QTimeLine(millisec*speed());
        anim->setFrameRange(0, 100);
        anim->setCurveShape(QTimeLine::EaseOutCurve);
        setTimeLine(anim);
        if (item)
          setItem(item);
        anim->start();
#ifndef NDEBUG
        ++sAnimTracker;
#endif
    }
    ~ItemAnimation() {
#ifndef NDEBUG
        --sAnimTracker;
#endif
    }
    virtual void afterAnimationStep(qreal step) = 0;

    template <class T, class C = QGraphicsItem, typename... Args>
    static T* create(C* item, Args&&... args) {
        if (speed() > 0)
          return new T(item, T::kDefaultLength, std::forward<Args>(args)...);
        T::doIt(item, std::forward<Args>(args)...);
        return nullptr;
    }
};

class MatrixAnimator : public ItemAnimation {
    QGraphicsView* const mView;
    const QMatrix mEnd, mStart;
    static std::unordered_map<QGraphicsView*, MatrixAnimator*> sActive;
public:
    enum { kDefaultLength = 200 };
    static void doIt(QGraphicsItem* item, QGraphicsView* view, QMatrix Mdst) {
        view->setMatrix(Mdst);
    }

    void afterAnimationStep(qreal a) {
        if (a >= 1.f) { delete this; return; }
        qreal src[6] = { mStart.m11(), mStart.m12(), mStart.m21(), mStart.m22(),
                         mStart.dx(), mStart.dy() };
        qreal dst[6] = { mEnd.m11(), mEnd.m12(), mEnd.m21(), mEnd.m22(),
                         mEnd.dx(), mEnd.dy() };
        for (unsigned i = 0; i < 6; ++i)
            dst[i] = src[i] + a * (dst[i]-src[i]);
        mView->setMatrix(QMatrix(dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]));
    }
    MatrixAnimator(QGraphicsItem* item, unsigned ms, QGraphicsView* V, QMatrix Mdst)
      : ItemAnimation(item, ms), mView(V), mEnd(Mdst), mStart(V->matrix()) {
      auto itr = sActive.find(V);
      if (itr != sActive.end()) {
          MatrixAnimator* anim = itr->second;
          sActive.erase(itr);
          anim->timeLine()->stop();
          delete anim;
      }
      sActive.emplace(V, this);
    }
    ~MatrixAnimator() {
        auto itr = sActive.find(mView);
        if (itr != sActive.end()) {
            doIt(item(), mView, mEnd);
            sActive.erase(itr);
        }
    }
};
std::unordered_map<QGraphicsView*, MatrixAnimator*> MatrixAnimator::sActive;

class ItemRemover : public ItemAnimation {
public:
    template <typename... Args>
    static void doIt(QGraphicsItem* item, Args&&... args) { item->scene()->removeItem(item); }

    void afterAnimationStep(qreal step) {
        if (step >= 1.f) { delete this; return; }
        QGraphicsItem* i = this->item();
        i->setOpacity(1.f-step);
    }
    ItemRemover(QGraphicsItem* item, unsigned ms) : ItemAnimation(item, ms) {}
    ~ItemRemover() { doIt(item()); }
};

class NoodleBack : public ItemRemover {
    QPointF mP0, mP1;
    QPointF mSaved;
    bool mReversed;
public:
    enum { kDefaultLength = 1 };
    NoodleBack(NodeConnection* item, unsigned ms=kDefaultLength, bool rev=0) : ItemRemover(item, ms), mReversed(rev) {
        std::tie(mP0, mP1) = item->toScene();
        mSaved = mP1;
        if (mReversed) {
            std::swap(mP0, mP1);
            mSaved = mP0;
        }

        item->baked(mP0, mP1);
        //if (ms == kDefaultLength)
        timeLine()->setDuration(15.f*sqrt((mP0-mP1).manhattanLength())*speed());
    }
    void afterAnimationStep(qreal step) final {
        if (step >= 1.f) { delete this; return; }
        const QPointF& p0 = mReversed ? mP1 : mP0;
        QPointF& p1 = mReversed ? mP0 : mP1;
        p1 = mSaved + (p0 - mSaved)*step;
        p1.setY(p1.y() + (12.f-float(rand())/float(RAND_MAX/24)));
        static_cast<NodeConnection*>(item())->baked(mP0, mP1);
        //if (step > 0.75f) ItemRemover::afterAnimationStep(step);
    }
};

class NodeScene : public QGraphicsScene {
public:
    template <class T>
    void updateConnection(const QGraphicsItem* node, PortValue port, bool all, const T& Op) {
        for (QGraphicsItem* item : items()) {
            if (NodeConnection* NC = qgraphicsitem_cast<NodeConnection*>(item)) {
                if ((NC->output() == node && (all || NC->outputPort() == port)) ||
                    (NC->input() == node && (all || NC->inputPort() == port))) {
                    Op(NC);
                }
            }
        }
    }

    void removeConnection(const QGraphicsItem* dst, PortValue B,
                          bool doUpdate = true) {
        for (QGraphicsItem* item : items()) {
            if (NodeConnection* NC = qgraphicsitem_cast<NodeConnection*>(item)) {
                if (NC->input() == dst && NC->inputPort() == B) {
                    QRectF bounds = item->boundingRect();
                    NoodleBack::create<NoodleBack>(NC);
                    if (doUpdate) update(bounds);
                }
            }
        }
    }

    void addConnection(const QGraphicsItem* src, PortValue A,
                       const QGraphicsItem* dst, PortValue B,
                       bool doUpdate = true) {
        removeConnection(dst, B);
        addItem(new NodeConnection(src, dst, A,B, sSettings.ConnectionWidth,
                                   sSettings.ConnectionColor,
                                   src->zValue() + zEpsilon()));
    }
};

class NodeEditor : public QGraphicsView {
    typedef QGraphicsItem NodeType;
    typedef std::pair<NodeType*, NodePiece> OverItem;
    class Action;

    OverItem mOver;
    std::unique_ptr<Action> mAction;

    struct MovementBegin {
        QPoint Pos;
        const Qt::MouseButton Button;
        MovementBegin(QPoint P={0,0}, Qt::MouseButton B = Qt::LeftButton) : Pos(P), Button(B) {}
        MovementBegin(const QMouseEvent* E) : Pos(E->pos()), Button(E->button()) {}
    };
    class Action {
    protected:
        NodeEditor* mView;
        MovementBegin mStart;
    public:
        Action(NodeEditor* V, MovementBegin S) : mView(V), mStart(S) { mView->clearOver(); }
        virtual ~Action() {}

        virtual void update(const QPoint& delta, const QPoint& P) = 0;
        virtual OverItem over(OverItem& prev, OverItem now) { return prev; }
        virtual bool end(QMouseEvent* E, bool isRelease) { return !E || E->button() == mStart.Button; }
        virtual bool cancel() { return true; }
        void motion(const QPoint& pos) {
            update(mStart.Pos - pos, pos);
            mStart.Pos = pos;
        }
    };
    class Pan : public Action {
        void update(const QPoint& delta, const QPoint& P) final {
            QPointF Xfrm = mView->mapToScene(P) - mView->mapToScene(mStart.Pos);
            mView->translate(Xfrm.x(), Xfrm.y());
        }
    public:
        Pan(NodeEditor* V, MovementBegin B) : Action(V, B) {}
    };
    class Zoom : public Action {
        QMatrix mIdentity;
        const QPointF mPivot;
        float mScale = 1;
        void update(const QPoint& delta, const QPoint& P = QPoint(0,0)) final {
            zoom(delta);
        }
    public:
        static float limit(float inScale, const float curScale, bool andMin = true) {
          float outScale = curScale * inScale;
          if (outScale > 3.f)      return 3.f / curScale;
          else if (andMin && outScale < .1f) return .1f / curScale;
          return inScale;
        }
        void zoom(const QPoint& delta, float S = 1000.f) {
          S = QPointF::dotProduct(delta, delta) / S;
          S = 1.f + ((fabs(delta.y()) < fabs(delta.x()) ? (delta.x() < 0) : (delta.y() > 0)) ? S : -S);
          mScale *= S;
          S = limit(mScale, mIdentity.m11());

          mView->setMatrix(QMatrix(1, 0, 0, 1, -mPivot.x(), -mPivot.y()) *
                           QMatrix(S, 0, 0, S, mPivot.x(), mPivot.y()) *
                           mIdentity);
        }
        Zoom(NodeEditor* V, MovementBegin B) : Action(V, B), mIdentity(V->matrix()),
            mPivot(V->mapToScene(B.Pos)) {}
    };

    class ConnectionCutter : public Action {
        QPainterPath mPath;
        QGraphicsPathItem* mItem;
    public:
        ConnectionCutter(NodeEditor* V, MovementBegin B) : Action(V, B), mPath(V->mapToScene(B.Pos)), mItem(new QGraphicsPathItem) {
            V->scene()->addItem(mItem);
            mItem->setPen(QPen(QBrush(Qt::red, Qt::Dense5Pattern), sSettings.CutWidth));
        }
        ~ConnectionCutter();

        void update(const QPoint& delta, const QPoint& P) final;
        bool cancel() final;
    };

    class InterruptableTool : public Action {
    protected:
        std::unique_ptr<Action> mNavigation;
        bool mCanceled = false;

        virtual void toolUpdate(const QPoint& delta, const QPoint& P) = 0;
        virtual OverItem toolOver(OverItem& prev, OverItem now) { return prev; }

    public:
        InterruptableTool(NodeEditor* V, MovementBegin B = MovementBegin()) :
            Action(V,B) { sEventCompress.restart(); }

        bool interrupted() const { return mNavigation.get(); }

        void update(const QPoint& delta, const QPoint& P) final {
            if (mNavigation) mNavigation->motion(P);
            return toolUpdate(delta,P);
        }

        OverItem over(OverItem& prev, OverItem now) final {
            if (mNavigation) mNavigation->over(prev, now);
            return toolOver(prev, now);
        }

        bool end(QMouseEvent* event, bool isRelease) final {
            if (mNavigation) {
                if (mNavigation->end(event, isRelease))
                    mNavigation.reset();
                return false;
            }
            if (!isRelease) {
                if (event) {
                    assert(mView->mAction.get() == this);
                    mView->mAction.release();
                    if (mView->navigationEvent(event))
                        mNavigation.reset(mView->mAction.release());
                    mView->mAction.reset(this);
                    return mNavigation == nullptr;
                }
                return true;
            }
            return sEventCompress.elapsed() > 100;
        }
        bool cancel() final { return mCanceled = true; }
    };
    class TemporaryNodeConnection : public NodeConnection {
        QPointF A, B;
        QColor mColor = sSettings.SelectionColor;
        float mWidth = sSettings.NewConnectionWidth;
    public:
        TemporaryNodeConnection(const QGraphicsItem* item, bool reverse,
                                QPointF P, qreal Z = zMax())
            : NodeConnection(reverse ? B : A, reverse ? A : B, mWidth, mColor,
                             Z, nullptr, false), A(P) {
          assert(item);
          A = item->mapToScene(P);
          B = A;
          changed();
        }
        void setEnd(QPointF b) { B = std::move(b); }
        void reverse() { std::swap(A,B); }
    };
    class Connector : public InterruptableTool {
        TemporaryNodeConnection *mPath;
        OverItem mNode;

    public:
        Connector(NodeEditor* V, QPointF P, OverItem I) : InterruptableTool(V),
            mNode(I), mPath(new TemporaryNodeConnection(I.first, I.second & kInputTag, P)) {
            mPath->setAcceptHoverEvents(false);
            mView->scene()->addItem(mPath);
            if (0 == mNode.first) {
                mNode.first->setFlag(QGraphicsItem::ItemIsSelectable, false);
                mNode.first->setSelected(false);
            }
            //QGraphicsBlurEffect* blur = new QGraphicsBlurEffect;
            //blur->setBlurRadius(6);
            //blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
            //mPath->setGraphicsEffect(blur);
            sEventCompress.restart();
        }
        ~Connector();
        void toolUpdate(const QPoint& delta, const QPoint& P);
        OverItem toolOver(OverItem& prev, OverItem now);
        bool valid() const { return !interrupted() && !mCanceled; }
    };
    class TextEdit : public InterruptableTool {
        class FocusLocked : public QLineEdit {
        public:
            FocusLocked(const QString& str, QWidget* P) : QLineEdit(str, P) {}
            ~FocusLocked() { hide(); parentWidget()->setFocus(); }
            void focusOutEvent(QFocusEvent* F) { if (isVisible()) setFocus(); }
        };
        FocusLocked mLineEdit;
        QGraphicsItem* mItem;
        const QRectF mPins;
        void toolUpdate(const QPoint& delta, const QPoint& P) final { resize(); }

        void resize() {
            QRectF R;
            R.setTopLeft(mView->mapFromScene(mPins.topLeft()));
            R.setBottomRight(mView->mapFromScene(mPins.bottomRight()));
            mLineEdit.setGeometry(R.left(), R.top(), R.width(), R.height());
            QFont F = mLineEdit.font();
            F.setPixelSize(R.height());
            mLineEdit.setFont(F);
        }
    public:
        TextEdit(NodeEditor* V, QGraphicsItem* I, QRectF R, const QString& str) :
          InterruptableTool(V, MovementBegin()), mLineEdit(str, V), mItem(I),
          mPins(I->mapToScene(R.topLeft()), I->mapToScene(R.bottomRight())) {
            mLineEdit.setAlignment(Qt::AlignCenter);
            resize();
            //mLineEdit.setStyleSheet("background-color: rgba(0,0,0,0);");
            mLineEdit.connect(&mLineEdit, &QLineEdit::returnPressed, [&]() {
              mLineEdit.hide();
              qApp->postEvent(
                  mView->viewport(),
                  new QMouseEvent(QEvent::MouseButtonRelease,
                                  mView->mapFromGlobal(QCursor::pos()),
                                  Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier));
            });
            mLineEdit.setFocusPolicy(Qt::StrongFocus);
            mLineEdit.setFocus();
            mLineEdit.show();
        }
        ~TextEdit();
    };
public:
    NodeEditor(QGraphicsScene& s, QWidget* p = nullptr) : QGraphicsView(&s, p) {
        setInteractive(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        const qreal mn = std::numeric_limits<qreal>::max() * -0.5f,
                    mx = std::numeric_limits<qreal>::max();
        setSceneRect(QRectF(mn, mn, mx, mx));
        setTransformationAnchor(NoAnchor);
        setResizeAnchor(NoAnchor);
        setDragMode(NoDrag);
    }
    ~NodeEditor() { mAction.reset(); }

    bool editable() const { return matrix().m11() > sSettings.ConnectionCutoff; }

    OverItem setOver(NodeType* item, NodePiece over);
    NodePiece over(NodeType* item) const { return mOver.first == item ? mOver.second : NodePiece(0); }

    void clearOver() {
        if (mOver.first) {
            // FIXME: Invalidate the proper rect
            mOver.first->update();
            mOver = {nullptr, 0};
        }
    }

    bool navigationEvent(QMouseEvent *event) {
        if (event->button() == Qt::RightButton)
            mAction.reset(new Zoom(this, event));
        else if (event->button() == Qt::MidButton)
            mAction.reset(new Pan(this, event));
        return mAction.get();
    }

    bool endAction(QMouseEvent* event, bool isRelease) {
        if (!mAction) return true;
        if (mAction->end(event, isRelease)) mAction.reset();
        return false;
    }

    void connectEvent(QPointF P) {
        if (endAction(nullptr, true))
            mAction.reset(new Connector(this, P, mOver));
    }

    void textEdit(QGraphicsItem* I, QRectF R, const QString& str) {
        mAction.reset(new TextEdit(this, I, R, str));
    }

    void mousePressEvent(QMouseEvent *event)  final {
        if (!endAction(event, false))
            return;
        if (event->modifiers() == Qt::MetaModifier) {
            mAction.reset(new ConnectionCutter(this, event));
            return;
        }
        if (navigationEvent(event))
            return;
        setDragMode(QGraphicsView::RubberBandDrag);
        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event)  final {
        if (mAction) {
            mAction->motion(event->pos());
            return;
        }
        return QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event)  final {
        //for (auto *i : items()) printf("%p: %d\n", i, i->isSelected());
        endAction(event, true);
        QGraphicsView::mouseReleaseEvent(event);
    }
    void resizeEvent(QResizeEvent *event) final {
        // QGraphicsView tries to re-center things
        return QAbstractScrollArea::resizeEvent(event);
    }

    void fitInView(QRectF bounds) {
        const QPointF center = bounds.center();
        const QSizeF size = bounds.size();
        if (size.isEmpty()) return;

        const QSize margin(sSettings.Radius*2.f, sSettings.Radius*2.f);
        const QSize vpSize = viewport()->size() - margin;

        float scale = std::min(float(vpSize.width()) / size.width(),
                               float(vpSize.height()) / size.height());
        scale = Zoom::limit(scale, 1, false);
        QMatrix M = QMatrix(1,0,0,1,-center.x(), -center.y()) *
                    QMatrix(scale,0,0,scale,
                            vpSize.width()*0.5f+margin.width()*0.5f,
                            vpSize.height()*0.5f+margin.height()*0.5f);

        MatrixAnimator::create<MatrixAnimator,QGraphicsItem>(nullptr, this, M);
    }

    void fitInView(bool selected) {
        if (selected) {
            if (auto sel = listCheck(scene()->selectedItems())) {
                QRectF R(0,0,0,0);
                for (const auto* item : sel)
                    R |= item->mapRectToScene(item->boundingRect());
                fitInView(R);
                return;
            }
        }
        return fitInView(scene()->itemsBoundingRect());
    }

    void keyPressEvent(QKeyEvent *event) final {
      switch (event->key()) {
          case Qt::Key_F: return fitInView(true);
          case Qt::Key_A: return fitInView(false);

          case Qt::Key_Escape:
              if (mAction && mAction->cancel()) mAction.reset();
          default:
              break;
      }
      QGraphicsView::keyPressEvent(event);
    }
    void wheelEvent(QWheelEvent *event) final {
        const QPoint PD = event->pixelDelta();
        if (!PD.isNull()) Zoom(this, event->pos()).zoom(sSettings.zoomDelta(PD));
    }
};

class NodeView {
    QString mName;

    typedef  std::unordered_set<uint16_t> ConnectedPorts;
    std::vector<QString> mOutputs, mInputs;

public:
    ConnectedPorts mConnectedIn, mConnectedOut;

    NodeView(QString n) : mName(std::move(n)) {
        for (int i = 0, n = rand() % 10; i < n; ++i) mInputs.push_back(QString::asprintf("input%d", i));
        for (int i = 0, n = rand() % 10; i < n; ++i) mOutputs.push_back(QString::asprintf("output%d", i));
    //if (rand() % 2) mOutputs.resize(0);
    //else mInputs.resize(0);
        //for (size_t i = 0, n = numInputs(); i < n; ++i) if(rand()%2) mHiddenIn.insert(i);
        //for (size_t i = 0, n = numOutputs(); i < n; ++i) if(rand()%2) mHiddenOut.insert(i);
    }
	void setName(QString str) { mName.swap(str); }
    const QString& name() const { return mName; }

    const std::vector<QString>& outputs() const { return mOutputs; }
    const std::vector<QString>& inputs() const { return mInputs; }

    size_t numOutputs() const { return mOutputs.size(); }
    size_t numInputs()  const { return mInputs.size(); }

    bool connected(size_t port, uint8_t tag) const {
        return (tag & kInputTag) ? mConnectedIn.count(port) :  mConnectedOut.count(port);
    }
};

class NodeGeometry {
    typedef NodePiece PortID;
    typedef uint16_t port_t;
    typedef std::unordered_map<PortID, std::pair<QPointF*,QRectF*>> PortMap;

    PortMap mPorts;
    std::vector<QRectF>   mBorders;
    QRectF                mBounds;
    unsigned mOverlap : 1;
    unsigned mHasThis : 1;
    unsigned mMinimized : 2;

    enum { kExpanded, kExposeUsed, kCollapsed };

    static const QString* moreLabel() {
        static QString kMore("more");
        return &kMore;
    }

    void dispose() {
        for (auto& port : mPorts) {
            delete port.second.first;
            delete port.second.second;
        }
        mPorts.clear();
    }

    const PortMap::mapped_type& locationForID(PortID id) const {
        id = {id.value(), id & kConnectionTag};
        const auto port = mPorts.find(id);
        if (port != mPorts.end())
            return port->second;
        assert(mMinimized && mPorts.find({kMorePort,id.tag()}) != mPorts.end());
        return mPorts.find({kMorePort,id.tag()})->second;
    }
    static NodePiece inputID(NodePiece::value_type v) { return {v,kInputTag}; }
    static NodePiece outputID(NodePiece::value_type v) { return {v,kOutputTag}; }
public:

    void layout(const NodeView& node) {
        QFontMetrics FM(sSettings.Font);

        const float pHeight = FM.height() + sSettings.VerticalPad;
        const float offset = pHeight * 0.5f;
        const float radius = sSettings.Radius;
        const float pLeft = radius;
        const float iLeft = pLeft + radius;
        const float diamtr = radius * 2.f;
        
        float linePad = diamtr*2.f + sSettings.HorizontalPad + sSettings.RoundRadius*2.f;
        float width = std::max(125.f, FM.width(node.name()) + sSettings.HorizontalPad*2);
        float height = sSettings.TitleHeight();

        const size_t nIn = node.numInputs(), nOut = node.numOutputs(), // numInputPorts(node), nOut = numOutputPorts(node),
                     nInOut = nIn + nOut;

        dispose();
        mBorders.resize(5 + (mOverlap && nIn && nOut));
        mBorders[0] = QRectF(0, 0, 0, height);

        // unordered_map, need to save the first and last label rect
        std::pair<const QRectF*, const QRectF*> firsts(nullptr, nullptr);
        std::pair<const QRectF*, const QRectF*> lasts(nullptr, nullptr);

        auto addPortPos = [&] (uint16_t port, uint8_t tag, QPointF P) -> auto& {
            auto& layout = mPorts[NodePiece(port,tag)];
            if (!layout.first) layout.first = new QPointF;
            *layout.first = std::move(P);
            return layout;
        };

        auto addPort = [&] (uint16_t port, uint8_t tag, float labelX, float lineWidth) {
            auto& layout = addPortPos(port, tag, QPointF(pLeft, offset + height));
            if (!layout.second) layout.second = new QRectF;
            *layout.second = QRectF(labelX, height, 0, pHeight);
            width = std::max(width, lineWidth + FM.width(*label(node, NodePiece(port,tag))));
            height += pHeight;
            
            if (mOverlap) {
              lasts.second = layout.second;
              if (!firsts.second) firsts.second = layout.second;
            }
        };

        auto addPorts = [&] (size_t N, uint8_t tag, float labelX, float lineWidth) {
            if (N == 0) return;

            // Don't bother adding 'more' if there is only one port.
            if (!mMinimized || N == 1) {
                for (port_t port = 0; port < N; ++port)
                    addPort(port, tag, labelX, lineWidth);
                return;
            }
            
            if (mMinimized != kCollapsed) {
                size_t nAdded = 1;
                for (port_t port = 0; port < N; ++port) {
                    if (node.connected(port, tag))
                        ++nAdded;
                    else
                        mPorts.erase({port,tag});
                }
                if (nAdded >= N) {
                    // Remove any stray 'more' port
                    mPorts.erase({kMorePort,tag});
                    for (port_t port = 0; port < N; ++port)
                        addPort(port, tag, labelX, lineWidth);
                    return;
                }
                for (port_t port = 0; port < N; ++port) {
                    if (node.connected(port, tag))
                        addPort(port, tag, labelX, lineWidth);
                }
            }
            addPort(kMorePort, tag, labelX, lineWidth);
        };

        addPorts(nIn, kInputTag, iLeft, linePad);

        // swap into first so output iteration can set second
        std::swap(lasts.first, lasts.second);
        std::swap(firsts.first, firsts.second);

        if (mOverlap) {
            height = sSettings.TitleHeight();
            if (nIn) linePad = width;
        }
        addPorts(nOut, kOutputTag, mOverlap ? 0 : iLeft, linePad);

        if (mOverlap) {
            // Set widths to half the total width, except for ports that are below
            // last element of opposing side.
            const float adjX = (width * 0.5f) + sSettings.HorizontalPad * .5f;
            const float adjWidth = (width * 0.5f) - diamtr - sSettings.HorizontalPad*0.5f;
            const float fullWidth = width - radius*4.f;
            for (auto& port : mPorts) {
                if (!port.second.second) continue;
                if (port.first & kOutputTag) {
                    const bool overlap = lasts.first ? port.second.second->y() <= lasts.first->y() : false;
                    port.second.second->setX(overlap ? adjX : iLeft);
                    port.second.second->setWidth(overlap ? adjWidth : fullWidth);
                } else {
                    const bool overlap = lasts.second ? port.second.second->y() <= lasts.second->y() : false;
                    port.second.second->setWidth(overlap ? adjWidth : fullWidth);
                }
            }
            height = std::max(lasts.first ? lasts.first->bottom() : height,
                              lasts.second ? lasts.second->bottom() : height);
        } else {
            const float adjWidth = width - radius*4.f;
            for (auto& L : mPorts) {
                if (L.second.second) L.second.second->setWidth(adjWidth);
            }
        }

        const float bSize = sSettings.VerticalPad * 2.f;
        mBorders[0].setWidth(width);
        mBorders[4] = QRectF(0, height, width,  bSize);

        const size_t nButtons = 3;
        const float buttonwidth = 10;
        float minButtons = sSettings.Radius * 2.f; //width - (nButtons*(sSettings.HorizontalPad+buttonwidth)) - sSettings.Radius;
        for (size_t i = kMaximizeButton; i <= kMinimizeButton; ++i) {
            mBorders[i] = QRectF(minButtons, height, buttonwidth,  bSize).adjusted(0, 1, 0, -1);
            minButtons += buttonwidth + sSettings.HorizontalPad;
        }
        height += bSize;

        // Overlapping shared center zone.
        if (mOverlap && lasts.first && lasts.second) {
            const float x = firsts.first->right(), y = firsts.first->top();
            mBorders[5] = QRectF(x, y, firsts.second->x()-x,
                                 (nIn < nOut ? lasts.first : lasts.second)->bottom()-y);
        }

        float adjWidth = width - radius;
        for (auto& port : mPorts) {
            if (port.first & kOutputTag)
                port.second.first->setX(adjWidth);
        }

        if (mHasThis) {
            const float y = mBorders.front().center().y();
            const float ox = 0; //diamtr;
            addPortPos(kThisPort, kInputTag,  QPointF(pLeft+ox, y));
            addPortPos(kThisPort, kOutputTag, QPointF(adjWidth-ox, y));
        }
        //if (mMethods) addPortPos(kThisPort, kOutputTag, QPointF(adjWidth-sSettings.Radius, height-sSettings.Radius));

        mBounds = QRectF(0,0,width,height);
    }

    bool layout(const NodeView& node, unsigned what) {
        const unsigned prevLayout = mMinimized;
        switch (what) {
            case kMaximizeButton:  mMinimized = kExpanded;   break;
            case kConnectedButton: mMinimized = kExposeUsed; break;
            case kMinimizeButton:  mMinimized = kCollapsed;  break;
            default: break;
        }
        if (prevLayout != mMinimized) {
            layout(node);
            return true;
        }
        return false;
    }

    NodePiece newPort(PortID idx, NodePiece prev, NodeView& node) {
        (idx & kInputTag ? node.mConnectedIn : node.mConnectedOut).insert(idx.value());
        layout(node);
        return NodePiece(idx.value(), (prev & kConnectionTag) | kSelectConnectionTag);
    }

    typedef PortMap::value_type value_type;

    NodeGeometry(const NodeView& node) {
        mOverlap = rand() % 2;
        mHasThis = rand() % 2;
        mMinimized = rand() % 3;
        //mMinimized = kExpanded;
        //if (mMinimized == kCollapsed) mOverlap = true;

        //mMinimized = kExposeUsed;
        //mHasThis = true;
        //mMinimized = kCollapsed;

        layout(node);
    }
    ~NodeGeometry() { dispose(); }

    const QRectF& bounds() const { return mBounds; }
    unsigned collapsed() const { return mMinimized; }
    bool columns() const { return mOverlap; }
    
    std::pair<const QRectF*,const QRectF*> nodeArea() const { return {&mBorders[0], &mBorders[4]}; }
    const QRectF* buttonArea() const { return &mBorders[1]; }

    Range<PortMap::const_iterator>
    portLocations() const { return {mPorts.begin(), mPorts.end()}; }

    const QPointF& portLocation(NodePiece over) const {
        return *locationForID(over).first;
    }

    size_t additionalConnectors() const { return mHasThis ? 2 : 0; }

    const QString* label(const NodeView& node, PortID port) const {
        if (port.value() == kMorePort)
            return moreLabel();
        if (port & kInputTag)
            return &node.inputs()[port.value()];
        if (port & kOutputTag)
            return &node.outputs()[port.value()];
        return nullptr;
    }

    value_type portGeo(PortID::value_type id, uint8_t tag) const {
        assert(mPorts.find(PortID(id, tag)) != mPorts.end());
        return *mPorts.find(PortID(id, tag));
    }

    struct GeometryIterator {
        size_t NumInputs;
        size_t NumPorts;
    };

    GeometryIterator
    visibleLabels(const NodeView& node) const {
        const size_t Nin = node.numInputs(), Nout = node.numOutputs();
        switch (mMinimized) {
            case kExpanded:   return {Nin, Nin+Nout};
            case kCollapsed:  return {Nin ? 1UL : 0UL, Nin + (Nout ? 1UL : 0UL)};
            case kExposeUsed: break; // if (!node.numInputs() && !node.numOutputs()) return {0,0};
        }
        assert(mMinimized == kExposeUsed);

        GeometryIterator rval = {0,0};
        for (size_t i = 0; i < Nin; ++i)
            if (node.connected(i, kInputTag)) ++rval.NumInputs;

        // Possibly add the 'more' port
        if (rval.NumInputs < Nin)
            ++rval.NumInputs;
        
        rval.NumPorts = rval.NumInputs;
        for (size_t i = 0; i < Nout; ++i)
            if (node.connected(i, kOutputTag)) ++rval.NumPorts;

        if ((rval.NumInputs-rval.NumInputs) < Nout)
            ++rval.NumPorts;

        return rval;
    }

    NodePiece hitTest(QPointF P, const NodeView& node /*, bool centered = true*/) {
        float rTest = sSettings.Radius*1.7f;
        float diameter = sSettings.Radius*2.f;
        const size_t Nin = node.numInputs(), //numInputPorts(node),
                     Nout = node.numOutputs(), //numOutputPorts(node),
                     Nthis = mHasThis ? Nin + Nout : -1;

        if (P.x() < diameter || P.x() > mBounds.right()-diameter) {
        for (const auto& port : mPorts) {
            if ((P-(*port.second.first)).manhattanLength() < rTest)
                return port.first;
        }
        }

        const bool centered = false;
        // Overlapping shared center zone
        if (mOverlap && Nin && Nout) {
            if (mBorders.back().contains(P))
                return { 0, kTitleTag };
        }

        if (P.y() > mBorders[0].bottom() && P.y() < mBorders[1].top()) {
        for (const auto& port : mPorts) {
            if (const QRectF* R = port.second.second) {
                if (R->contains(P)) {
                    if (!centered) {
                        bool area = P.x() > (mBounds.width()*0.5f);
                        if (port.first & kInputTag ? area : !area)
                            return port.first | uint8_t(kAreaTag);
                    }
                    return port.first | uint8_t(kNameTag);
                }
            }
        }
        }

        uint16_t id = 0;
        for (QRectF& R : mBorders) {
            if (R.contains(P)) return { id, kTitleTag };
            ++id;
        }

        return NodePiece(0);
    }

    QRectF area(NodePiece piece, float expand = sSettings.SelBlur) const {
        if (piece & kConnectionTag) {
            
            // FIXME: Tied to closely to what drag-move does, or should kAreaTag
            // mean an undefined region on a specific node's port?
            if (piece & kAreaTag) return QRectF();

            const float rad = sSettings.Radius + expand;
            const float d = rad * 2.f;
            const auto& port = locationForID(piece);
            // Port only requested (or a label doesn't exist for the port)
            if (!(piece & kNameTag) || !port.second) {
                const QPointF* P = port.first;
                return QRectF(P->x()-rad, P->y()-rad, d, d);
            }
            assert(port.second);
            if (piece & kOutputTag)
                return port.second->adjusted(0, 0, d, 0);
            return port.second->adjusted(-d, 0, 0, 0);
        }
        if (piece & kTitleTag)
            return mBorders[piece.value()];
        return QRectF();
    }

    PortValue mapPort(NodePiece over, NodeView& node) {
        if (!mMinimized || !(over & kConnectionTag) || over.value() != kMorePort)
            return over;
        std::vector<std::string> items;
        std::vector<NodePiece::value_type> ports;
        NodePiece::value_type port = 0;
        const uint8_t tag = over & kConnectionTag;
        const bool collapsed = mMinimized == kCollapsed;
        for (auto&& name : tag & kInputTag ?  node.inputs() : node.outputs()) {
            if (collapsed || !node.connected(port, tag)) {
                items.emplace_back(name.toStdString());
                if (!collapsed) ports.emplace_back(port);
            }
            ++port;
        }

       TabMenu connectMenu(items, false);
       auto result = connectMenu.exec("Property", (over & kInputTag ? 1.f : 0.f));
       if (!result.first)
           return NodePiece();

       printf("got: %s\n", result.first->text().toUtf8().data());
       assert(std::find(items.begin(), items.end(), result.first->text().toStdString()) != items.end());
       const size_t idx = std::distance(items.begin(), std::find(items.begin(), items.end(), result.first->text().toStdString()));
       return newPort(NodePiece(collapsed ? idx : ports[idx],over.tag()), over, node);
    }
};

class NodeItem : public QGraphicsItem {
    typedef std::pair<QGraphicsItem*, NodePiece> OverItem;

    NodeView& mNode;
    NodeGeometry mGeometry;

    NodeScene* parentScene() {
        return static_cast<NodeScene*>(scene());
    }
    NodeEditor* parentWidget(QWidget* w) {
        return static_cast<NodeEditor*>(w->parentWidget());
    }
    NodeEditor* parentWidget(QGraphicsSceneEvent* e) {
        return parentWidget(e->widget());
    }

    const NodeGeometry& geometry() const { return mGeometry; }
    NodeGeometry& geometry() { return mGeometry; }

    void reorder(NodeEditor* P = nullptr) {
        static qreal sMax = zMin() + zEpsilon();
        if (zValue() >= sMax) return;
        // FIXME: Rebalnce
        //if (sMax >= (zMax() - zEpsilon()))
        //    Rebalance(scene(), sMax)();

        sMax += zEpsilon();
        setZValue(sMax);
        if (P) {
            auto S = parentScene();
            qreal z = sMax + zEpsilon();
            for (const auto& port : geometry().portLocations())
                S->updateConnection(this, port.first, geometry().collapsed(), [z](QGraphicsItem* C) { C->setZValue(z);});
        }
    }

public:
    NodeItem(NodeView& n, QGraphicsItem* p = nullptr) : QGraphicsItem(p), mNode(n), mGeometry(n) {
        setAcceptHoverEvents(true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
        reorder();
        //setGraphicsEffect(new QGraphicsBlurEffect);

        setFlag(ItemSendsGeometryChanges);
        setFlag(ItemIsSelectable, true);
    }

    enum { Type = kNodeType };
    int type() const final { return Type; }

    const QString& name() { return mNode.name(); }

    QRectF boundingRect() const final {
        if (isSelected()) {
          const float expand = sSettings.SelBlur + sSettings.SelExpansion + fabs(sSettings.SelLine * 0.5f);
          return geometry().bounds().adjusted(-expand, -expand, expand, expand);
        } else if (sSettings.SelBlur) { //&& parentWidget(widget)->over(this) & kConnectionTag) {
          const float expand = sSettings.SelBlur;
          return geometry().bounds().adjusted(-expand, -expand, expand, expand);
        }
        return geometry().bounds();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) final {
        painter->setClipRect(option->exposedRect.marginsAdded(QMarginsF(1,1,1,1)));

        const NodeGeometry& Ng = geometry();
        const float radius = sSettings.Radius;
        const float hPad = sSettings.HorizontalPad;
        QRectF border = Ng.bounds().adjusted(sSettings.RoundRadius, 0, -sSettings.RoundRadius, 0);

        if (isSelected()) {
            const float radius = sSettings.RoundRadius*1.5;
            const float selExpand = sSettings.SelExpansion;
            const float fullExpand = selExpand + sSettings.SelBlur + fabs(sSettings.SelLine * 0.5f);
            const QRectF selRect = border.adjusted(-selExpand, -selExpand, selExpand, selExpand);
            const float lineWidth = fabs(sSettings.SelLine);
            if (sSettings.SelBlur) {
                BlurredImage img(QPointF(fullExpand, fullExpand),
                                 QSize(Ng.bounds().width()+fullExpand*2,
                                       Ng.bounds().height()+fullExpand*2),
                                 widget);
                img->setPen(QPen(QBrush(sSettings.SelectionColor), lineWidth));
                img->setBrush(Qt::NoBrush);
                img->drawRoundedRect(selRect, radius, radius);
                img(painter);
            }
            if (sSettings.SelLine <= 0.f || sSettings.SelBlur <= 0.f) {
                const QPen pen = painter->pen();
                const QBrush brush = painter->brush();
                painter->setPen(QPen(QBrush(sSettings.SelectionColor), lineWidth * (sSettings.SelLine <= 0.f ? 0.25f : 1.f)));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(selRect, radius, radius);
                painter->setPen(pen);
                painter->setBrush(brush);
            }
        }

        //marginsRemoved(QMarginsF(sSettings.RoundRadius, 0, sSettings.RoundRadius, 0));

        const float tHeight = sSettings.TitleHeight() / border.height();
        QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.f*(100.f/border.width()), border.height()));
        QColor colors[4] = { Qt::gray, QColor(20,20,20),
                             QColor(30,30,30),  QColor(58, 58, 58) };
        float weights[4] = { 0, 0.45f*tHeight, 0.97f, 1 };
        for (unsigned i = 0; i < 4; ++i)
            gradient.setColorAt(weights[i], colors[i]);

        painter->setPen(QColor(230,230,230));
        painter->setBrush(gradient);
        painter->drawRoundedRect(border, sSettings.RoundRadius,
                                         sSettings.RoundRadius);

        // Title
        auto nodeArea = Ng.nodeArea();
        if (nodeArea.first) {
          painter->setFont(sSettings.Title);
          painter->drawText(*nodeArea.first, Qt::AlignHCenter|Qt::AlignVCenter,
                            mNode.name());
        }
  
        const int lAlign = Qt::AlignLeft, rAlign = Qt::AlignRight;
        //const int lAlign = Qt::AlignCenter, rAlign = Qt::AlignCenter;

        QPen pen = painter->pen();
        auto portItr = geometry().visibleLabels(mNode);
        const size_t nIn = portItr.NumInputs, nOut = portItr.NumPorts-nIn;

        auto drawPort = [&] (QPainter* painter, const NodeGeometry::value_type& p) {
             const auto& port = p.second;
             if (port.second) {
                 assert(Ng.label(mNode, p.first));
                 const QString* label = Ng.label(mNode, p.first);
                 const int align = p.first & kInputTag ? lAlign : rAlign;
                 const float adj[2] = { align == Qt::AlignLeft  ?  hPad : 0,
                                        align == Qt::AlignRight ? -hPad : 0 };
                 painter->drawText(port.second->adjusted(adj[0], 0, adj[1], 0),
                                   align|Qt::AlignVCenter, *label);
                                   //QString::fromStdString(*label));
             }
             painter->drawEllipse(*port.first, radius, radius);
        };
        auto drawLine = [&] (const float y) {
            painter->drawLine(border.x()+1.f, y, border.right()-1.f, y);
        };

        if (portItr.NumPorts) {
        // Sub-section / Lines
        if (Ng.columns() || parentWidget(widget)->matrix().m11() < 0.25f) {
            painter->setPen(QPen(QColor(60, 60, 60, 60)));
            for (auto& port : Ng.portLocations()) {
                if (port.second.second)
                    drawLine(port.second.second->bottom());
            }
            painter->setPen(Qt::gray);
        } else if (1) {
            painter->setPen(Qt::NoPen);
            for (unsigned i = 0; i < 3; ++i) gradient.setColorAt(float(i)*0.25f, colors[i]);
            // painter->setBrush(gradient);
            for (auto& port : Ng.portLocations()) {
                if (!port.second.second) continue;
                const QRectF& R = *port.second.second;
                std::pair<float,float> Pos = std::make_pair(R.top(), R.bottom());
                if (port.first & kInputTag) std::swap(Pos.first, Pos.second);
                gradient.setStart(QPointF(0,Pos.first));
                gradient.setFinalStop(QPointF(0,Pos.second));
                painter->setBrush(gradient);
                painter->drawRect(R.adjusted(-hPad+0.5f, 0, hPad-0.5f, 0));
            }

            painter->setBrush(Qt::SolidPattern);
            painter->setPen(Qt::gray);
            if (!Ng.columns() && nIn && nOut) {
                // Separate inputs and outputs with a line
                //assert(Ng.portGeo(0, kOutputTag).second.second);
                //drawLine(Ng.portGeo(0, kOutputTag).second.second->top());
            }
        }
        }

#if 0
{
        size_t i = 0;
        while (i < portItr.NumInputs) painter->fillRect(*Ng.portGeo(mNode, i++, portItr).Rect, QBrush(Qt::green));
        i = portItr.NumInputs;
        while (i < portItr.NumPorts)  painter->fillRect(*Ng.portGeo(mNode, i++, portItr).Rect, QBrush(Qt::blue));
        //return;
}
#endif

        // Port area.
        //pen.setWidth(0.8f);
        //pen.setColor(Qt::gray);
        //Qt::graypainter->setPen(pen);
        painter->setPen(Qt::gray);
        if (nodeArea.first) {
            /*
            constexpr uint8_t drawBox = 0;
            if (drawBox && Ng.columns() && nIn && nOut) {
                // Center-line between in/out truncated after last row with overlap
                const auto port = Ng.portGeo(mNode, std::min(nIn,nOut)-1,portItr);
                assert(port.Rect);
                const float x1 = border.center().x()-1;
                const float y1 = port.Rect->bottom();
                painter->drawLine(x1, nodeArea.first->bottom(), x1, y1);
                if (drawBox > 1 && nIn != nOut) {
                  float x0 = nIn < nOut ? border.left()+1.f : border.right()-1.f;
                  painter->drawLine(x0, y1, x1, y1);
                }
            }
            */
            drawLine(nodeArea.first->bottom());
        }
        if (nodeArea.second)
            drawLine(nodeArea.second->top());

        auto over = parentWidget(widget)->over(this);

        if (const QRectF* buttons = Ng.buttonArea()) {
            auto drawButton = [] (QPainter* painter, const QRectF& R, size_t nLines) {
              const float space = R.height() * 0.25f;
              float y = R.top();
              for (unsigned j = 0; j < nLines; ++j) {
                  y += space;
                  painter->drawLine(R.left()+1, y, R.right()-1, y);
              }
            };
            size_t nLines = 3;
            for (size_t i = 0; i < 3; ++i) {
                if (Ng.collapsed() != i) drawButton(painter, buttons[i], nLines);
                --nLines;
            }
            if (over && over & kTitleTag && ((over.value() - kMaximizeButton) != Ng.collapsed())
                && over.value() >= kMaximizeButton && over.value() <= kMinimizeButton) {
                const QRectF& R = buttons[over.value()-kMaximizeButton];
                BlurredImage img(-R.topLeft(), QSize(R.width(), R.height()), widget);
                img->setPen(sSettings.SelectionColor);
                img->setBrush(Qt::NoBrush);
                drawButton(img, R, 3 - (over.value()-kMaximizeButton));
                img(painter);
            }
        }

        size_t i = 0;
        const size_t Nconnect = portItr.NumPorts + Ng.additionalConnectors();
        if (!Nconnect) return;

        // Ports / Connectors
        painter->setFont(sSettings.Font);
        pen.setWidth(1);
        painter->setPen(pen);
        //painter->setBrush(gradient);

#if 1
        for (auto& port : Ng.portLocations())
            drawPort(painter, port);
#else
        while (i < portItr.NumPorts)
            drawPort(painter, i++);

        painter->setBrush(colors[2]);
        while (i < Nconnect) {
            const QPointF& P = Ng.portGeo(mNode, i)
            painter->setPen(Qt::NoPen);
            drawPort(painter, i);
            painter->setPen(pen);
            int aMult = 1, offx = 1;
            if (&P == &Ng.mPorts.back()) {
                aMult = 3;
                offx = 0;
            }
            painter->drawArc(P.x()-radius-offx, P.y()-radius, radius*2.f+1.f, radius*2.f+1.f, 90*16 * aMult, 90*16*2);
            //drawPort(painter, QRectF(), Ng.mPorts[i]);
            ++i;
        }
#endif

        if (over) {
            if (const uint8_t tag = (over & kConnectionTag)) {
                if (sSettings.SelBlur) {
                    const QRectF area = Ng.area(over);
                    BlurredImage img(-area.topLeft(), QSize(area.width(), area.height()), widget);
                    img->setFont(sSettings.Font);
                    const QBrush brush = QBrush(sSettings.SelectionColor, Qt::SolidPattern);
                    img->setPen(QPen(brush, fabs(sSettings.SelLine)));
                    img->setBrush(sSettings.SelLine < 0 ? Qt::NoBrush : brush);
                    drawPort(img, Ng.portGeo(over.value(), tag));
                    img(painter, sSettings.SelBlur);
                } else {
                    painter->setPen(Qt::NoPen);
                    painter->setPen(sSettings.SelectionColor);
                    painter->setBrush(QBrush(sSettings.SelectionColor, Qt::SolidPattern));
                    drawPort(painter, Ng.portGeo(over.value(), tag));
                }
            }
        }
    }

    void updateLocations() {
        NodeScene* scene = parentScene();
        for (const auto& port : geometry().portLocations())
            scene->updateConnection(this, port.first, geometry().collapsed(), [](NodeConnection* C) { C->changed();});
    }

    const QPointF& location(NodePiece over) const {
        return geometry().portLocation(over);
    }

    void update(NodePiece piece) {
        QGraphicsItem::update(geometry().area(piece));
    }

    static const QPointF& location(const OverItem& over) {
        return static_cast<NodeItem*>(over.first)->location(over.second);
    }

    void setOver(NodeEditor* Parent, const QPointF& P, QPointF* connection = 0) {
        NodePiece over = connection || Parent->editable() ?
                         geometry().hitTest(P, mNode) : NodePiece(0,kTitleTag);
        if (connection) {
            // Requesting a connection location, the over must be input/output
            if (over & kConnectionTag)
                *connection = location({this,over});
            else
                over = {0,0};
        } else if (over & kAreaTag)
            over = {kTitleArea, kTitleTag};
        Parent->setOver(this, over);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) final {
        switch (change) {
          case ItemPositionChange:
            return QGraphicsItem::itemChange(change, snapPoint(value.toPointF(), sSettings.Snap));
          case ItemPositionHasChanged:
            updateLocations();
            break;
          case ItemSelectedChange:
            prepareGeometryChange();
            break;
          default: break;
        };
        return QGraphicsItem::itemChange(change, value);
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) final {
        QGraphicsItem::hoverEnterEvent(event);
        setOver(parentWidget(event), event->pos());
    }
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) final {
        QGraphicsItem::hoverMoveEvent(event);
        setOver(parentWidget(event), event->pos());
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) final {
        QGraphicsItem::hoverLeaveEvent(event);
        parentWidget(event)->setOver(this, 0);
    }

    PortValue mapPort(NodePiece port) {
        const PortValue nport = geometry().mapPort(port, mNode);
        if (port != nport) updateLocations();
        return nport;
    }

    void layout() {
        prepareGeometryChange();
        geometry().layout(mNode);
        updateLocations();
    }
    NodeView& node() { return mNode; }

    void mousePressEvent(QGraphicsSceneMouseEvent *event)  final {
        auto Parent = parentWidget(event);
        reorder(Parent);
        //auto over = setOver(Parent, event->pos());
        auto port = mapPort(Parent->over(this));
        if (!((port & kConnectionTag) || port.value()))
            return QGraphicsItem::mousePressEvent(event);
        Parent->setOver(this, port);
        if (port & kConnectionTag)
            Parent->connectEvent(location(port));
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)  final {
        if (parentWidget(event)->over(this).tag() >= kTitleTag) {
            QGraphicsItem::mouseMoveEvent(event);
            updateLocations();
        }
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) final {
        auto piece = parentWidget(event)->over(this);
        QGraphicsItem::mouseReleaseEvent(event);
        updateLocations();
        if (piece & kTitleTag) {
            prepareGeometryChange();
            if (geometry().layout(mNode, piece.value()))
                updateLocations();
        }
        //Parent->connectEvent(P(over), false);
    }
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) final {
        auto piece = geometry().hitTest(event->pos(), mNode);
        if ((piece & kTitleTag) && piece.value() == 0) {
            const float inset = sSettings.HorizontalPad+sSettings.Radius*2.f;
            parentWidget(event)->textEdit(this, geometry().area(piece).adjusted(inset, 2, -inset, -2), mNode.name());
            //return;
        }
        QGraphicsItem::mouseDoubleClickEvent(event);
    }
};

std::pair<QPointF, QPointF> NodeConnection::toScene() const {
    if (!mBound) return { *mP0, *mP1 };
    return { mPort0.item->mapToScene(static_cast<const NodeItem*>(mPort0.item)->location(mPort0.port)),
             mPort1.item->mapToScene(static_cast<const NodeItem*>(mPort1.item)->location(mPort1.port)) };
}

NodeEditor::OverItem NodeEditor::setOver(NodeType* item, NodePiece over) {
    //printf("setOver: %d : %d\n", over.value(), over.tag());
    if ((mOver.first != item) || (mOver.second != over)) {
        if (mOver.first) static_cast<NodeItem*>(mOver.first)->update(mOver.second);
        if (item) static_cast<NodeItem*>(item)->update(over);
    }
    if (mAction)
        return mAction->over(mOver, OverItem(item,over));
    OverItem previous = OverItem(item, over);
    std::swap(mOver, previous);
    return previous;
}

void NodeEditor::Connector::toolUpdate(const QPoint& delta, const QPoint& P) {
    const QPointF Ps = mView->mapToScene(P);
    mPath->setEnd(Ps);
    for (QGraphicsItem* I : mView->items(P)) {
        if (I != mPath && I != mNode.first) {
            QPointF Pcon;
            if (NodeItem* NI = qgraphicsitem_cast<NodeItem*>(I)) {
                NI->setOver(mView, NI->mapFromScene(Ps), &Pcon);
                if (mView->mOver.second) mPath->setEnd(NI->mapToScene(Pcon));
                mPath->changed();
                return;
            }
        }
    }
    mPath->changed();
    mView->clearOver();
}

NodeEditor::OverItem NodeEditor::Connector::toolOver(OverItem& prev, OverItem now) {
    uint8_t Ctype = now.second & kConnectionTag;
    if (Ctype && Ctype != (mNode.second & kConnectionTag)) {
        // Make the over hilite include the label
        Ctype |= kNameTag;
        prev = OverItem(now.first, {now.second.value(), Ctype});
    } else
        prev = OverItem();

    return prev;
}

NodeEditor::Connector::~Connector() {
    NodeScene* scene = static_cast<NodeScene*>(mView->scene());
    if (!scene) return;
    bool reverse = mNode.second & kInputTag;
    bool isValid = valid();
    // Check for cancel
    if (isValid) {
        // Map from 'more' selection to actual port.
        // Do this now so test for Ctype below includes menu canceled.
        const bool moreChoice = mView->mOver.second.value() == kMorePort;
        if (moreChoice) {
            // Save the item, in case the menu tracking changes it.
            auto over = static_cast<NodeItem*>(mView->mOver.first);
            auto port = over->mapPort(mView->mOver.second);
            mView->mOver = {over, port};
            // if (!port) over->layout();
        }

        const uint8_t Ctype = mView->mOver.second & kConnectionTag;
        if (Ctype) {
            scene->removeItem(mPath); mPath = nullptr;
            OverItem* nodes[2] = { &mNode, &mView->mOver };
            if (reverse) std::swap(nodes[0], nodes[1]);
            scene->addConnection(nodes[0]->first, nodes[0]->second.reduce(kConnectionTag),
                                 nodes[1]->first, nodes[1]->second.reduce(kConnectionTag));
            (nodes[0]->second & kInputTag ?  static_cast<NodeItem*>(nodes[0]->first)->node().mConnectedIn :  static_cast<NodeItem*>(nodes[0]->first)->node().mConnectedOut).insert(nodes[0]->second.value());
            (nodes[1]->second & kInputTag ?  static_cast<NodeItem*>(nodes[1]->first)->node().mConnectedIn :  static_cast<NodeItem*>(nodes[1]->first)->node().mConnectedOut).insert(nodes[1]->second.value());
        } else {
            isValid = false;
            if (reverse) {
                // Snap back to source, not mouse
                mPath->reverse();
                scene->removeConnection(mNode.first, mNode.second.reduce(kConnectionTag));
            }
            (mNode.second & kInputTag ?  static_cast<NodeItem*>(mNode.first)->node().mConnectedIn :  static_cast<NodeItem*>(mNode.first)->node().mConnectedOut).erase(mNode.second.value());
            static_cast<NodeItem*>(mNode.first)->layout();
        }
    }
    if (!isValid && (mNode.second & kSelectConnectionTag)) {
        (mNode.second & kInputTag ?  static_cast<NodeItem*>(mNode.first)->node().mConnectedIn :  static_cast<NodeItem*>(mNode.first)->node().mConnectedOut).erase(mNode.second.value());
        static_cast<NodeItem*>(mNode.first)->layout();
    }
    if (!isValid && (mView->mOver.second & kSelectConnectionTag)) {
        (mView->mOver.second & kInputTag ?  static_cast<NodeItem*>(mView->mOver.first)->node().mConnectedIn :  static_cast<NodeItem*>(mView->mOver.first)->node().mConnectedOut).erase(mView->mOver.second.value());
        static_cast<NodeItem*>(mView->mOver.first)->layout();
    }

    if (mPath) NoodleBack::create<NoodleBack>(mPath, reverse);
    mView->clearOver();
}

bool NodeEditor::ConnectionCutter::cancel() {
    for (QGraphicsItem* item : mView->scene()->items(QPainterPathStroker(QPen(Qt::SolidPattern, sSettings.CutWidth)).createStroke(mPath))) {
        if (NodeConnection* NC = qgraphicsitem_cast<NodeConnection*>(item))
            NC->setPen();
    }
    mPath = QPainterPath();
    return true;
}

void NodeEditor::ConnectionCutter::update(const QPoint& delta, const QPoint& P) {
    if (mPath.elementCount() < 1)
        return;
    mPath.lineTo(mView->mapToScene(P));
    mItem->setPath(mPath);

    // Update color for all connections that intersect with last line segement
    const int N = mPath.elementCount();
    QPainterPath path(mPath.elementAt(N-2));
    path.lineTo(mPath.elementAt(N-1));
    const QPen pen = QPen(QBrush(Qt::red), sSettings.ConnectionWidth, Qt::DotLine);
    for (QGraphicsItem* item : mView->scene()->items(QPainterPathStroker(QPen(Qt::SolidPattern, sSettings.CutWidth)).createStroke(path))) {
        if (NodeConnection* NC = qgraphicsitem_cast<NodeConnection*>(item))
            NC->QGraphicsPathItem::setPen(pen);
    }
}

NodeEditor::ConnectionCutter::~ConnectionCutter() {
    NodeScene* scene = static_cast<NodeScene*>(mView->scene());
    ItemRemover::create<ItemRemover>(mItem);
    if (mPath.elementCount() < 2)
        return;

    for (QGraphicsItem* item : mView->scene()->items(QPainterPathStroker(QPen(Qt::SolidPattern, sSettings.CutWidth)).createStroke(mPath))) {
        if (NodeConnection* NC = qgraphicsitem_cast<NodeConnection*>(item)) {
            NodeItem* item = const_cast<NodeItem*>(static_cast<const NodeItem*>(NC->input()));
            item->node().mConnectedIn.erase(NC->inputPort().value());
            item->layout();
            item = const_cast<NodeItem*>(static_cast<const NodeItem*>(NC->output()));
            item->node().mConnectedOut.erase(NC->outputPort().value());
            item->layout();
            scene->removeConnection(NC->input(), NC->inputPort());
        }
    }
}

NodeEditor::TextEdit::~TextEdit() {
	if (!mCanceled && mItem) {
        static_cast<NodeItem*>(mItem)->node().setName(mLineEdit.text());
        mItem->update();
    }
}

class Window : public QMainWindow {
    std::unique_ptr<TabMenu> mTabMenu;
    std::map<std::string, std::vector<std::string>> mItems;

    std::unordered_set<QGraphicsScene*> mNodeScenes;
    std::list<std::unique_ptr<NodeView>> mNodes;

public:
    Window(QWidget* parent = nullptr) : QMainWindow(parent) {
        mItems["Polygon"].push_back("PolyBevel");
        mItems["Polygon"].push_back("PolyBridge");
        mItems["Polygon"].push_back("PolyCur");
        mItems["Polygon"].push_back("PolyExtrude");
        mItems["Polygon"].push_back("PolyFacet");
        mItems["Polygon"].push_back("PolySlice");

        mItems["Edge"].push_back("Blast");
        mItems["Edge"].push_back("Collapse");
        mItems["Edge"].push_back("Cusp");
        mItems["Edge"].push_back("Divide");

        mItems["Export"].push_back("File");
        mItems["Export"].push_back("Source");
        mItems["Export"].push_back("Bake ODE");
        mItems["Export"].push_back("Geometry");
    }

    virtual void keyPressEvent(QKeyEvent *event) {
      if (event->key()!=Qt::Key_Tab)
          return QMainWindow::keyPressEvent(event);
      if (!mTabMenu) {
          if (mItems.empty()) return;
          mTabMenu.reset(new TabMenu(mItems, this));
/*
          std::vector<std::string> items;
          items.push_back("A");
          items.push_back("B");
          items.push_back("C");
          items.push_back("D");
          items.push_back("E");
          mTabMenu.reset(new TabMenu(items, this));
*/
      }
      auto Cmd = mTabMenu->exec();
      if (Cmd.first) {
          for (QGraphicsScene* s : mNodeScenes) {
              mNodes.emplace_back(new NodeView(Cmd.first->text()));
              NodeItem* item = new NodeItem(*mNodes.back());
              s->addItem(item);

              QGraphicsView* NE = qobject_cast<QGraphicsView*>(Cmd.second.first);
              while (!NE && Cmd.second.first) {
                  Cmd.second.first = Cmd.second.first->parentWidget();
                  NE = qobject_cast<QGraphicsView*>(Cmd.second.first);
              }
              if (NE) {
                  const QSizeF S = item->boundingRect().size()*0.5f;
                  item->setPos(NE->mapToScene(Cmd.second.second) - QPointF(S.width(), S.height()));
                  return;
              }
          }
      }
    }
    void addScene(QGraphicsScene& scene) {  mNodeScenes.insert(&scene); }
    void removeScene(QGraphicsScene& s) { mNodeScenes.erase(&s); }
};

int main(int argc, const char** argv) {
    TaggedInteger<uint16_t> ti(std::numeric_limits<uint16_t>::max(), 3);
    printf("hash: %zx %lu %d %d  %d\n", std::hash<NodePiece>()(ti), sizeof(uint16_t), ti.value(), ti.tag(), std::numeric_limits<uint16_t>::max());

    ti = {32,0};
    printf("ti.0: %u\n", unsigned(ti[0]));
    printf("ti.1: %u\n", unsigned(ti[1]));
    printf("ti.2: %u\n", unsigned(ti[2]));
    ti = {64,1};
    printf("ti.0: %u\n", unsigned(ti[0]));
    printf("ti.1: %u\n", unsigned(ti[1]));
    printf("ti.2: %u\n", unsigned(ti[2]));
    ti = {128,15};
    printf("ti.0: %u\n", unsigned(ti[0]));
    printf("ti.1: %u\n", unsigned(ti[1]));
    printf("ti.2: %u\n", unsigned(ti[2]));
    printf("ti.255: %u\n", unsigned(ti[15]));

    QApplication app(argc, (char**)argv, 0);
    sSettings.init(app);

    Window mainWindow;

    NodeScene scene;
    mainWindow.addScene(scene);

    mainWindow.resize(1280, 720);
    mainWindow.setTabPosition(Qt::TopDockWidgetArea, QTabWidget::North);
    mainWindow.setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
    mainWindow.setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    mainWindow.setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
    mainWindow.setAnimated(true);

    QDockWidget* docks[3];
    Stageview* stage = new Stageview;

    docks[0] = new QDockWidget(&mainWindow);
    docks[0]->setWidget(stage);
    mainWindow.addDockWidget(Qt::LeftDockWidgetArea, docks[0]);

    docks[1] = new QDockWidget(&mainWindow);
    docks[1]->setWidget(new NodeEditor(scene));
    mainWindow.splitDockWidget(docks[0], docks[1], Qt::Vertical);

    docks[2] = new QDockWidget(&mainWindow);
    docks[2]->setWidget(new QWidget);
    //mainWindow.splitDockWidget(docks[0], docks[1], Qt::Vertical);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, docks[2]);

    mainWindow.show();
    stage->hide();
    stage->show();
    app.exec();

    return 0;
}

/*
    class Rebalance {
        QList<QGraphicsItem*> mNodes;
        std::unordered_set<QGraphicsItem*> mDone;
        qreal& mOutMax;
        const qreal mOffset;

        void recurse(QGraphicsItem* current, QList<QGraphicsItem*> items) {
            std::pair<qreal,qreal> range;
            qreal priorZ = current->zValue();
            qreal currentZ = min() + (priorZ - mOffset);
            QList<QGraphicsItem*>::iterator itr = items.begin();
            while (itr != items.end()) {
                QGraphicsItem* node = *itr;
                if (mDone.count(node)) continue;

                mDone.insert(node);
                node->setZValue(currentZ);

                auto overlap = node->collidingItems(Qt::IntersectsItemBoundingRect);
                if (!overlap.empty()) {
                    //overlap.push_back(current);
                    std::sort(overlap.begin(), overlap.end(),
                              [](QGraphicsItem* a, QGraphicsItem* b) -> bool {
                                return a->zValue() < b->zValue();
                              });
                    QList<QGraphicsItem*>::iterator lastBelow = overlap.begin();
                    qreal nAbove = 0, nBelow = 0;
                    for (auto itr2 = lastBelow, end = overlap.end(); itr2 != end; ++itr2) {
                        QGraphicsItem* itm = *itr2;
                        if (itm->zValue() > priorZ) {
                            nAbove += 1;
                            itm->setZValue(currentZ + zEpsilon()*nAbove);
                        } else {
                            nBelow += 1;
                            lastBelow = itr2;
                        }
                    }

                    range.first = std::min(range.first, currentZ - zEpsilon()*nBelow);
                    range.second = std::max(range.second, currentZ + zEpsilon()*nAbove);

                    for (auto itr2 = overlap.begin(); itr2 != lastBelow; ++itr2) {
                        (*itr2)->setZValue(currentZ - zEpsilon()*nBelow);
                        nBelow -= 1;
                    }

                    recurse(node, overlap);
                }
                ++itr;
            }
            current->setZValue(currentZ-mOffset);
        }
    public:
        Rebalance(QGraphicsScene* s, qreal& mx) : mNodes(s->items(Qt::AscendingOrder)),
            mOutMax(mx), mOffset(mNodes.empty() ? 0 : mNodes.front()->zValue()) {}

        void operator () () {
            if (mNodes.empty()) return;
            QGraphicsItem* node = mNodes.front();
            recurse(node, std::move(mNodes));
        }
    };
*/

