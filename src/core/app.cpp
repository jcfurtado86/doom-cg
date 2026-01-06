#include "core/app.h"
#include "core/window.h"
#include "core/fps.h"
#include "core/game.h"
#include "core/config.h"

#include <GL/glut.h>
#include <GL/glu.h>

static const char *kTitle = "Um dia vai ser DOOM";

static void displayCb()
{
    gameRender();
    fpsTickAndUpdateTitle(kTitle);
}

static void reshapeCb(int w, int h)
{
    if (h == 0)
        h = 1;
    float a = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, a, 1.0f, 100.0f);

    glMatrixMode(GL_MODELVIEW);

    atualizaCentroJanela(w, h);
}

static void timerCb(int)
{
    const float dt = GameConfig::TIMER_MS * 0.001f;

    gameUpdate(dt);

    glutPostRedisplay();
    glutTimerFunc(GameConfig::TIMER_MS, timerCb, 0);
}

void appInit()
{
    glutDisplayFunc(displayCb);
    glutReshapeFunc(reshapeCb);
    glutTimerFunc(0, timerCb, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}
