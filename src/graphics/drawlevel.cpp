#include <GL/glew.h>
#include <GL/glut.h>
#include <cmath>
#include "graphics/drawlevel.h"
#include "core/camera.h"
#include "core/entities.h"
#include "level/levelmetrics.h"
#include "utils/utils.h"
#include <cstdio>

extern GLuint texParede;
extern GLuint texParedeInterna;
extern GLuint texLava;
extern GLuint texSangue;
extern GLuint texChao;
extern GLuint texChaoInterno;
extern GLuint texTeto;

// Transformando em Arrays para suportar múltiplos inimigos ---
extern GLuint texEnemies[5];
extern GLuint texEnemiesRage[5];
extern GLuint texEnemiesDamage[5];
// -----------------------------------------------------------------------------

extern GLuint texHealth;
extern GLuint texAmmo;

extern GLuint progLava;
extern GLuint progSangue;

// Controle de tempo
extern float tempo;

// =====================
// CONFIG / CONSTANTES
// =====================

// Config do grid
static const float TILE = 4.0f;      // tamanho do tile no mundo (ajuste)
static const float CEILING_H = 4.0f; // altura do teto
static const float WALL_H = 4.0f;    // altura da parede
static const float EPS_Y = 0.001f;   // evita z-fighting

static const GLfloat kAmbientOutdoor[] = {0.45f, 0.30f, 0.25f, 1.0f}; // quente (seu atual)
static const GLfloat kAmbientIndoor[] = {0.12f, 0.12f, 0.18f, 1.0f};  // frio/azulado

// ======================
// CONFIG ÚNICA DO CULLING (XZ)
// ======================
static float gCullHFovDeg = 170.0f;     // FOV horizontal do culling (cenário + entidades)
static float gCullNearTiles = 2.0f;     // dentro disso não faz culling angular
static float gCullMaxDistTiles = 20.0f; // 0 = sem limite; em tiles

// Retorna TRUE se deve renderizar o objeto no plano XZ (distância + cone de FOV)
// - Usa as configs globais gCull*
// - Usa forward já normalizado (fwdx,fwdz) e flag hasFwd
static inline bool isVisibleXZ(float objX, float objZ,
                               float camX, float camZ,
                               bool hasFwd, float fwdx, float fwdz)
{
    float vx = objX - camX;
    float vz = objZ - camZ;
    float distSq = vx * vx + vz * vz;

    // 0) Distância máxima (se habilitada)
    if (gCullMaxDistTiles > 0.0f)
    {
        float maxDist = gCullMaxDistTiles * TILE;
        float maxDistSq = maxDist * maxDist;
        if (distSq > maxDistSq)
            return false;
    }

    // 1) Dentro do near: não faz culling angular
    float nearDist = gCullNearTiles * TILE;
    float nearDistSq = nearDist * nearDist;
    if (distSq <= nearDistSq)
        return true;

    // 2) Sem forward válido: não faz culling angular
    if (!hasFwd)
        return true;

    // 3) Cone por FOV horizontal
    float cosHalf = std::cos(deg2rad(gCullHFovDeg * 0.5f));

    float invDist = 1.0f / std::sqrt(distSq);
    float nvx = vx * invDist;
    float nvz = vz * invDist;

    float dot = clampf(nvx * fwdx + nvz * fwdz, -1.0f, 1.0f);
    return dot >= cosHalf;
}

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
    const float rate = 4.0f;
    float block = floorf(t * rate);
    float r = hash01(block);

    if (r < 0.22f)
    {
        float phase = t * rate - block;

        if (phase > 0.35f && phase < 0.55f)
            return 0.12f;

        if (r < 0.06f && phase > 0.65f && phase < 0.78f)
            return 0.40f;
    }

    return 0.96f + 0.04f * sinf(t * 5.0f);
}

static void setIndoorLampAt(float x, float z, float intensity)
{
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
    glDisable(GL_LIGHT0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientIndoor);

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
    glNormal3f(0.0f, -1.0f, 0.0f);

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
    glNormal3f(0.0f, 1.0f, 0.0f);

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
    glUseProgram(0);
    glColor3f(1, 1, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texChaoX);

    desenhaQuadChao(x, z, TILE, 2.0f);

    if (temTeto)
    {
        glBindTexture(GL_TEXTURE_2D, texTeto);
        desenhaQuadTeto(x, z, TILE, 2.0f);
    }
}

// --- Desenha parede FACE POR FACE ---
static void desenhaParedePorFace(float x, float z, GLuint texParedeX, int f)
{
    float half = TILE * 0.5f;

    glUseProgram(0);
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, texParedeX);

    float tilesX = 1.0f;
    float tilesY = 2.0f;

    glBegin(GL_QUADS);

    switch (f)
    {
    case 0: // z+ (Frente)
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

    case 1: // z- (Trás)
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

    case 2: // x+ (Direita)
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

    case 3: // x- (Esquerda)
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
    }
    glEnd();
}

// Wrapper para desenhar o cubo todo (parede outdoor)
static void desenhaParedeCuboCompleto(float x, float z, GLuint texParedeX)
{
    desenhaParedePorFace(x, z, texParedeX, 0);
    desenhaParedePorFace(x, z, texParedeX, 1);
    desenhaParedePorFace(x, z, texParedeX, 2);
    desenhaParedePorFace(x, z, texParedeX, 3);

    float half = TILE * 0.5f;
    glBindTexture(GL_TEXTURE_2D, texParedeX);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x - half, WALL_H, z + half);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(x + half, WALL_H, z + half);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(x + half, WALL_H, z - half);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x - half, WALL_H, z - half);
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

// --- Checa vizinhos ---
static char getTileAt(const MapLoader &map, int tx, int tz)
{
    const auto &data = map.data();
    const int H = map.getHeight();

    if (tz < 0 || tz >= H)
        return '0';
    if (tx < 0 || tx >= (int)data[tz].size())
        return '0';

    return data[tz][tx];
}

static void drawFace(float wx, float wz, int face, char neighbor, GLuint texParedeInternaX)
{
    bool outside = (neighbor == '0' || neighbor == 'L' || neighbor == 'B');

    if (outside)
    {
        glDisable(GL_LIGHT1);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbientOutdoor);
        glEnable(GL_LIGHT0);

        desenhaParedePorFace(wx, wz, texParedeInternaX, face);
    }
    else if (neighbor != '2')
    {
        beginIndoor(wx, wz);
        desenhaParedePorFace(wx, wz, texParedeInternaX, face);
        endIndoor();
    }
}

void drawLevel(const MapLoader &map, float px, float pz, float dx, float dz)
{
    const auto &data = map.data();
    const int H = map.getHeight();

    LevelMetrics m = LevelMetrics::fromMap(map, TILE);

    float fwdx, fwdz;
    bool hasFwd = getForwardXZ(dx, dz, fwdx, fwdz);

    for (int z = 0; z < H; z++)
    {
        for (int x = 0; x < (int)data[z].size(); x++)
        {
            float wx, wz;
            m.tileCenter(x, z, wx, wz);

            // CULLING ÚNICO (cenário)
            if (!isVisibleXZ(wx, wz, px, pz, hasFwd, fwdx, fwdz))
                continue;

            char c = data[z][x];

            bool isEntity = (c == 'J' || c == 'T' || c == 'M' || c == 'K' ||
                             c == 'G' || c == 'H' || c == 'A' || c == 'E' ||
                             c == 'F' || c == 'I');

            if (isEntity)
            {
                char viz1 = getTileAt(map, x + 1, z);
                char viz2 = getTileAt(map, x - 1, z);
                char viz3 = getTileAt(map, x, z + 1);
                char viz4 = getTileAt(map, x, z - 1);

                bool isIndoor = (viz1 == '3' || viz1 == '2' ||
                                 viz2 == '3' || viz2 == '2' ||
                                 viz3 == '3' || viz3 == '2' ||
                                 viz4 == '3' || viz4 == '2');

                if (isIndoor)
                {
                    beginIndoor(wx, wz);
                    desenhaTileChao(wx, wz, texChaoInterno, true);
                    endIndoor();
                }
                else
                {
                    desenhaTileChao(wx, wz, texChao, false);
                }
            }
            else if (c == '0')
            {
                desenhaTileChao(wx, wz, texChao, false);
            }
            else if (c == '3')
            {
                beginIndoor(wx, wz);
                desenhaTileChao(wx, wz, texChaoInterno, true);
                endIndoor();
            }
            else if (c == '1')
            {
                desenhaParedeCuboCompleto(wx, wz, texParede);
            }
            else if (c == '2')
            {
                char vizFrente = getTileAt(map, x, z + 1);
                char vizTras = getTileAt(map, x, z - 1);
                char vizDireita = getTileAt(map, x + 1, z);
                char vizEsq = getTileAt(map, x - 1, z);

                drawFace(wx, wz, 0, vizFrente, texParedeInterna);
                drawFace(wx, wz, 1, vizTras, texParedeInterna);
                drawFace(wx, wz, 2, vizDireita, texParedeInterna);
                drawFace(wx, wz, 3, vizEsq, texParedeInterna);
            }
            else if (c == 'L')
            {
                desenhaTileLava(wx, wz);
            }
            else if (c == 'B')
            {
                desenhaTileSangue(wx, wz);
            }
        }
    }
}

static void drawSprite(float x, float z, float w, float h, GLuint tex, float camX, float camZ)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    glBindTexture(GL_TEXTURE_2D, tex);
    glColor3f(1, 1, 1);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    float ddx = camX - x;
    float ddz = camZ - z;
    float angle = std::atan2(ddx, ddz) * 180.0f / 3.14159f;

    glRotatef(angle, 0.0f, 1.0f, 0.0f);

    float hw = w * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);

    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-hw, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(hw, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(hw, h, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-hw, h, 0.0f);
    glEnd();

    glPopMatrix();

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}

// Desenha inimigos e itens
void drawEntities(const std::vector<Enemy> &enemies, const std::vector<Item> &items,
                  float camX, float camZ, float dx, float dz)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    float fwdx, fwdz;
    bool hasFwd = getForwardXZ(dx, dz, fwdx, fwdz);

    // --- ITENS ---
    for (const auto &item : items)
    {
        if (!item.active)
            continue;

        if (!isVisibleXZ(item.x, item.z, camX, camZ, hasFwd, fwdx, fwdz))
            continue;

        if (item.type == ITEM_HEALTH)
            drawSprite(item.x, item.z, 0.7f, 0.7f, texHealth, camX, camZ);
        else if (item.type == ITEM_AMMO)
            drawSprite(item.x, item.z, 0.7f, 0.7f, texAmmo, camX, camZ);
    }

    // --- INIMIGOS ---
    for (const auto &en : enemies)
    {
        if (en.state == STATE_DEAD)
            continue;

        if (!isVisibleXZ(en.x, en.z, camX, camZ, hasFwd, fwdx, fwdz))
            continue;

        int t = (en.type < 0 || en.type > 4) ? 0 : en.type;

        GLuint currentTex;
        if (en.hurtTimer > 0.0f)
            currentTex = texEnemiesDamage[t];
        else if (en.state == STATE_CHASE || en.state == STATE_ATTACK)
            currentTex = texEnemiesRage[t];
        else
            currentTex = texEnemies[t];

        drawSprite(en.x, en.z, 2.5f, 2.5f, currentTex, camX, camZ);
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
}
