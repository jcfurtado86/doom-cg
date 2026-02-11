#include "audio/audio_system.h"
#include "level/level.h"
#include "core/entities.h" // Enemy states, etc.

#include <cstdio>
#include <cmath>
#include <cstdlib>

// ---------------- helpers internos ----------------

static inline float frand01() {
    return (float)std::rand() / (float)RAND_MAX;
}

static void stopIf(ALuint s, AudioEngine& e) {
    if (s) e.stop(s);
}

static void play2D(AudioSystem& a, ALuint s) {
    if (!a.ok || s == 0) return;
    a.engine.stop(s);
    a.engine.play(s);
}

static void play3DAt(AudioSystem& a, ALuint s, float x, float z) {
    if (!a.ok || s == 0) return;
    a.engine.setSourcePos(s, {x, 0.0f, z});
    a.engine.stop(s);
    a.engine.play(s);
}

// Checa se é lava no tile
static bool isLavaTile(const Level& level, int tx, int tz) {
    const auto& data = level.map.data();
    if (tz < 0 || tz >= (int)data.size()) return false;
    if (tx < 0 || tx >= (int)data[tz].size()) return false;
    return data[tz][tx] == 'L';
}

// Acha lava mais próxima (busca local)
static bool nearestLava(const Level& level, float px, float pz, float& outX, float& outZ, float& outDist) {
    float tile = level.metrics.tile;
    float offX = level.metrics.offsetX;
    float offZ = level.metrics.offsetZ;

    int ptx = (int)((px - offX) / tile);
    int ptz = (int)((pz - offZ) / tile);

    const int R = 10;
    bool found = false;
    float bestD2 = 1e30f;
    float bestX = 0.0f, bestZ = 0.0f;

    for (int dz = -R; dz <= R; ++dz) {
        for (int dx = -R; dx <= R; ++dx) {
            int tx = ptx + dx;
            int tz = ptz + dz;
            if (!isLavaTile(level, tx, tz)) continue;

            float cx = offX + (tx + 0.5f) * tile;
            float cz = offZ + (tz + 0.5f) * tile;

            float ddx = cx - px;
            float ddz = cz - pz;
            float d2 = ddx * ddx + ddz * ddz;

            if (d2 < bestD2) {
                bestD2 = d2;
                bestX = cx;
                bestZ = cz;
                found = true;
            }
        }
    }

    if (!found) return false;

    outX = bestX;
    outZ = bestZ;
    outDist = std::sqrt(bestD2);
    return true;
}

// Cria/atualiza arrays de sources de inimigos conforme quantidade no level
static void ensureEnemySources(AudioSystem& a, const Level& level) {
    if (!a.ok || a.bufEnemy == 0) return;

    const size_t need = level.enemies.size();
    if (a.srcEnemies.size() == need) return;

    for (ALuint s : a.srcEnemies) stopIf(s, a.engine);
    a.srcEnemies.assign(need, 0);

    for (size_t i = 0; i < need; ++i) {
        ALuint s = a.engine.createSource(a.bufEnemy, true);
        if (!s) continue;

        alSourcei(s, AL_SOURCE_RELATIVE, AL_FALSE);
        a.engine.setSourceDistance(s,
            AudioTuning::ENEMY_REF_DIST,
            AudioTuning::ENEMY_ROLLOFF,
            AudioTuning::ENEMY_MAX_DIST
        );
        a.engine.setSourceGain(s, AudioTuning::MASTER * AudioTuning::ENEMY_BASE_GAIN);
        a.engine.stop(s);

        a.srcEnemies[i] = s;
    }
}

static void ensureEnemyExtra(AudioSystem& a, const Level& level) {
    if (!a.ok) return;

    const size_t n = level.enemies.size();

    if (a.srcEnemyScreams.size() != n) {
        for (ALuint s : a.srcEnemyScreams) stopIf(s, a.engine);
        a.srcEnemyScreams.assign(n, 0);
        a.enemyScreamTimer.assign(n, 0.0f);
    }

    if (a.enemyPrevState.size() != n) {
        a.enemyPrevState.assign(n, 0);
    }

    // Se temos buffer de scream, garante 1 source por inimigo
    if (a.bufEnemyScream && a.srcEnemyScreams.size() == n) {
        for (size_t i = 0; i < n; ++i) {
            if (a.srcEnemyScreams[i] != 0) continue;

            ALuint s = a.engine.createSource(a.bufEnemyScream, false);
            if (!s) continue;

            alSourcei(s, AL_SOURCE_RELATIVE, AL_FALSE);
            a.engine.setSourceGain(s, AudioTuning::MASTER * AudioTuning::ENEMY_SCREAM_GAIN);
            a.engine.setSourceDistance(
                s,
                AudioTuning::ENEMY_REF_DIST,
                AudioTuning::ENEMY_ROLLOFF * 0.8f,
                AudioTuning::SCREAM_MAX_AUDIBLE_DIST
            );

            a.srcEnemyScreams[i] = s;

            // agenda inicial
            float r = frand01();
            a.enemyScreamTimer[i] =
                AudioTuning::ENEMY_SCREAM_MIN_INTERVAL +
                r * (AudioTuning::ENEMY_SCREAM_MAX_INTERVAL - AudioTuning::ENEMY_SCREAM_MIN_INTERVAL);
        }
    }
}

// ---------------- API ----------------

void audioInit(AudioSystem& a, const Level& level) {
    if (a.ok) return;

    a.ok = a.engine.init();
    if (!a.ok) {
        std::printf("[Audio] Falha ao iniciar OpenAL (seguindo sem audio)\n");
        return;
    }

    a.engine.setDistanceModel();

    // Loads (com fallback mono/estéreo)
    a.bufAmbient = a.engine.loadWav("assets/audio/ambient_mono.wav");
    if (!a.bufAmbient) a.bufAmbient = a.engine.loadWav("assets/audio/ambient.wav");

    a.bufShot = a.engine.loadWav("assets/audio/shot_mono.wav");
    if (!a.bufShot) a.bufShot = a.engine.loadWav("assets/audio/shot.wav");

    a.bufStep = a.engine.loadWav("assets/audio/step_mono.wav");
    if (!a.bufStep) a.bufStep = a.engine.loadWav("assets/audio/step.wav");

    a.bufEnemy = a.engine.loadWav("assets/audio/enemy_mono.wav");
    if (!a.bufEnemy) a.bufEnemy = a.engine.loadWav("assets/audio/enemy.wav");

    a.bufReload = a.engine.loadWav("assets/audio/reload_mono.wav");
    if (!a.bufReload) a.bufReload = a.engine.loadWav("assets/audio/reload.wav");

    a.bufClickReload = a.engine.loadWav("assets/audio/click_reload_mono.wav");

    a.bufKill = a.engine.loadWav("assets/audio/kill_mono.wav");

    a.bufEnemyScream = a.engine.loadWav("assets/audio/enemy_scream_mono.wav");

    a.bufHurt = a.engine.loadWav("assets/audio/hurt_mono.wav");
    if (!a.bufHurt) a.bufHurt = a.engine.loadWav("assets/audio/hurt.wav");

    a.bufLava = a.engine.loadWav("assets/audio/lava_mono.wav");
    if (!a.bufLava) a.bufLava = a.engine.loadWav("assets/audio/lava.wav");

    a.bufBreath = a.engine.loadWav("assets/audio/breath_mono.wav");

    a.bufGrunt = a.engine.loadWav("assets/audio/grunt_mono.wav");
    if (!a.bufGrunt) a.bufGrunt = a.engine.loadWav("assets/audio/grunt.wav");

    // Ambient (2D loop)
    if (a.bufAmbient) {
        a.srcAmbient = a.engine.createSource(a.bufAmbient, true);
        alSourcei(a.srcAmbient, AL_SOURCE_RELATIVE, AL_TRUE);
        a.engine.setSourcePos(a.srcAmbient, {0.0f, 0.0f, 0.0f});
        a.engine.setSourceDistance(a.srcAmbient, 1.0f, 0.0f, 1000.0f);
        a.engine.setSourceGain(a.srcAmbient, AudioTuning::MASTER * AudioTuning::AMBIENT_GAIN);
        a.engine.play(a.srcAmbient);
    }

    // Shot (2D one-shot)
    if (a.bufShot) {
        a.srcShot = a.engine.createSource(a.bufShot, false);
        alSourcei(a.srcShot, AL_SOURCE_RELATIVE, AL_TRUE);
        a.engine.setSourceGain(a.srcShot, AudioTuning::MASTER * AudioTuning::SHOT_GAIN);
    }

    // Step (2D loop, controlado no update)
    if (a.bufStep) {
        a.srcStep = a.engine.createSource(a.bufStep, true);
        alSourcei(a.srcStep, AL_SOURCE_RELATIVE, AL_TRUE);
        a.engine.setSourceGain(a.srcStep, AudioTuning::MASTER * AudioTuning::STEP_GAIN);
    }

    // Reload (2D one-shot)
    if (a.bufReload) {
        a.srcReload = a.engine.createSource(a.bufReload, false);
        alSourcei(a.srcReload, AL_SOURCE_RELATIVE, AL_TRUE);
        a.engine.setSourceGain(a.srcReload, AudioTuning::MASTER * AudioTuning::RELOAD_GAIN);
    }

    // Pump click (2D one-shot)
    if (a.bufClickReload) {
        a.srcClickReload = a.engine.createSource(a.bufClickReload, false);
        if (a.srcClickReload) {
            alSourcei(a.srcClickReload, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(a.srcClickReload, AL_POSITION, 0, 0, 0);
            a.engine.setSourceGain(a.srcClickReload, AudioTuning::MASTER * AudioTuning::PUMP_GAIN);
        }
    }

    // Kill (3D one-shot)
    if (a.bufKill) {
        a.srcKill = a.engine.createSource(a.bufKill, false);
        if (a.srcKill) {
            alSourcei(a.srcKill, AL_SOURCE_RELATIVE, AL_FALSE);
            a.engine.setSourceGain(a.srcKill, AudioTuning::MASTER * AudioTuning::KILL_GAIN);
            a.engine.setSourceDistance(a.srcKill,
                AudioTuning::ENEMY_REF_DIST,
                AudioTuning::ENEMY_ROLLOFF,
                AudioTuning::ENEMY_MAX_DIST
            );
        }
    }

    // Hurt (2D one-shot)
    if (a.bufHurt) {
        a.srcHurt = a.engine.createSource(a.bufHurt, false);
        alSourcei(a.srcHurt, AL_SOURCE_RELATIVE, AL_TRUE);
        a.engine.setSourceGain(a.srcHurt, AudioTuning::MASTER * AudioTuning::DAMAGE_GAIN);
    }

    // Lava (3D loop start/stop no update)
    if (a.bufLava) {
        a.srcLava = a.engine.createSource(a.bufLava, true);
        if (a.srcLava) {
            alSourcei(a.srcLava, AL_SOURCE_RELATIVE, AL_FALSE);
            a.engine.setSourceDistance(a.srcLava, 6.0f, 0.8f, 25.0f);
            a.engine.setSourceGain(a.srcLava, AudioTuning::MASTER * AudioTuning::LAVA_GAIN);
            a.engine.stop(a.srcLava);
            a.lavaPlaying = false;
        }
    }

    // Breath (2D loop sempre tocando, ganho ajustado no update)
    if (a.bufBreath) {
        a.srcBreath = a.engine.createSource(a.bufBreath, true);
        if (a.srcBreath) {
            alSourcei(a.srcBreath, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(a.srcBreath, AL_POSITION, 0.0f, 0.0f, 0.0f);
            a.engine.setSourceGain(a.srcBreath, 0.0f);
            a.engine.play(a.srcBreath);
        }
    }

    // Grunt (2D one-shot)
    if (a.bufGrunt) {
        a.srcGrunt = a.engine.createSource(a.bufGrunt, false);
        if (a.srcGrunt) {
            alSourcei(a.srcGrunt, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(a.srcGrunt, AL_POSITION, 0.0f, 0.0f, 0.0f);
            a.engine.setSourceGain(a.srcGrunt, AudioTuning::MASTER * AudioTuning::GRUNT_GAIN);
        }
    }

    // Inimigos
    ensureEnemySources(a, level);
    ensureEnemyExtra(a, level);
}

void audioUpdate(
    AudioSystem& a,
    const Level& level,
    const AudioListener& listener,
    float dt,
    bool playerMoving,
    int playerHp
) {
    if (!a.ok) return;

    // Listener
    a.engine.setListener(listener.pos, listener.vel, listener.forward, listener.up);

    // Step
    if (a.srcStep) {
        if (playerMoving && !a.stepPlaying) {
            alSourcei(a.srcStep, AL_SOURCE_RELATIVE, AL_TRUE);
            a.engine.setSourcePos(a.srcStep, {0.0f, 0.0f, 0.0f});
            a.engine.setSourceDistance(a.srcStep, 1.0f, 0.0f, 1000.0f);
            a.engine.play(a.srcStep);
            a.stepPlaying = true;
        } else if (!playerMoving && a.stepPlaying) {
            a.engine.stop(a.srcStep);
            a.stepPlaying = false;
        }
    }

    // Enemy loops
    ensureEnemySources(a, level);
    for (size_t i = 0; i < level.enemies.size() && i < a.srcEnemies.size(); ++i) {
        ALuint s = a.srcEnemies[i];
        if (!s) continue;

        const auto& en = level.enemies[i];
        if (en.state == STATE_DEAD) {
            a.engine.stop(s);
            continue;
        }

        a.engine.setSourcePos(s, {en.x, 0.0f, en.z});

        float dx = en.x - listener.pos.x;
        float dz = en.z - listener.pos.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        ALint st = 0;
        alGetSourcei(s, AL_SOURCE_STATE, &st);
        const bool playing = (st == AL_PLAYING);

        if (!playing && dist <= AudioTuning::ENEMY_START_DIST) {
            a.engine.play(s);
        } else if (playing && dist >= AudioTuning::ENEMY_STOP_DIST) {
            a.engine.stop(s);
        }
    }

    // Kill detect + screams
    ensureEnemyExtra(a, level);

    // kill detect
    for (size_t i = 0; i < level.enemies.size(); ++i) {
        const auto& en = level.enemies[i];
        if (a.enemyPrevState[i] != STATE_DEAD && en.state == STATE_DEAD) {
            audioPlayKillAt(a, en.x, en.z);
        }
        a.enemyPrevState[i] = (int)en.state;
    }

    // screams
    if (a.bufEnemyScream && !a.srcEnemyScreams.empty()) {
        for (size_t i = 0; i < level.enemies.size() && i < a.srcEnemyScreams.size(); ++i) {
            const auto& en = level.enemies[i];
            if (en.state == STATE_DEAD) continue;

            // distância para audibilidade
            float dxs = en.x - listener.pos.x;
            float dzs = en.z - listener.pos.z;
            float ds = std::sqrt(dxs * dxs + dzs * dzs);

            // decai mais lento se muito longe
            if (ds > AudioTuning::SCREAM_MAX_AUDIBLE_DIST) {
                a.enemyScreamTimer[i] -= dt * 0.25f;
                continue;
            }

            a.enemyScreamTimer[i] -= dt;
            if (a.enemyScreamTimer[i] > 0.0f) continue;

            // chance de gritar
            if (frand01() <= AudioTuning::SCREAM_CHANCE) {
                ALuint s = a.srcEnemyScreams[i];
                if (s) {
                    a.engine.setSourcePos(s, {en.x, 0.0f, en.z});
                    a.engine.stop(s);
                    a.engine.setSourceGain(s, AudioTuning::MASTER * AudioTuning::ENEMY_SCREAM_GAIN);
                    a.engine.play(s);
                }
            }

            // re-agenda
            float tmin = AudioTuning::ENEMY_SCREAM_MIN_INTERVAL;
            float tmax = AudioTuning::ENEMY_SCREAM_MAX_INTERVAL;
            a.enemyScreamTimer[i] = tmin + (tmax - tmin) * frand01();
        }
    }

    // Lava loop start/stop
    if (a.srcLava) {
        float lx, lz, dist;
        bool hasLava = nearestLava(level, listener.pos.x, listener.pos.z, lx, lz, dist);

        const float LAVA_START = 8.0f;
        const float LAVA_STOP  = 9.5f;

        if (hasLava) {
            a.engine.setSourcePos(a.srcLava, {lx, 0.0f, lz});
            if (!a.lavaPlaying && dist <= LAVA_START) {
                a.engine.play(a.srcLava);
                a.lavaPlaying = true;
            } else if (a.lavaPlaying && dist >= LAVA_STOP) {
                a.engine.stop(a.srcLava);
                a.lavaPlaying = false;
            }
        } else {
            if (a.lavaPlaying) {
                a.engine.stop(a.srcLava);
                a.lavaPlaying = false;
            }
        }
    }

    // Breath gain (low HP)
    if (a.srcBreath) {
        if (playerHp > AudioTuning::LOW_HP_THRESHOLD) {
            a.engine.setSourceGain(a.srcBreath, 0.0f);
        } else {
            float t = (AudioTuning::LOW_HP_THRESHOLD - playerHp) / (float)AudioTuning::LOW_HP_THRESHOLD;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float gain = (AudioTuning::MASTER * AudioTuning::BREATH_GAIN) * (0.20f + t * 1.00f);
            a.engine.setSourceGain(a.srcBreath, gain);

            ALint st = 0;
            alGetSourcei(a.srcBreath, AL_SOURCE_STATE, &st);
            if (st != AL_PLAYING) a.engine.play(a.srcBreath);
        }
    }
}

void audioPlayShot(AudioSystem& a) {
    if (!a.ok || a.srcShot == 0) return;

    alSourcei(a.srcShot, AL_SOURCE_RELATIVE, AL_TRUE);
    a.engine.setSourcePos(a.srcShot, {0.0f, 0.0f, 0.0f});
    a.engine.setSourceDistance(a.srcShot, 1.0f, 0.0f, 1000.0f);

    a.engine.stop(a.srcShot);
    a.engine.play(a.srcShot);
}

void audioPlayReload(AudioSystem& a) {
    if (!a.ok || a.srcReload == 0) return;
    a.engine.stop(a.srcReload);
    a.engine.play(a.srcReload);
}

void audioPlayPumpClick(AudioSystem& a) {
    if (!a.ok || a.srcClickReload == 0) return;
    a.engine.stop(a.srcClickReload);
    a.engine.play(a.srcClickReload);
}

void audioPlayHurt(AudioSystem& a) {
    if (!a.ok || a.srcHurt == 0) return;
    a.engine.stop(a.srcHurt);
    a.engine.play(a.srcHurt);
}

void audioPlayKillAt(AudioSystem& a, float x, float z) {
    play3DAt(a, a.srcKill, x, z);
}

void audioOnPlayerShot(AudioSystem& a) {
    if (!a.ok) return;
    a.shotsSinceGrunt++;

    if (a.shotsSinceGrunt >= AudioTuning::GRUNT_EVERY_N_SHOTS) {
        a.shotsSinceGrunt = 0;
        if (a.srcGrunt) {
            a.engine.stop(a.srcGrunt);
            a.engine.play(a.srcGrunt);
        }
    }
}
