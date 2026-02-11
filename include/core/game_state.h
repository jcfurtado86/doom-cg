// core/game_state.h
#pragma once
#include "utils/assets.h"
#include "level/level.h"
#include "audio/AudioEngine.h" // ou audio/audio_system.h quando criar
#include <GL/glew.h>

enum GameState { MENU_INICIAL, JOGANDO, PAUSADO, GAME_OVER };

struct PlayerState {
    int health = 100;
    float damageAlpha = 0.0f;
    float healthAlpha = 0.0f;

    int currentAmmo = 12;
    int reserveAmmo = 25;
};

struct WeaponAnim {
    int state = 0;
    float timer = 0.0f;
};

struct RenderAssets {
    GLuint texChao = 0, texParede = 0, texSangue = 0, texLava = 0;
    GLuint texChaoInterno = 0, texParedeInterna = 0, texTeto = 0, texSkydome = 0;

    GLuint texEnemies[5] = {0};
    GLuint texEnemiesRage[5] = {0};
    GLuint texEnemiesDamage[5] = {0};

    GLuint texHealth = 0, texAmmo = 0;
    GLuint texGunHUD = 0, texHudFundo = 0;

    GLuint texGunDefault = 0, texGunFire1 = 0, texGunFire2 = 0;
    GLuint texGunReload1 = 0, texGunReload2 = 0;

    GLuint texDamage = 0;
    GLuint texHealthOverlay = 0;

    GLuint progSangue = 0;
    GLuint progLava = 0;
};

struct GameContext {
    GameAssets assets;
    RenderAssets r;

    Level level;

    GameState state = MENU_INICIAL;
    PlayerState player;
    WeaponAnim weapon;

    float time = 0.0f;
};
