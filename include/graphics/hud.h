#pragma once
#include "core/game_state.h"

void hudDrawWeapon(const GameContext& g, int janelaW, int janelaH);
void hudDrawDoomBar(const GameContext& g, int janelaW, int janelaH);
void hudDrawCrosshair(int janelaW, int janelaH);
void hudDrawDamageOverlay(const GameContext& g, int janelaW, int janelaH);
void hudDrawHealthOverlay(const GameContext& g, int janelaW, int janelaH);
