#include <GL/glut.h>
#include "core/scene.h"

int janelaW = 1920;
int janelaH = 1080;
int centerX = janelaW / 2;
int centerY = janelaH / 2;

float camX = 0.0f;
float camY = 1.5f;
float camZ = 25.0f;
float yaw = 0.0f;
float pitch = 0.0f;

bool ignoreWarp = false;
bool firstMouse = true;

bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;

bool fullScreen = false;

void atualizaCentroJanela(int w, int h)
{
    centerX = w / 2;
    centerY = h / 2;
}

void altFullScreen()
{
    firstMouse = true;

    if (!fullScreen)
    {
        glutFullScreen(); // entra no fullscreen
        fullScreen = true;
    }
    else
    {
        glutReshapeWindow(janelaW, janelaH);
        fullScreen = false;
    }
}

void setCamera(float x, float y, float z, float newYaw, float newPitch)
{
    camX = x;
    camY = y;
    camZ = z;
    yaw = newYaw;
    pitch = newPitch;
}
