#include "core/fps.h"
#include <GL/glut.h>
#include <cstdio>

void fpsTickAndUpdateTitle(const char *baseTitle)
{
    static int fps = 0;
    static int frameCount = 0;
    static int previousTime = 0;

    frameCount++;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);

    if (currentTime - previousTime > 1000)
    {
        fps = frameCount;
        frameCount = 0;
        previousTime = currentTime;

        char titulo[128];
        std::snprintf(titulo, sizeof(titulo), "%s (%d FPS)", baseTitle, fps);
        glutSetWindowTitle(titulo);
    }
}
