#include <GL/glut.h>
#include <cmath>

#include "core/movement.h"
#include "input/keystate.h"
#include "core/camera.h"
#include "core/game.h"
#include "core/config.h"
#include "level/levelmetrics.h"
#include "core/camera.h"
#include "core/window.h"

static bool isWallTile(int tx, int tz)
{
    const auto &data = gLevel.map.data();
    if (tz < 0 || tx < 0 || tz >= gLevel.map.getHeight())
        return false;
    if (tx >= (int)data[tz].size())
        return false;

    char c = data[tz][tx];
    return (c == '1' || c == '2');
}

static bool pointIntersectsTile(float px, float pz, int tx, int tz, const LevelMetrics &m, float radius)
{
    float wx, wz;
    m.tileCenter(tx, tz, wx, wz);
    float half = m.tile * 0.5f;

    float minX = wx - half;
    float maxX = wx + half;
    float minZ = wz - half;
    float maxZ = wz + half;

    float closestX = std::fmax(minX, std::fmin(px, maxX));
    float closestZ = std::fmax(minZ, std::fmin(pz, maxZ));

    float dx = px - closestX;
    float dz = pz - closestZ;
    return (dx * dx + dz * dz) < (radius * radius);
}

static bool canMoveTo(float nx, float nz)
{
    if (gLevel.map.getHeight() == 0)
        return true;

    const LevelMetrics &m = gLevel.metrics;
    float tile = m.tile;
    float radius = tile * GameConfig::PLAYER_RADIUS_FACTOR;

    float localX = nx - m.offsetX;
    float localZ = nz - m.offsetZ;
    int tx = (int)std::floor(localX / tile);
    int tz = (int)std::floor(localZ / tile);

    for (int dz = -1; dz <= 1; ++dz)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int ntx = tx + dx;
            int ntz = tz + dz;
            if (!isWallTile(ntx, ntz))
                continue;

            if (pointIntersectsTile(nx, nz, ntx, ntz, m, radius))
                return false;
        }
    }

    return true;
}

void atualizaMovimento()
{
    float passo = GameConfig::PLAYER_STEPS;

    float radYaw = yaw * (float)M_PI / 180.0f;
    float dirX = std::sin(radYaw);
    float dirZ = -std::cos(radYaw);

    float strafeX = dirZ;
    float strafeZ = -dirX;

    float dx = 0.0f;
    float dz = 0.0f;

    if (keyW)
    {
        dx += dirX * passo;
        dz += dirZ * passo;
    }
    if (keyS)
    {
        dx -= dirX * passo;
        dz -= dirZ * passo;
    }
    if (keyA)
    {
        dx += strafeX * passo;
        dz += strafeZ * passo;
    }
    if (keyD)
    {
        dx -= strafeX * passo;
        dz -= strafeZ * passo;
    }

    if (dx == 0.0f && dz == 0.0f)
        return;

    // normaliza pra não andar mais rápido na diagonal
    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0f)
    {
        dx = (dx / len) * passo;
        dz = (dz / len) * passo;
    }

    // slide por eixo (menos “travadas”)
    float nx = camX + dx;
    if (canMoveTo(nx, camZ))
        camX = nx;

    float nz = camZ + dz;
    if (canMoveTo(camX, nz))
        camZ = nz;
}

void mouseMotion(int x, int y)
{
    if (ignoreWarp)
    {
        ignoreWarp = false;
        return;
    }

    if (firstMouse)
    {
        firstMouse = false;
        ignoreWarp = true;
        glutWarpPointer(centerX, centerY);
        return;
    }

    int dx = x - centerX;
    int dy = y - centerY;

    float sens = 0.1f;

    yaw += dx * sens;
    pitch -= dy * sens;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    ignoreWarp = true;
    glutWarpPointer(centerX, centerY);

    glutPostRedisplay();
}
