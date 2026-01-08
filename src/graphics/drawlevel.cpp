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

static const GLfloat kAmbientOutdoor[] = {0.45f, 0.30f, 0.25f, 1.0f}; // quente (seu atual)
static const GLfloat kAmbientIndoor[] = {0.12f, 0.12f, 0.18f, 1.0f};  // frio/azulado

static void bindTexture0(GLuint tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
}

static float hash01(float x)
{
    float s = sinf(x * 12.9898f) * 43758.5453f;
    return s - floorf(s);
}

static float flickerFluorescente(float t)
{
    const float rate = 4.0f; // era 9.0
    float block = floorf(t * rate);
    float r = hash01(block);

    if (r < 0.22f) // era 0.12
    {
        float phase = t * rate - block;

        // apagão mais longo
        if (phase > 0.35f && phase < 0.55f)
            return 0.12f; // quase apaga

        // as vezes um segundo tranco
        if (r < 0.06f && phase > 0.65f && phase < 0.78f)
            return 0.40f;
    }

    return 0.96f + 0.04f * sinf(t * 5.0f);
}

static void setIndoorLampAt(float x, float z, float intensity)
{
    // posição da lâmpada (pontual)
    GLfloat pos[] = {x, CEILING_H - 0.05f, z, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, pos);

    GLfloat diff[] = {
        1.20f * intensity,
        1.22f * intensity,
        1.28f * intensity,
        1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diff);

    GLfloat amb[] = {
        1.10f * intensity,
        1.10f * intensity,
        1.12f * intensity,
        1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, amb);
}

static void beginIndoor(float wx, float wz)
{
    // sol NÃO entra
    glDisable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientIndoor); // esfria o ambiente

    // lâmpada fria entra
    glEnable(GL_LIGHT1);

    float f = flickerFluorescente(tempo);
    float intensity = 1.2f * f;

    setIndoorLampAt(wx, wz, intensity);
}

static void endIndoor()
{
    glDisable(GL_LIGHT1);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientOutdoor);

    glEnable(GL_LIGHT0);
}

static void desenhaQuadTeto(float x, float z, float tile, float tilesUV)
{
    float half = tile * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f); // NORMAL DO TETO

    // note a ordem invertida
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
    glNormal3f(0.0f, 1.0f, 0.0f); // NORMAL DO CHÃO

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

static bool isTileOutdoor(char c)
{
    return (c == '0' || c == 'L' || c == 'B' || c == '1');
}

static char getTileAt(const MapLoader &map, int gridX, int gridZ)
{
    const auto &data = map.data();
    int H = map.getHeight();

    if (gridZ < 0 || gridZ >= H)
        return '1';
    if (gridX < 0 || gridX >= (int)data[gridZ].size())
        return '1';

    return data[gridZ][gridX];
}

static void desenhaFaceParede(float x, float z, int face, float tilesX, float tilesY)
{
    float half = TILE * 0.5f;

    glBegin(GL_QUADS);

    switch(face)
    {
        case 0:
            glNormal3f(0.0f, 0.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x - half, 0.0f, z + half);
            glTexCoord2f(tilesX, 0.0f);
            glVertex3f(x + half, 0.0f, z + half);
            glTexCoord2f(tilesX, tilesY);
            glVertex3f(x + half, WALL_H, z + half);
            glTexCoord2f(0.0f, tilesY);
            glVertex3f(x - half, WALL_H, z + half);
            break;

        case 1:
            glNormal3f(0.0f, 0.0f, -1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x + half, 0.0f, z - half);
            glTexCoord2f(tilesX, 0.0f);
            glVertex3f(x - half, 0.0f, z - half);
            glTexCoord2f(tilesX, tilesY);
            glVertex3f(x - half, WALL_H, z - half);
            glTexCoord2f(0.0f, tilesY);
            glVertex3f(x + half, WALL_H, z - half);
            break;

        case 2:
            glNormal3f(1.0f, 0.0f, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x + half, 0.0f, z + half);
            glTexCoord2f(tilesX, 0.0f);
            glVertex3f(x + half, 0.0f, z - half);
            glTexCoord2f(tilesX, tilesY);
            glVertex3f(x + half, WALL_H, z - half);
            glTexCoord2f(0.0f, tilesY);
            glVertex3f(x + half, WALL_H, z + half);
            break;

        case 3:
            glNormal3f(-1.0f, 0.0f, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x - half, 0.0f, z - half);
            glTexCoord2f(tilesX, 0.0f);
            glVertex3f(x - half, 0.0f, z + half);
            glTexCoord2f(tilesX, tilesY);
            glVertex3f(x - half, WALL_H, z + half);
            glTexCoord2f(0.0f, tilesY);
            glVertex3f(x - half, WALL_H, z - half);
            break;

        case 4:
            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x - half, WALL_H, z + half);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(x + half, WALL_H, z + half);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(x + half, WALL_H, z - half);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(x - half, WALL_H, z - half);
            break;
    }

    glEnd();
}

static void desenhaParede(float x, float z, GLuint texParedeX)
{
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, texParedeX);

    float tilesX = 1.0f;
    float tilesY = 2.0f;

    for (int face = 0; face < 5; face++)
    {
        desenhaFaceParede(x, z, face, tilesX, tilesY);
    }
}

static void desenhaParedeComIluminacaoMista(const MapLoader &map, int gridX, int gridZ, float wx, float wz, GLuint texParedeX)
{
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, texParedeX);

    float tilesX = 1.0f;
    float tilesY = 2.0f;

    int adjacentX[] = {0, 0, 1, -1, 0};
    int adjacentZ[] = {1, -1, 0, 0, 0};

    for (int face = 0; face < 5; face++)
    {
        int adjX = gridX + adjacentX[face];
        int adjZ = gridZ + adjacentZ[face];
        char tileAdj = getTileAt(map, adjX, adjZ);

        bool useOutdoor = (face == 4) || isTileOutdoor(tileAdj);

        if (useOutdoor)
        {
            glEnable(GL_LIGHT0);
            glDisable(GL_LIGHT1);
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientOutdoor);
        }
        else
        {
            glDisable(GL_LIGHT0);
            glEnable(GL_LIGHT1);
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientIndoor);

            float f = flickerFluorescente(tempo);
            float intensity = 1.2f * f;
            setIndoorLampAt(wx, wz, intensity);
        }

        desenhaFaceParede(wx, wz, face, tilesX, tilesY);
    }

    glDisable(GL_LIGHT1);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientOutdoor);
    glEnable(GL_LIGHT0);
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
            if (c == '0') // chão A (outdoor)
                desenhaTileChao(wx, wz, texChao, false);
            else if (c == '3') // chão B (indoor, tem teto)
            {
                beginIndoor(wx, wz);
                desenhaTileChao(wx, wz, texChaoInterno, true);
                endIndoor();
            }
            else if (c == '1')
                desenhaParede(wx, wz, texParede);
            else if (c == '2')
            {
                desenhaParedeComIluminacaoMista(map, x, z, wx, wz, texParedeInterna);
            }
            else if (c == 'L')
            {
                // lava é shader: não pega GL_LIGHT0/1.
                // Se quiser “indoor” com lava escura/clara, a gente ajusta no shader depois.
                desenhaTileLava(wx, wz);
            }
            else if (c == 'B')
            {
                desenhaTileSangue(wx, wz);
            }
        }
    }
}
