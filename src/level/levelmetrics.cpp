#include "level/levelmetrics.h"
#include <cmath>

LevelMetrics LevelMetrics::fromMap(const MapLoader& map, float tile)
{
    LevelMetrics m;
    m.tile = tile;

    int W = map.getWidth();
    int H = map.getHeight();

    // mesma fórmula do draw
    m.offsetX = -(W * tile) * 0.5f;
    m.offsetZ = -(H * tile) * 0.5f;

    return m;
}

void LevelMetrics::tileCenter(int tileX, int tileZ, float& outX, float& outZ) const
{
    // +0.5 para ir pro centro do tile
    outX = offsetX + (tileX + 0.5f) * tile;
    outZ = offsetZ + (tileZ + 0.5f) * tile;
}

void LevelMetrics::spawnPos(const MapLoader& map, float& outX, float& outZ) const
{
    int sx = (int)map.getPlayerStartX();
    int sz = (int)map.getPlayerStartZ();
    tileCenter(sx, sz, outX, outZ);
}
