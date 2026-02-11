#pragma once
#include "level/level.h"

bool isWalkable(const Level& lvl, float x, float z);

bool nearestLava(const Level& lvl, float px, float pz,
                 float& outX, float& outZ, float& outDist);
