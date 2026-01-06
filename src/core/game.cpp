#include "core/game.h"
#include "utils/assets.h"
#include "level/level.h"
#include "core/config.h"

#include "core/camera.h"
#include "input/input.h"
#include "graphics/drawlevel.h"
#include "core/movement.h"

#include <GL/glut.h>

#include <cmath>

GLuint texChao;
GLuint texParede;
GLuint texSangue;
GLuint texLava;
GLuint texChaoInterno;
GLuint texParedeInterna;
GLuint texTeto;

GLuint progSangue;
GLuint progLava;

float tempo = 0.0f;

static GameAssets gAssets;
Level gLevel;

static void setupSunLightOnce()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    
    // Ambiente global
    GLfloat sceneAmbient[] = {0.45f, 0.45f, 0.50f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sceneAmbient);

    // Cor do sol
    GLfloat sunDiffuse[] = {1.2f, 1.1f, 1.0f, 1.0f};
    GLfloat sunSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f}; // sem brilho especular por enquanto
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

static void setSunDirectionEachFrame()
{
    // direção DE ONDE a luz vem (mundo). w=0 => direcional
    GLfloat sunDir[] = { 0.3f, 1.0f, 0.2f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
}

bool gameInit(const char *mapPath)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    setupSunLightOnce();

    if (!loadAssets(gAssets))
        return false;

    texChao = gAssets.texChao;
    texParede = gAssets.texParede;
    texSangue = gAssets.texSangue;
    texLava = gAssets.texLava;
    texChaoInterno = gAssets.texChaoInterno;
    texParedeInterna = gAssets.texParedeInterna;
    texTeto = gAssets.texTeto;

    progSangue = gAssets.progSangue;
    progLava = gAssets.progLava;

    if (!loadLevel(gLevel, mapPath, GameConfig::TILE_SIZE))
        return false;

    applySpawn(gLevel, camX, camZ);
    camY = GameConfig::PLAYER_EYE_Y;

    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);

    return true;
}

void gameUpdate(float dt)
{
    // tempo global pros shaders (lava/sangue)
    tempo += dt;

    atualizaMovimento();
}

void gameRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;

    float dirX = cosf(radPitch) * sinf(radYaw);
    float dirY = sinf(radPitch);
    float dirZ = -cosf(radPitch) * cosf(radYaw);

    gluLookAt(
        camX, camY, camZ,
        camX + dirX, camY + dirY, camZ + dirZ,
        0.0f, 1.0f, 0.0f);

    setSunDirectionEachFrame();
 
    drawLevel(gLevel.map);

    glutSwapBuffers();
}
