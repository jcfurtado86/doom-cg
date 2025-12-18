#ifndef LEVELMETRICS_H
#define LEVELMETRICS_H

#include "level/maploader.h"

struct LevelMetrics
{
    float tile;     // tamanho do tile no mundo
    float offsetX;  // offset para centralizar
    float offsetZ;

    static LevelMetrics fromMap(const MapLoader& map, float tile);

    // centro do tile (x,z) no mundo
    void tileCenter(int tileX, int tileZ, float& outX, float& outZ) const;

    // spawn (tile do '9') no centro do tile
    void spawnPos(const MapLoader& map, float& outX, float& outZ) const;
};

#endif
