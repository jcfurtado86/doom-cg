#include <cmath>
#include "utils/utils.h"

float clampf(float v, float a, float b) {
    return (v < a) ? a : (v > b) ? b : v;
}

bool normalize2(float &x, float &z) {
    float len2 = x*x + z*z;
    if (len2 < 1e-8f) return false;
    float inv = 1.0f / std::sqrt(len2);
    x *= inv; z *= inv;
    return true;
}

// Retorna forward XZ normalizado a partir de dx,dz (que podem não estar normalizados).
// Se degenerar (ex: olhando quase 100% pra cima/baixo e seu dx,dz vierem ~0),
// retorna false para desabilitar culling angular.
bool getForwardXZ(float dx, float dz, float &outFx, float &outFz) {
    outFx = dx;
    outFz = dz;
    return normalize2(outFx, outFz);
}

float deg2rad(float d)
{
    return d * (3.1415926f / 180.0f);
}
