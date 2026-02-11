#pragma once
#include <GL/glew.h>

struct GameAssets
{
    // texturas
    GLuint texChao = 0;
    GLuint texParede = 0;
    GLuint texSangue = 0;
    GLuint texLava = 0;
    GLuint texChaoInterno = 0;
    GLuint texParedeInterna = 0;
    GLuint texTeto = 0;
    GLuint texEnemy = 0;
    GLuint texEnemyRage = 0;    // Viu o player
    GLuint texEnemyDamage = 0;  // Leva dano
    GLuint texHealthOverlay = 0; // Tela de cura
    GLuint texHealth = 0;
    GLuint texAmmo = 0;
    GLuint texGunDefault = 0;
    GLuint texGunFire1 = 0;
    GLuint texGunFire2 = 0;
    GLuint texGunReload1 = 0;
    GLuint texGunReload2 = 0;
    GLuint texDamage = 0;
    GLuint texGunHUD = 0;
    GLuint texHudFundo = 0;

    GLuint texEnemies[5]       = {0, 0, 0, 0, 0};
    GLuint texEnemiesRage[5]   = {0, 0, 0, 0, 0};
    GLuint texEnemiesDamage[5] = {0, 0, 0, 0, 0};

    // shaders
    GLuint progSangue = 0;
    GLuint progLava = 0;

    GLuint texSkydome = 0;
};

bool loadAssets(GameAssets &a);
