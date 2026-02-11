#include "audio/audio_system.h"
#include "core/game.h"
#include "utils/assets.h"
#include "level/level.h"
#include "core/config.h"
#include "graphics/skybox.h"
#include "input/keystate.h"
#include "core/camera.h"
#include "input/input.h"
#include "graphics/drawlevel.h"
#include "core/movement.h"
#include "core/entities.h"
#include <GL/glut.h>
#include "core/window.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>

// --- SISTEMA DE FOGO ---
struct ParticulaFogo
{
    float x, y;    // Posição
    float velY;    // Velocidade vertical
    float vida;    // Vida (1.0 = nova, 0.0 = morta)
    float tamanho; // Tamanho do quadrado
    float r, g, b; // Cor
};

std::vector<ParticulaFogo> fogo; // Lista de partículas

// --- VARIÁVEIS GLOBAIS ---
GLuint texChao;
GLuint texParede;
GLuint texSangue;
GLuint texLava;
GLuint texChaoInterno;
GLuint texParedeInterna;
GLuint texTeto;
GLuint texSkydome;

// Texturas de Entidades
GLuint texEnemies[5];
GLuint texEnemiesRage[5];
GLuint texEnemiesDamage[5];
GLuint texHealth;
GLuint texAmmo;
GLuint texGunHUD;
GLuint texHudFundo;

GLuint texGunDefault, texGunFire1, texGunFire2;
GLuint texGunReload1, texGunReload2;

GLuint texDamage;
float damageAlpha = 0.0f; // Começa invisível

GLuint texHealthOverlay;
float healthAlpha = 0.0f;

GLuint progSangue;
GLuint progLava;

float tempo = 0.0f;

int playerHealth = 100; // Vida do Jogador

// SISTEMA DE MUNIÇÃO
const int MAX_MAGAZINE = 12; // Capacidade do pente
int currentAmmo = 12;        // Balas na arma
int reserveAmmo = 25;        // Balas no bolso

GameState currentState = MENU_INICIAL; // Começa no menu

// Estados da animação
enum WeaponState
{
    W_IDLE,
    W_FIRE_1,
    W_FIRE_2,
    W_RETURN,

    W_PUMP, // shotgun pump/cycle

    W_RELOAD_1,
    W_RELOAD_2,
    W_RELOAD_3
};

WeaponState weaponState = W_IDLE;
float weaponTimer = 0.0f;

// --- Assets / Level ---
static GameAssets gAssets;
Level gLevel;

// Configurações da IA
const float ENEMY_SPEED = 2.5f;
const float ENEMY_VIEW_DIST = 15.0f;
const float ENEMY_ATTACK_DIST = 1.5f;

static AudioSystem gAudioSys;

// --- FUNÇÕES AUXILIARES DE LUZ ---
static void setupIndoorLightOnce()
{
    glEnable(GL_LIGHT1);
    GLfloat lampDiffuse[] = {1.7f, 1.7f, 1.8f, 1.0f};
    GLfloat lampSpecular[] = {0, 0, 0, 1.0f};
    GLfloat lampAmbient[] = {0.98f, 0.99f, 1.41f, 1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lampDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lampSpecular);
    glLightfv(GL_LIGHT1, GL_AMBIENT, lampAmbient);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.6f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.06f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);
    glDisable(GL_LIGHT1);
}

static void setupSunLightOnce()
{
    glEnable(GL_LIGHT0);
    GLfloat sceneAmbient[] = {0.45f, 0.30f, 0.25f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sceneAmbient);
    GLfloat sunDiffuse[] = {1.4f, 0.55f, 0.30f, 1.0f};
    GLfloat sunSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

static void setSunDirectionEachFrame()
{
    GLfloat sunDir[] = {0.3f, 1.0f, 0.2f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
}

void playerTryReload()
{
    if (weaponState != W_IDLE)
        return;

    if (currentAmmo >= MAX_MAGAZINE)
        return;

    if (reserveAmmo <= 0)
        return;

    std::printf("Recarregando...\n");
    weaponState = W_RELOAD_1;
    weaponTimer = 0.50f;

    audioPlayReload(gAudioSys);
}

// --- ATAQUE ---
void playerTryAttack()
{
    if (weaponState != W_IDLE)
        return;
    if (currentAmmo <= 0)
    {
        return;
    }

    currentAmmo--;

    audioOnPlayerShot(gAudioSys);
    audioPlayShot(gAudioSys);

    weaponState = W_FIRE_1;
    weaponTimer = 0.08f;

    for (auto &en : gLevel.enemies)
    {
        if (en.state == STATE_DEAD)
            continue;

        float dx = en.x - camX;
        float dz = en.z - camZ;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (dist < 17.0f)
        {
            float radYaw = yaw * 3.14159f / 180.0f;
            float camDirX = std::sin(radYaw);
            float camDirZ = -std::cos(radYaw);

            float toEnemyX = dx / dist;
            float toEnemyZ = dz / dist;

            float dot = camDirX * toEnemyX + camDirZ * toEnemyZ;

            if (dot > 0.6f)
            {
                en.hp -= 30;
                en.hurtTimer = 0.5f;

                if (en.hp <= 0)
                {
                    en.state = STATE_DEAD;
                    en.respawnTimer = 15.0f;

                    Item drop;
                    drop.type = ITEM_AMMO;
                    drop.x = en.x;
                    drop.z = en.z;
                    drop.active = true;
                    drop.respawnTimer = 0.0f;

                    gLevel.items.push_back(drop);
                }
                return;
            }
        }
    }
}

// --- Walkable ---
bool isWalkable(float x, float z)
{
    float tile = gLevel.metrics.tile;
    float offX = gLevel.metrics.offsetX;
    float offZ = gLevel.metrics.offsetZ;

    int tx = (int)((x - offX) / tile);
    int tz = (int)((z - offZ) / tile);

    const auto &data = gLevel.map.data();

    if (tz < 0 || tz >= (int)data.size())
        return false;
    if (tx < 0 || tx >= (int)data[tz].size())
        return false;

    char c = data[tz][tx];
    if (c == '1' || c == '2')
        return false;

    return true;
}

// --- ENTIDADES ---
void updateEntities(float dt)
{
    // Inimigos
    for (auto &en : gLevel.enemies)
    {
        if (en.state == STATE_DEAD)
        {
            en.respawnTimer -= dt;
            if (en.respawnTimer <= 0.0f)
            {
                en.state = STATE_IDLE;
                en.hp = 100;
                en.x = en.startX;
                en.z = en.startZ;
                en.hurtTimer = 0.0f;
            }
            continue;
        }

        if (en.hurtTimer > 0.0f)
            en.hurtTimer -= dt;

        float dx = camX - en.x;
        float dz = camZ - en.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        switch (en.state)
        {
        case STATE_IDLE:
            if (dist < ENEMY_VIEW_DIST)
                en.state = STATE_CHASE;
            break;

        case STATE_CHASE:
            if (dist < ENEMY_ATTACK_DIST)
            {
                en.state = STATE_ATTACK;
                en.attackCooldown = 0.5f;
            }
            else if (dist > ENEMY_VIEW_DIST * 1.5f)
            {
                en.state = STATE_IDLE;
            }
            else
            {
                float dirX = dx / dist;
                float dirZ = dz / dist;

                float moveStep = ENEMY_SPEED * dt;

                float nextX = en.x + dirX * moveStep;
                if (isWalkable(nextX, en.z))
                    en.x = nextX;

                float nextZ = en.z + dirZ * moveStep;
                if (isWalkable(en.x, nextZ))
                    en.z = nextZ;
            }
            break;

        case STATE_ATTACK:
            if (dist > ENEMY_ATTACK_DIST)
            {
                en.state = STATE_CHASE;
            }
            else
            {
                en.attackCooldown -= dt;
                if (en.attackCooldown <= 0.0f)
                {
                    playerHealth -= 10;
                    en.attackCooldown = 1.0f;
                    damageAlpha = 1.0f;

                    audioPlayHurt(gAudioSys);
                }
            }
            break;

        default:
            break;
        }
    }

    // Itens
    for (auto &item : gLevel.items)
    {
        if (!item.active)
        {
            item.respawnTimer -= dt;
            if (item.respawnTimer <= 0.0f)
            {
                item.active = true;
            }
            continue;
        }

        float dx = camX - item.x;
        float dz = camZ - item.z;

        if (dx * dx + dz * dz < 1.0f)
        {
            item.active = false;

            if (item.type == ITEM_HEALTH)
            {
                item.respawnTimer = 15.0f;
                playerHealth += 50;
                if (playerHealth > 100)
                    playerHealth = 100;
                healthAlpha = 1.0f;
            }
            else if (item.type == ITEM_AMMO)
            {
                item.respawnTimer = 999999.0f;
                reserveAmmo = 20;
            }
        }
    }
}

// --- INIT ---
bool gameInit(const char *mapPath)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    setupSunLightOnce();
    setupIndoorLightOnce();

    if (!loadAssets(gAssets))
        return false;

    texChao = gAssets.texChao;
    texParede = gAssets.texParede;
    texSangue = gAssets.texSangue;
    texLava = gAssets.texLava;
    texChaoInterno = gAssets.texChaoInterno;
    texParedeInterna = gAssets.texParedeInterna;
    texTeto = gAssets.texTeto;

    texSkydome = gAssets.texSkydome;

    texGunDefault = gAssets.texGunDefault;
    texGunFire1 = gAssets.texGunFire1;
    texGunFire2 = gAssets.texGunFire2;
    texGunReload1 = gAssets.texGunReload1;
    texGunReload2 = gAssets.texGunReload2;
    texGunHUD = gAssets.texGunHUD;
    texHudFundo = gAssets.texHudFundo;

    texDamage = gAssets.texDamage;

    for (int i = 0; i < 5; i++)
    {
        texEnemies[i] = gAssets.texEnemies[i];
        texEnemiesRage[i] = gAssets.texEnemiesRage[i];
        texEnemiesDamage[i] = gAssets.texEnemiesDamage[i];
    }

    texHealthOverlay = gAssets.texHealthOverlay;
    texHealth = gAssets.texHealth;
    texAmmo = gAssets.texAmmo;

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

    // Audio init + ambient + enemy sources
    audioInit(gAudioSys, gLevel);

    return true;
}

// --- WEAPON ANIM ---
void updateWeaponAnim(float dt)
{
    const float TIME_FRAME_1 = 0.05f;
    const float TIME_FRAME_2 = 0.12f;
    const float RELOAD_T2 = 0.85f;
    const float RELOAD_T3 = 0.25f;

    if (weaponState == W_IDLE)
        return;

    weaponTimer -= dt;

    if (weaponTimer > 0.0f)
        return;

    if (weaponState == W_FIRE_1)
    {
        weaponState = W_FIRE_2;
        weaponTimer = TIME_FRAME_2;
    }
    else if (weaponState == W_FIRE_2)
    {
        weaponState = W_PUMP;
        weaponTimer = AudioTuning::PUMP_TIME;
        audioPlayPumpClick(gAudioSys);
    }
    else if (weaponState == W_RETURN)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;
    }

    else if (weaponState == W_PUMP)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;
    }

    else if (weaponState == W_RELOAD_1)
    {
        weaponState = W_RELOAD_2;
        weaponTimer = RELOAD_T2;
    }
    else if (weaponState == W_RELOAD_2)
    {
        weaponState = W_RELOAD_3;
        weaponTimer = RELOAD_T3;
    }
    else if (weaponState == W_RELOAD_3)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;

        int needed = MAX_MAGAZINE - currentAmmo;
        if (needed > reserveAmmo)
            needed = reserveAmmo;

        currentAmmo += needed;
        reserveAmmo -= needed;
    }
}

// --- Texto ---
void drawText(float x, float y, const char *text, float escala)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(escala, escala, 1.0f);
    glLineWidth(2.0f);

    for (const char *c = text; *c != '\0'; c++)
    {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    }

    glPopMatrix();
}

// --- ATUALIZA O FOGO (CRIA E MOVE) ---
void atualizaFogo()
{
    // 1. CRIAR NOVAS PARTÍCULAS (Nascem no chão)
    // Aumente 'novasParticulas' para ter mais fogo
    int novasParticulas = 20;

    for (int i = 0; i < novasParticulas; i++)
    {
        ParticulaFogo p;
        p.x = (rand() % janelaW);               // Espalha na largura da tela
        p.y = 0;                                // Nasce no chão
        p.velY = 2.0f + ((rand() % 15) / 5.0f); // Sobe rápido
        p.vida = 1.0f;                          // Vida cheia
        p.tamanho = 15.0f + (rand() % 25);      // Tamanho variado

        // Cor: Varia entre Vermelho e Laranja
        p.r = 1.0f;
        p.g = (rand() % 150) / 255.0f; // Um pouco de verde cria laranja/amarelo
        p.b = 0.0f;

        fogo.push_back(p);
    }

    // 2. MOVER E ENVELHECER
    for (size_t i = 0; i < fogo.size(); i++)
    {
        fogo[i].y += fogo[i].velY;       // Sobe
        fogo[i].vida -= 0.015f;          // Envelhece e some
        fogo[i].x += ((rand() % 5) - 2); // Treme para os lados (efeito calor)
        fogo[i].tamanho *= 0.98f;        // Diminui um pouco enquanto sobe

        // Se morreu, remove da lista
        if (fogo[i].vida <= 0.0f)
        {
            fogo.erase(fogo.begin() + i);
            i--;
        }
    }
}

// --- DESENHA O FOGO (COM BRILHO) ---
void desenhaFogo()
{
    glDisable(GL_TEXTURE_2D);

    // O SEGREDO: Blending Aditivo (GL_ONE)
    // As cores se somam: Vermelho + Vermelho = Amarelo Brilhante
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBegin(GL_QUADS);
    for (auto &p : fogo)
    {
        // Alfa baseada na vida (some suavemente)
        glColor4f(p.r, p.g, p.b, p.vida * 0.6f);

        float t = p.tamanho;
        glVertex2f(p.x - t, p.y - t);
        glVertex2f(p.x + t, p.y - t);
        glVertex2f(p.x + t, p.y + t);
        glVertex2f(p.x - t, p.y + t);
    }
    glEnd();

    // Volta ao normal para não estragar o resto do jogo
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// --- BARRA ESTILO DOOM (PROPORÇÃO CORRIGIDA) ---
void drawDoomBar()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Configurações para desenho 2D
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float hBar = janelaH * 0.10f;

    // =========================================================
    // 1. FUNDO DO HUD (PERSONALIZADO E CORRIGIDO)
    // =========================================================
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texHudFundo); // Usa a sua imagem (meu_hud.png)

    // --- CORREÇÃO DE TEXTURA ESTICADA ---
    // Configura a imagem para REPETIR (azulejo) em vez de esticar
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Ajuste este número para controlar quantas vezes a imagem repete
    // Se ainda parecer esticada, aumente para 8.0 ou 10.0
    float repeticaoX = 6.0f;
    float repeticaoY = 1.0f;

    glColor3f(1.0f, 1.0f, 1.0f); // Branco puro para manter as cores originais da imagem

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, 0);
    glTexCoord2f(repeticaoX, 0.0f);
    glVertex2f(janelaW, 0);
    glTexCoord2f(repeticaoX, repeticaoY);
    glVertex2f(janelaW, hBar);
    glTexCoord2f(0.0f, repeticaoY);
    glVertex2f(0, hBar);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Bordas (Linhas decorativas)
    glLineWidth(3.0f);
    glColor3f(0.7f, 0.7f, 0.75f); // Cinza claro
    glBegin(GL_LINES);
    glVertex2f(0, hBar);
    glVertex2f(janelaW, hBar);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.25f); // Cinza escuro (Divisória central)
    glBegin(GL_LINES);
    glVertex2f(janelaW / 2.0f, 0);
    glVertex2f(janelaW / 2.0f, hBar);
    glEnd();

    // Configuração de Fontes e Escalas
    float scaleLbl = 0.0018f * hBar;
    float scaleNum = 0.0035f * hBar;
    float colLbl[3] = {1.0f, 0.8f, 0.5f}; // Dourado
    float colNum[3] = {0.8f, 0.0f, 0.0f}; // Vermelho Sangue

    // =========================================================
    // 2. HEALTH (VIDA)
    // =========================================================
    float yLblHealth = hBar * 0.35f;
    float xTextHealth = janelaW * 0.08f;
    glColor3fv(colLbl);
    glLineWidth(2.0f);
    drawText(xTextHealth, yLblHealth, "HEALTH", scaleLbl);

    float barH = hBar * 0.5f;
    float barY = (hBar - barH) / 2.0f;
    float barX = xTextHealth + (janelaW * 0.08f);
    float barMaxW = (janelaW * 0.45f) - barX;

    // Fundo preto da barra de vida
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + barMaxW, barY);
    glVertex2f(barX + barMaxW, barY + barH);
    glVertex2f(barX, barY + barH);
    glEnd();

    // Cálculo da cor baseada na vida
    float porcentagem = (float)playerHealth / 100.0f;
    if (porcentagem < 0.0f)
        porcentagem = 0.0f;
    if (porcentagem > 1.0f)
        porcentagem = 1.0f;

    if (porcentagem > 0.6f)
        glColor3f(0.0f, 0.8f, 0.0f); // Verde
    else if (porcentagem > 0.3f)
        glColor3f(1.0f, 0.8f, 0.0f); // Amarelo
    else
        glColor3f(0.8f, 0.0f, 0.0f); // Vermelho

    // Barra colorida preenchida
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + (barMaxW * porcentagem), barY);
    glVertex2f(barX + (barMaxW * porcentagem), barY + barH);
    glVertex2f(barX, barY + barH);
    glEnd();

    // =========================================================
    // 3. ARMA PIXEL ART (SHOTGUN)
    // =========================================================
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1.0f, 1.0f, 1.0f);

    float iconSize = hBar * 1.5f;
    float iconY = (hBar - iconSize) / 2.0f + (hBar * 0.1f);

    // Usa a textura da Shotgun que carregámos
    glBindTexture(GL_TEXTURE_2D, gAssets.texGunHUD);

    // Garante que fique "pixelado" (Retro style)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float weaponWidth = iconSize * 2.2f;
    float xIconGun = (janelaW * 0.75f) - (weaponWidth / 2.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(xIconGun, iconY);
    glTexCoord2f(1, 1);
    glVertex2f(xIconGun + weaponWidth, iconY);
    glTexCoord2f(1, 0);
    glVertex2f(xIconGun + weaponWidth, iconY + iconSize);
    glTexCoord2f(0, 0);
    glVertex2f(xIconGun, iconY + iconSize);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    // =========================================================
    // 4. TEXTOS DE MUNIÇÃO
    // =========================================================
    float xAmmoBlock = xIconGun + weaponWidth + 10.0f;
    float yNum = hBar * 0.50f;
    float deslocamentoNumero = 5.0f;
    float xNumAdjusted = xAmmoBlock + deslocamentoNumero;

    // Desenha o número da munição
    glColor3fv(colNum);
    glLineWidth(4.0f);
    glPushMatrix();
    glTranslatef(xNumAdjusted, yNum, 0);
    glScalef(scaleNum, scaleNum, 1.0f);
    {
        std::string s = std::to_string(currentAmmo);
        for (char c : s)
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    glPopMatrix();

    // Desenha o rótulo "AMMO"
    float yLblAmmo = hBar * 0.20f;
    glColor3fv(colLbl);
    glLineWidth(2.0f);
    drawText(xAmmoBlock, yLblAmmo, "AMMO", scaleLbl);

    // Restaura as matrizes e atributos originais
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void drawWeaponHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint currentTex = texGunDefault;
    if (weaponState == W_FIRE_1 || weaponState == W_RETURN)
        currentTex = texGunFire1;
    else if (weaponState == W_FIRE_2)
        currentTex = texGunFire2;
    else if (weaponState == W_RELOAD_1 || weaponState == W_RELOAD_3)
        currentTex = texGunReload1;
    else if (weaponState == W_RELOAD_2)
        currentTex = texGunReload2;

    if (currentTex == 0)
    {
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        return;
    }

    glBindTexture(GL_TEXTURE_2D, currentTex);
    glColor4f(1, 1, 1, 1);

    float gunH = janelaH * 0.5f;
    float gunW = gunH;
    float x = (janelaW - gunW) / 2.0f;
    float y = 0.0f;

    if (weaponState != W_IDLE)
    {
        y -= 20.0f;
        x += (rand() % 10 - 5);
    }

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + gunW, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + gunW, y + gunH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y + gunH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawDamageOverlay()
{
    if (damageAlpha <= 0.0f)
        return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, texDamage);
    glColor4f(1.0f, 1.0f, 1.0f, damageAlpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(janelaW, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(janelaW, janelaH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, janelaH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawHealthOverlay()
{
    if (healthAlpha <= 0.0f)
        return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, texHealthOverlay);
    glColor4f(1.0f, 1.0f, 1.0f, healthAlpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(janelaW, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(janelaW, janelaH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, janelaH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static bool isLavaTile(int tx, int tz)
{
    const auto &data = gLevel.map.data();
    if (tz < 0 || tz >= (int)data.size())
        return false;
    if (tx < 0 || tx >= (int)data[tz].size())
        return false;
    return data[tz][tx] == 'L';
}

static bool nearestLava(float px, float pz, float &outX, float &outZ, float &outDist)
{
    float tile = gLevel.metrics.tile;
    float offX = gLevel.metrics.offsetX;
    float offZ = gLevel.metrics.offsetZ;

    int ptx = (int)((px - offX) / tile);
    int ptz = (int)((pz - offZ) / tile);

    const int R = 10;
    bool found = false;
    float bestD2 = 1e30f;
    float bestX = 0, bestZ = 0;

    for (int dz = -R; dz <= R; ++dz)
    {
        for (int dx = -R; dx <= R; ++dx)
        {
            int tx = ptx + dx;
            int tz = ptz + dz;
            if (!isLavaTile(tx, tz))
                continue;

            float cx = offX + (tx + 0.5f) * tile;
            float cz = offZ + (tz + 0.5f) * tile;

            float ddx = cx - px;
            float ddz = cz - pz;
            float d2 = ddx * ddx + ddz * ddz;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestX = cx;
                bestZ = cz;
                found = true;
            }
        }
    }

    if (!found)
        return false;

    outX = bestX;
    outZ = bestZ;
    outDist = std::sqrt(bestD2);
    return true;
}

// Reinicia o jogo
void gameReset()
{
    playerHealth = 100;

    // Resetar munição (precisa acessar as variaveis globais)
    extern int currentAmmo, reserveAmmo;
    currentAmmo = 12;
    reserveAmmo = 25;

    // Resetar efeitos visuais
    extern float damageAlpha, healthAlpha;
    damageAlpha = 0.0f;
    healthAlpha = 0.0f;

    // Respawna o jogador
    applySpawn(gLevel, camX, camZ);
}

// Desenha a mira
void drawCrosshair()
{
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.0f, 1.0f, 0.0f); // Mira Verde
    glLineWidth(2.0f);

    float cx = janelaW / 2.0f;
    float cy = janelaH / 2.0f;
    float size = 10.0f;

    glBegin(GL_LINES);
    glVertex2f(cx - size, cy);
    glVertex2f(cx + size, cy);
    glVertex2f(cx, cy - size);
    glVertex2f(cx, cy + size);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

// --- MENU GENÉRICO / GAME OVER ---
void drawMenuScreen(std::string title, std::string subTitle)
{

    // 1. ATUALIZA A ANIMAÇÃO DO FOGO (Chama todo frame)
    atualizaFogo();

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // ZONA DE SEGURANÇA
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 2. FUNDO (Degradê Infernal)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
    glVertex2f(0, janelaH); // Topo vermelho
    glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
    glVertex2f(janelaW, janelaH);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glVertex2f(janelaW, 0); // Base preta
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glVertex2f(0, 0);
    glEnd();

    // 3. DESENHA O FOGO (ANTES DO TEXTO PARA FICAR NO FUNDO)
    desenhaFogo();

    glDisable(GL_BLEND);

    // 4. TÍTULO - VERSÃO GIGANTE E GROSSA

    // Configurações de Tamanho
    float scaleX = 1.0f; // Alargado horizontalmente
    float scaleY = 1.0f; // Altura normal

    // Medir largura base do texto
    float rawWidth = 0;
    for (char c : title)
        rawWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, c);

    float titleW = rawWidth * scaleX;
    float xBase = (janelaW - titleW) / 2.0f;
    float yBase = (janelaH / 2.0f) + 40.0f;

    // Função local para desenhar camadas grossas
    auto drawThickLayer = [&](float x, float y, float spread, float r, float g, float b)
    {
        glColor3f(r, g, b);
        for (float dy = -spread; dy <= spread; dy += 1.5f)
        {
            for (float dx = -spread; dx <= spread; dx += 1.5f)
            {
                glPushMatrix();
                glTranslatef(x + dx, y + dy, 0);
                glScalef(scaleX, scaleY, 1);
                for (char c : title)
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
                glPopMatrix();
            }
        }
    };

    // CAMADA 1: Sombra
    drawThickLayer(xBase + 10.0f, yBase - 10.0f, 4.0f, 0.0f, 0.0f, 0.0f);

    // CAMADA 2: Corpo
    drawThickLayer(xBase + 5.0f, yBase - 5.0f, 3.0f, 0.5f, 0.0f, 0.0f);

    // CAMADA 3: Frente
    drawThickLayer(xBase, yBase, 1.5f, 1.0f, 0.1f, 0.1f);

    // 5. SUBTÍTULO
    float scaleSub = 0.22f;
    glLineWidth(3.0f);

    float wSub = 0;
    for (char c : subTitle)
        wSub += glutStrokeWidth(GLUT_STROKE_ROMAN, c);
    wSub *= scaleSub;
    float xSub = (janelaW - wSub) / 2.0f;
    float ySub = (janelaH / 2.0f) - 90.0f;

    // Pisca Amarelo/Branco
    if ((int)(tempo * 3) % 2 == 0)
        glColor3f(1, 1, 1);
    else
        glColor3f(1, 1, 0);

    for (float d = 0; d <= 1.0f; d += 1.0f)
    {
        glPushMatrix();
        glTranslatef(xSub + d, ySub - d, 0);
        glScalef(scaleSub, scaleSub, 1);
        for (char c : subTitle)
            glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
        glPopMatrix();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void gameUpdate(float dt)
{
    // 1. SE NÃO ESTIVER JOGANDO, NÃO RODA A LÓGICA DO JOGO
    if (currentState != JOGANDO)
    {
        return;
    }

    tempo += dt;
    atualizaMovimento();

    AudioListener L;
    L.pos = {camX, camY, camZ};
    {
        float ry = yaw * 3.14159f / 180.0f;
        float rp = pitch * 3.14159f / 180.0f;
        L.forward = {cosf(rp) * sinf(ry), sinf(rp), -cosf(rp) * cosf(ry)};
    }
    L.up = {0.0f, 1.0f, 0.0f};
    L.vel = {0.0f, 0.0f, 0.0f};

    bool moving = (keyW || keyA || keyS || keyD);
    audioUpdate(gAudioSys, gLevel, L, dt, moving, playerHealth);

    if (damageAlpha > 0.0f)
    {
        damageAlpha -= dt * 0.5f;
        if (damageAlpha < 0.0f)
            damageAlpha = 0.0f;
    }
    if (healthAlpha > 0.0f)
    {
        healthAlpha -= dt * 0.9f;
        if (healthAlpha < 0.0f)
            healthAlpha = 0.0f;
    }

    updateEntities(dt);
    updateWeaponAnim(dt);
  
    
    // 3. CHECAGEM DE GAME OVER
    if (playerHealth <= 0)
    {
        currentState = GAME_OVER;
        damageAlpha = 1.0f; // Tela vermelha ao morrer
    }
}

// Função auxiliar para desenhar o mundo 3D (Inimigos, Mapa, Céu)
void drawWorld3D()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // LIGAR O 3D
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    // Configuração da Câmera
    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;
    float dirX = cosf(radPitch) * sinf(radYaw);
    float dirY = sinf(radPitch);
    float dirZ = -cosf(radPitch) * cosf(radYaw);
    gluLookAt(camX, camY, camZ, camX + dirX, camY + dirY, camZ + dirZ, 0.0f, 1.0f, 0.0f);

    // Desenha o cenário
    setSunDirectionEachFrame();
    drawSkydome(camX, camY, camZ);
    drawLevel(gLevel.map, camX, camZ, dirX, dirZ);
    drawEntities(gLevel.enemies, gLevel.items, camX, camZ, dirX, dirZ);
}

// --- MENU DE PAUSE (CENTRALIZADO) ---
void drawPauseMenu()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Configura 2D
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 1. FILTRO ESCURO TRANSPARENTE
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f); // 60% Preto
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(janelaW, 0);
    glVertex2f(janelaW, janelaH);
    glVertex2f(0, janelaH);
    glEnd();
    glDisable(GL_BLEND);

    // 2. TÍTULO "PAUSADO"
    std::string txt = "PAUSADO";
    float scale = 0.6f;
    glLineWidth(5.0f); // Borda grossa

    // --- CÁLCULO EXATO DA LARGURA ---
    float wTotal = 0;
    for (char c : txt)
        wTotal += glutStrokeWidth(GLUT_STROKE_ROMAN, c);
    wTotal *= scale; // Ajusta pela escala

    float x = (janelaW - wTotal) / 2.0f;
    float y = (janelaH / 2.0f) + 20.0f;

    // Sombra Preta
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(x + 3, y - 3, 0);
    glScalef(scale, scale, 1);
    for (char c : txt)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    glPopMatrix();

    // Texto Branco
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    for (char c : txt)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    glPopMatrix();

    // 3. SUBTÍTULO
    std::string sub = "Pressione P para Voltar";
    float scaleSub = 0.22f;
    glLineWidth(3.0f);

    // Cálculo Exato Subtítulo
    float wSub = 0;
    for (char c : sub)
        wSub += glutStrokeWidth(GLUT_STROKE_ROMAN, c);
    wSub *= scaleSub;

    float xSub = (janelaW - wSub) / 2.0f;
    float ySub = (janelaH / 2.0f) - 60.0f;

    // Pisca Amarelo
    if ((int)(tempo * 3) % 2 == 0)
        glColor3f(1, 1, 0);
    else
        glColor3f(1, 1, 1);

    glPushMatrix();
    glTranslatef(xSub, ySub, 0);
    glScalef(scaleSub, scaleSub, 1);
    for (char c : sub)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    glPopMatrix();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

// FUNÇÃO PRINCIPAL DE DESENHO
void gameRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- ESTADO: MENU INICIAL ---
    if (currentState == MENU_INICIAL)
    {
        drawMenuScreen("DOOM LIKE", "Pressione ENTER para Jogar");
    }
    // --- ESTADO: GAME OVER ---
    else if (currentState == GAME_OVER)
    {
        // No Game Over, desenhamos o fundo 3D estático para ficar bonito
        drawWorld3D();
        drawDamageOverlay();

        // Mantém a barra visível no Game Over
        drawDoomBar();

        drawMenuScreen("GAME OVER", "Pressione ENTER para Reiniciar");
    }
    // --- ESTADO: PAUSADO ---
    else if (currentState == PAUSADO)
    {
        // 1. Desenha o jogo congelado no fundo
        drawWorld3D();

        // 2. Desenha o HUD
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        drawWeaponHUD();

        // Desenha a barra
        drawDoomBar();

        glPopAttrib();

        // 3. Desenha o menu escuro por cima
        drawPauseMenu();
    }
    // --- ESTADO: JOGANDO ---
    else
    {
        // 1. Mundo 3D
        drawWorld3D();

        // 2. HUD (Arma, Vida, Mira, Sangue)
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        drawWeaponHUD();

        // Desenha a barra clássica de novo!
        drawDoomBar();

        drawCrosshair();
        drawDamageOverlay();
        glPopAttrib();
    }

    glutSwapBuffers();
}