/*
  g++ -std=c++11 xgl.cpp -o xgl -lX11 -lGL -lGLU -lpthread
  g++ -std=c++11 xgl.cpp -o xgl -lX11 -lGL -lGLU -lpthread -O0 -g

  https://retokoradi.com/2014/03/30/opengl-transition-to-core-profile
*/

#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

#include <thread>
#include <vector>
#include <cmath>

#include <unistd.h>
#include <time.h>
#include <string.h>

Display  *sDisplay;
bool     sQuit = false;
bool     sMakeCurrent = true;
bool     sOwnerFirst = false;
bool     sWaitShown = false;
bool     sFlushX = true;

Window                  sRoot;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };


class GLContext
{
    GLXContext mCtx;
    Window     mWin;
    int        mID;
    float      mTime;
    bool       mExposed;

    GLContext(const GLContext&) = delete;

public:
    GLContext(int id) : mCtx(0), mWin(0), mID(id), mTime(0), mExposed(false) {
        if (!sDisplay) {
            sDisplay = XOpenDisplay(nullptr);
            sRoot = DefaultRootWindow(sDisplay);
        }

        if (!sDisplay || !sRoot) {
            printf("\n\tcannot connect to X server\n\n");
            exit(0);
        }

        XVisualInfo* vi = glXChooseVisual(sDisplay, 0, att);
        if (vi == nullptr) {
            printf("\n\tno appropriate visual found\n\n");
            exit(0);
        } 
        else {
            printf("\n\tvisual %p selected\n", (void *)vi->visualid); /* %p creates hexadecimal output like in glxinfo */
        }

        Colormap cmap;
        cmap = XCreateColormap(sDisplay, sRoot, vi->visual, AllocNone);

        XSetWindowAttributes swa;
        swa.colormap = cmap;
        swa.event_mask = ExposureMask | KeyPressMask;

        mWin = XCreateWindow(sDisplay, sRoot, 0, 0, 600, 600, 0,
                             vi->depth, InputOutput, vi->visual,
                             CWColormap | CWEventMask, &swa);

        XMapWindow(sDisplay, mWin);
        XStoreName(sDisplay, mWin, ("Window " + std::to_string(id)).c_str());

        mCtx = glXCreateContext(sDisplay, vi, NULL, GL_TRUE);
    }
    ~GLContext() {
        if (mID != -1) {
            printf("~GLContext %p %p\n", mCtx, mWin);
            glXDestroyContext(sDisplay, mCtx);
            XDestroyWindow(sDisplay, mWin);
        }
    }
    GLContext(GLContext&& o) : mCtx(o.mCtx), mWin(o.mWin), mID(o.mID) {
        o.mID = -1; o.mCtx = 0; o.mWin = 0;
    }

    void DrawAQuad(float t, float c = 1) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1., 1., -1., 1., 1., 20.);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

        glRotatef(t * 10.0, 0, 0, 1);

        float v = 0.75; //sin(t * M_PI);
        glBegin(GL_QUADS);
        glColor3f(c, 0, 0); glVertex3f(-v, -v, 0.);
        glColor3f(0, c, 0); glVertex3f( v, -v, 0.);
        glColor3f(0, 0, c); glVertex3f( v,  v, 0.);
        glColor3f(c, c, 0); glVertex3f(-v,  v, 0.);
        glEnd();
    } 

    void makeCurrent(int& w, int& h) {
        XWindowAttributes gwa;
        XGetWindowAttributes(sDisplay, mWin, &gwa);
        w = gwa.width;
        h = gwa.height;


        if (sMakeCurrent) {
            XLockDisplay(sDisplay);
            glXMakeCurrent(sDisplay, mWin, mCtx);
            XUnlockDisplay(sDisplay);
        }
    }

    void draw(float t, bool clear = true) {
        glEnable(GL_DEPTH_TEST);
        if (clear) {
            glClearColor(1.0, 1.0, 1.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        DrawAQuad(t);

        XLockDisplay(sDisplay);
        glXSwapBuffers(sDisplay, mWin);
        XUnlockDisplay(sDisplay);
    }

    void run() {
        mExposed = true;
        printf("  window %d, shown\n", mID);


        int w, h;
        makeCurrent(w, h);

//        XSelectInput(sDisplay, mWin, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
//        XEvent event;

        time_t start = time(0);
        while (!sQuit) {
/*
            XWindowEvent(sDisplay, mWin, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask, &event);
            if (event.type == DestroyNotify) {
                sQuit = true;
                break;
            }
*/
            glViewport(0, 0, w, h);
            draw(mTime);
            mTime = difftime(time(0), start);
        }
    }
    bool shown() const { return mExposed; }
};

void contextTaker(std::vector<GLContext>& contexts) {
    float t;
    time_t start = time(0);
    while (true) {
        for (auto& ctx : contexts) {
            if (sWaitShown && !ctx.shown())
                continue;

            int w, h;
            ctx.makeCurrent(w, h);
            w /= 2;
            h /= 2;

            glViewport(0, 0, w, h);
            ctx.draw(t);
        }
        t = difftime(start, time(0)); 

        XLockDisplay(sDisplay);
        XFlush(sDisplay);
        XUnlockDisplay(sDisplay);    
    }
}

void flush() {
    if (!sFlushX)
        return;

    XLockDisplay(sDisplay);
    XFlush(sDisplay);
    XUnlockDisplay(sDisplay);
}

int help(const char* prog, int code = -1) {
    prog = prog ? prog : "xgl";
    size_t len = strlen(prog);
    printf("%-*s [-t numthreads]\n"
           "%*s [-current 0|1]\n"
           "%*s [-owner 0|1]\n",
           len, prog, len, "", len, "");
    return code;
}

int main(int argc, char *argv[]) {

    int N = 2;

    for (int i = 1; i < argc; i+=2) {
        if (i+1 >= argc)
            return help(argv[0]);

        const char* arg = argv[i+0];
        const char* val = argv[i+1];
        if (! strcmp("-t", arg)) {
            N = atoi(val);
        }
        else if (! strcmp("-current", arg)) {
            sMakeCurrent = atoi(val);
        }
        else if (! strcmp("-owner", arg)) {
            sOwnerFirst = atoi(val);
        }
        else if (! strcmp("-wait", arg)) {
            sWaitShown = atoi(val);
        } else {
            return help(argv[0]);
        }
    }

    XInitThreads();

    std::vector<GLContext>   contexts;
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i)
        contexts.emplace_back(i);

    printf("%lu windows created of %lu\n", contexts.size(), N);

    XEvent xev;
    for (int i = 0; !sQuit && i < N;) {
        if (i < N) {
            //XLockDisplay(sDisplay);
            XNextEvent(sDisplay, &xev);
            //XUnlockDisplay(sDisplay);
        }

        if (xev.type == KeyPress) {
            sQuit = true;
            break;
        }
        if (xev.type == Expose) {
            //contexts[i].makeCurrent();
            //glXMakeCurrent(sDisplay, None, nullptr);
            if (sOwnerFirst && i == 0)
                threads.emplace_back(&contextTaker, std::ref(contexts));

            threads.emplace_back(&GLContext::run, std::ref(contexts[i]));
            ++i;
        }

        flush();
    }

    if (!sOwnerFirst)
        threads.emplace_back(&contextTaker, std::ref(contexts));

    for (auto&& glt : threads) {
        glt.join();
    }

    contexts.clear();
    XCloseDisplay(sDisplay);
    return 0;
}
