#pragma once
#include "core/game_state.h"

enum WeaponState {
    W_IDLE, W_FIRE_1, W_FIRE_2, W_RETURN,
    W_PUMP,
    W_RELOAD_1, W_RELOAD_2, W_RELOAD_3
};

struct WeaponConfig {
    int maxMagazine = 12;
};

void weaponTryReload(GameContext& g, const WeaponConfig& cfg);
void weaponTryAttack(GameContext& g, const WeaponConfig& cfg, float camX, float camZ, float yawDeg);
void weaponUpdateAnim(GameContext& g, const WeaponConfig& cfg, float dt);
