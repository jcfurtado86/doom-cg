#pragma once
#include "level/level.h"

// 1. Definimos os estados aqui para serem visíveis no input.cpp
enum GameState {
    MENU_INICIAL,
    JOGANDO,
    PAUSADO,
    GAME_OVER
};

// 2. Declaramos que essas variáveis existem (extern) para uso global
extern GameState currentState;
extern int playerHealth; 

void audioPlayStepTap();
extern Level gLevel;

bool gameInit(const char* mapPath);
void gameUpdate(float dt);
void gameRender();
void playerTryAttack();
void playerTryReload();

// Nova função para reiniciar o jogo ao morrer
void gameReset();