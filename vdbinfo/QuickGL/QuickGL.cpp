

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>
#include <Qt3DInput/QMouseEvent>
#include <QtOpenGL/QGLFormat>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "Camera.h"
#include "CameraController.h"

extern "C" void loadVDB(const char *filename);
extern "C" void drawVDBCube();

using namespace koala;
static Camera sCamera(Vector3(8, 8, 8));
static CameraController sCamController(&sCamera);
static Matrix4 sModelMatrix = Matrix4::kIdentity;

void CameraController::glSetup() const
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    const Box2f    &screenWindow = mCamera->screenWindow();
    const Vector2f &cp = mCamera->clippingPlanes();
    if ( mCamera->perspective() )
    {
        const float_t r = cp[0] * tan( M_PI * fov() / 360.0 );
        ::glFrustum( r * screenWindow.min.x, r * screenWindow.max.x, r * screenWindow.min.y, r * screenWindow.max.y, cp[0], cp[1] );
    }
    else
        ::glOrtho(screenWindow.min.x, screenWindow.max.x, screenWindow.min.y, screenWindow.max.y, cp[0], cp[1]);
 

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf( transform().inverse() );
}

class StdOut {
    const int backup;
    int pipefd[2];

    void addFlags(int fd, int add) {
        int flags = fcntl(fd, F_GETFL);
        flags |= add;
        if (fcntl(fd, F_SETFL, flags))
            perror("fcntl");
    }
public:
    StdOut() : backup(dup(fileno(stdout))) {
        // pipe2(pipefd, O_NONBLOCK);
        pipe(pipefd);
        addFlags(pipefd[0], O_NONBLOCK); //  | O_ASYNC);
        //addFlags(pipefd[1], O_NONBLOCK);

        // What used to be stdout will now go to the pipe.
        dup2(pipefd[1], fileno(stdout));
    }
    ~StdOut() {
        close(pipefd[0]);
        close(pipefd[1]);
        //restore
        dup2(backup, fileno(stdout));
    }

    std::string read() {
        std::string result;
        char buf[2];
        int res = ::read(pipefd[0], buf, sizeof(buf));
        while (res > 0) {
            result.append(buf, res);
            res = ::read(pipefd[0], buf, sizeof(buf));
        }
        if (res == -1) {
            if (errno != EAGAIN)
                perror("read");
        }
        return result;
    }
};

static StdOut stdOut;

static void drawGrid(int scale = 20) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glLineWidth(0.05);
    glBegin(GL_LINES);
        glColor4f(0.8, 0.8, 0.8, 0.5);
        for (int i = scale; i >= 1; --i) {
            glVertex3i(-i, 0, -scale);
            glVertex3i(-i, 0,  scale);
            glVertex3i( i, 0, -scale);
            glVertex3i( i, 0,  scale);
            glVertex3i(-scale, 0,  i);
            glVertex3i( scale, 0,  i);
            glVertex3i(-scale, 0, -i);
            glVertex3i( scale, 0, -i);
        }
    glEnd();

    glLineWidth(3);
    glBegin(GL_LINES);
        glColor3f(0, 0, 1);
        glVertex3i(0, 0, -scale);
        glVertex3i(0, 0,  scale);
        glColor3f(1, 0, 0);
        glVertex3i(-scale, 0, 0);
        glVertex3i( scale, 0, 0);
    glEnd();
}

class GLWidget : public QGLWidget
{
    //Q_OBJECT

public:
    GLWidget(QWidget *parent = nullptr)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
    }

    ~GLWidget() {
    }

    QSize minimumSizeHint() const override {
        return QSize(50, 50);
    }

    QSize sizeHint() const override {
        return QSize(400, 400);
    }

protected:
    void initializeGL() override {
        qglClearColor(QColor::fromCmykF(0.39, 0.39, 0.0, 0.0).dark());

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glShadeModel(GL_SMOOTH);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_MULTISAMPLE);
        static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sCamController.glSetup();

        drawGrid();

        glEnable(GL_DEPTH_TEST);
        drawVDBCube();

        glEnable(GL_LIGHTING);
        glEnable(GL_CULL_FACE);
    }

    void resizeGL(int width, int height) override {
        glViewport(0, 0, width, height);
        sCamController.viewPort(width, height);
        sCamera.resetScreen(Vector2i(width, height));
    }

    void mousePressEvent(QMouseEvent *event) override {
        Vector2i mouseLoc(event->x(), event->y());
        switch (event->button()) {
            case Qt::LeftButton:
                if (event->modifiers() & Qt::Modifier::ALT)
                    sCamController.motionStart(CameraController::Tumble, mouseLoc);
                if (event->modifiers() != Qt::Modifier::CTRL)
                    break;
            case Qt::MiddleButton:
                if (event->modifiers() & Qt::Modifier::ALT ||
                    event->modifiers() & Qt::Modifier::CTRL)
                sCamController.motionStart(CameraController::Track, mouseLoc);
                break;
            case Qt::RightButton:
                if (event->modifiers() & Qt::Modifier::ALT ||
                    event->modifiers() & Qt::Modifier::CTRL)
                    sCamController.motionStart(CameraController::Dolly, mouseLoc);
                break;
        }
        updateGL();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        Vector2i mouseLoc(event->x(), event->y());
        sCamController.motionUpdate(mouseLoc);
        updateGL();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        Vector2i mouseLoc(event->x(), event->y());
        sCamController.motionEnd(mouseLoc);
        updateGL();
    }

    virtual void wheelEvent(QWheelEvent *event) override {
        auto delta = event->angleDelta();
        Vector2i mouseLoc(event->x(), event->y());
        sCamController.motionStart(CameraController::Dolly, mouseLoc);
        mouseLoc += Vector2i(delta.x(), delta.y());
        sCamController.motionUpdate(mouseLoc);
        sCamController.motionEnd(mouseLoc);
        update();
    }
};

class Splitter : public QSplitter {
    int collapsedFrom;

public:
    Splitter(QWidget* parent) : QSplitter(parent) {
    }

protected:
    bool eventFilter(QObject * obj, QEvent * event) override {

        if (event->type()==QEvent::MouseButtonDblClick) {
            mouseDoubleClickEvent(static_cast<QMouseEvent*>(event));
            true;
        }
        return false;
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override {
        QList<int> sizes = QSplitter::sizes();
        if (!collapsedFrom) {
            collapsedFrom = sizes[1];
            sizes[0] += sizes[1];
            sizes[1] = 0;
        } else {
            sizes[0] -= collapsedFrom;
            sizes[1] = collapsedFrom;
            collapsedFrom = 0;
        }
        QSplitter::setSizes(sizes);
    }
};

class Window : public QWidget
{
public:
    Window()
    {
        glView = new GLWidget(this);
        textView = new QTextEdit(this);
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        QSplitter *splitter = new Splitter(this);
        splitter->addWidget(glView);
        splitter->addWidget(textView);
        splitter->handle(1)->installEventFilter(splitter);
        mainLayout->addWidget(splitter);
        setLayout(mainLayout);

        textView->setFocusPolicy(Qt::ClickFocus);
        glView->setFocus(Qt::MouseFocusReason);
    }


protected:

    void append(QString append = {}) {
        if (!append.isEmpty()) {
            append = "\n================ " + append +
                     " ================\n";
        }
        const auto str = stdOut.read();
        if (!str.empty()) {
            append += str.c_str();
            textView->append(append);
        }
    }

    void keyPressEvent(QKeyEvent *e)
    {
        if (e->modifiers() == Qt::Modifier::CTRL) {
            switch (e->key()) {
                case Qt::Key_Open:
                case Qt::Key_O: {
                    QFileDialog fd(this);
                    QString filename = QFileDialog::getOpenFileName(this, "Open VDB file");
                    if (!filename.isEmpty()) {
                        loadVDB(filename.toUtf8().data());
                        append(std::move(filename));
                        glView->updateGL();
                    }
                    return;
                }
            }
        } else {
/*
    if ( c == 'f' )
      sCamController.frame(Box3(sModelMatrix * Vector3(-1,-1,-1), sModelMatrix *Vector3(1,1,1)));
    else if (c == 'p') sCamera.perspective() = !sCamera.perspective();
    else if (c == '+') sCamera.fov() += 1.f;
    else if (c == '-') sCamera.fov() -= 1.f;
    else if (c == 'm') sDrawGate = !sDrawGate;
    else if (c == 's') sSample += 1;
    else if (c == 'a') sSample -= 1;
    else if (c == 'c') glEnable( !glIsEnabled(GL_CULL_FACE) );
    else if (c == 'r')
    {
        const Vector2i &res = sCamera.resolution();
        sCamera.resolution(res.y, res.x);
        sCamMask(sCamController);
    }
*/
            switch (e->key()) {
                case Qt::Key_Escape:
                    close();
                    return;

                case Qt::Key_Equal:
                case Qt::Key_Plus:
                    sCamera.fov() += 1.f;
                    glView->updateGL();
                    return;

                case Qt::Key_Minus:
                    sCamera.fov() -= 1.f;
                    if (sCamera.fov() <= 0.f)
                        sCamera.fov() = 1.f;
                    glView->updateGL();
                    return;

                case Qt::Key_H:
                case Qt::Key_F:
                    sCamController.frame(Box3(sModelMatrix * Vector3(-1,-1,-1), sModelMatrix *Vector3(1,1,1)));
                    glView->updateGL();
                    return;
            }
        }
        QWidget::keyPressEvent(e);
    }

private:
    GLWidget  *glView;
    QTextEdit *textView;
};

class GLApplication : public QApplication
{
public:
    GLApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            //qDebug() << "Open file" << openEvent->file();
        }

        return QApplication::event(event);
    }
};

int main(int argc, char *argv[])
{
    GLApplication app(argc, argv);
    Window window;
    window.resize(window.sizeHint());
    int desktopArea = QApplication::desktop()->width() *
                     QApplication::desktop()->height();
    int widgetArea = window.width() * window.height();
    if (((float)widgetArea / (float)desktopArea) < 0.75f)
        window.show();
    else
        window.showMaximized();
    return app.exec();
}

