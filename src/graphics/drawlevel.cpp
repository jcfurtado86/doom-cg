#include <GL/glew.h>
#include <GL/glut.h>
#include <cmath>
#include "graphics/drawlevel.h"
#include "core/camera.h"
#include "level/levelmetrics.h"
#include <cstdio>

extern GLuint texParede;
extern GLuint texParedeInterna;
extern GLuint texLava;
extern GLuint texSangue;
extern GLuint texChao;
extern GLuint texChaoInterno;
extern GLuint texTeto;

extern GLuint progLava;
extern GLuint progSangue;

// Controle de tempo
extern float tempo;

// Config do grid
static const float TILE = 4.0f;      // tamanho do tile no mundo (ajuste)
static const float CEILING_H = 4.0f; // altura do teto
static const float WALL_H = 4.0f;    // altura da parede
static const float EPS_Y = 0.001f;   // evita z-fighting

static void bindTexture0(GLuint tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
}

static void desenhaQuadTeto(float x, float z, float tile, float tilesUV)
{
    float half = tile * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f); // <<< NORMAL DO TETO

    // note a ordem invertida (normal para baixo)
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x - half, CEILING_H, z - half);
    glTexCoord2f(tilesUV, 0.0f);
    glVertex3f(x + half, CEILING_H, z - half);
    glTexCoord2f(tilesUV, tilesUV);
    glVertex3f(x + half, CEILING_H, z + half);
    glTexCoord2f(0.0f, tilesUV);
    glVertex3f(x - half, CEILING_H, z + half);
    glEnd();
}

static void desenhaQuadChao(float x, float z, float tile, float tilesUV)
{
    float half = tile * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f); // <<< NORMAL DO CHÃO

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x - half, EPS_Y, z + half);
    glTexCoord2f(tilesUV, 0.0f);
    glVertex3f(x + half, EPS_Y, z + half);
    glTexCoord2f(tilesUV, tilesUV);
    glVertex3f(x + half, EPS_Y, z - half);
    glTexCoord2f(0.0f, tilesUV);
    glVertex3f(x - half, EPS_Y, z - half);
    glEnd();
}

static void desenhaTileChao(float x, float z, GLuint texChaoX, bool temTeto)
{
    glUseProgram(0); // sem shader
    glColor3f(1, 1, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texChaoX);

    // chão
    desenhaQuadChao(x, z, TILE, 2.0f);

    // teto
    if (temTeto)
    {
        glBindTexture(GL_TEXTURE_2D, texTeto);
        desenhaQuadTeto(x, z, TILE, 2.0f);
    }
}

static void desenhaParede(float x, float z, GLuint texParedeX)
{
    float half = TILE * 0.5f;

    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, texParedeX);

    float tilesX = 1.0f;
    float tilesY = 2.0f;

    glBegin(GL_QUADS);

    // Frente (z+)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x - half, 0.0f,   z + half);
    glTexCoord2f(tilesX, 0.0f); glVertex3f(x + half, 0.0f,   z + half);
    glTexCoord2f(tilesX, tilesY); glVertex3f(x + half, WALL_H, z + half);
    glTexCoord2f(0.0f, tilesY); glVertex3f(x - half, WALL_H, z + half);

    // Trás (z-)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x + half, 0.0f,   z - half);
    glTexCoord2f(tilesX, 0.0f); glVertex3f(x - half, 0.0f,   z - half);
    glTexCoord2f(tilesX, tilesY); glVertex3f(x - half, WALL_H, z - half);
    glTexCoord2f(0.0f, tilesY); glVertex3f(x + half, WALL_H, z - half);

    // Direita (x+)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x + half, 0.0f,   z + half);
    glTexCoord2f(tilesX, 0.0f); glVertex3f(x + half, 0.0f,   z - half);
    glTexCoord2f(tilesX, tilesY); glVertex3f(x + half, WALL_H, z - half);
    glTexCoord2f(0.0f, tilesY); glVertex3f(x + half, WALL_H, z + half);

    // Esquerda (x-)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x - half, 0.0f,   z - half);
    glTexCoord2f(tilesX, 0.0f); glVertex3f(x - half, 0.0f,   z + half);
    glTexCoord2f(tilesX, tilesY); glVertex3f(x - half, WALL_H, z + half);
    glTexCoord2f(0.0f, tilesY); glVertex3f(x - half, WALL_H, z - half);

    // Topo
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - half, WALL_H, z + half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + half, WALL_H, z + half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + half, WALL_H, z - half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - half, WALL_H, z - half);

    glEnd();
}


static void desenhaTileLava(float x, float z)
{
    glUseProgram(progLava);

    GLint locTime = glGetUniformLocation(progLava, "uTime");
    GLint locStr = glGetUniformLocation(progLava, "uStrength");
    GLint locScr = glGetUniformLocation(progLava, "uScroll");
    GLint locHeat = glGetUniformLocation(progLava, "uHeat");
    GLint locTex = glGetUniformLocation(progLava, "uTexture");

    glUniform1f(locTime, tempo);
    glUniform1f(locStr, 1.0f);
    glUniform2f(locScr, 0.1f, 0.0f);
    glUniform1f(locHeat, 0.6f);

    bindTexture0(texLava);
    glUniform1i(locTex, 0);

    glColor3f(1, 1, 1);
    desenhaQuadChao(x, z, TILE, 2.0f);

    glUseProgram(0);
}

static void desenhaTileSangue(float x, float z)
{
    glUseProgram(progSangue);

    GLint locTime = glGetUniformLocation(progSangue, "uTime");
    GLint locStr = glGetUniformLocation(progSangue, "uStrength");
    GLint locSpd = glGetUniformLocation(progSangue, "uSpeed");
    GLint locTex = glGetUniformLocation(progSangue, "uTexture");

    glUniform1f(locTime, tempo);
    glUniform1f(locStr, 1.0f);
    glUniform2f(locSpd, 2.0f, 1.3f);

    bindTexture0(texSangue);
    glUniform1i(locTex, 0);

    glColor3f(1, 1, 1);
    desenhaQuadChao(x, z, TILE, 2.0f);

    glUseProgram(0);
}

void drawLevel(const MapLoader &map)
{
    const auto &data = map.data();
    int H = map.getHeight();

    // centraliza o mapa no mundo
    LevelMetrics m = LevelMetrics::fromMap(map, TILE);

    for (int z = 0; z < H; z++)
    {
        for (int x = 0; x < (int)data[z].size(); x++)
        {
            float wx, wz;
            m.tileCenter(x, z, wx, wz); // centro do tile

            char c = data[z][x];

            // TIRAR A RESPOSNABILIDADE DO TILE DAQUI
            if (c == '0') // chão A
                desenhaTileChao(wx, wz, texChao, false);
            else if (c == '3') // chão B
                desenhaTileChao(wx, wz, texChaoInterno, true);
            else if (c == '1') // parede A
                desenhaParede(wx, wz, texParede);
            else if (c == '2') // parede B
                desenhaParede(wx, wz, texParedeInterna);
            else if (c == 'L')
                desenhaTileLava(wx, wz);
            else if (c == 'B')
                desenhaTileSangue(wx, wz);
        }
    }
}
