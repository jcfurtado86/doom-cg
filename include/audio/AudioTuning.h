#pragma once

// Painel de ajuste rápido de volumes/distâncias.
// Valores padrão seguros (0.0f a ~2.0f). Ajuste ao gosto.

namespace AudioTuning
{
    // Master geral (multiplica tudo)
    static constexpr float MASTER = 1.0f;

    // ===== 2D (na "cabeça") =====
    // Aumentar o volume do som ambiente
    // 0.8f = um pouco mais alto
    // 1.0f = padrão “cheio"
    // 1.2f / 1.5f = bem alto (cuidado pra não estourar)
    static constexpr float AMBIENT_GAIN = 0.55f;
    static constexpr float SHOT_GAIN = 0.95f;
    static constexpr float STEP_GAIN = 0.55f;

    static constexpr float RELOAD_GAIN = 0.95f;
    static constexpr float PUMP_GAIN = 0.85f; // som do "pump/click" do shotgun
    static constexpr float PUMP_TIME = 0.75f; // segundos após o tiro para tocar o pump

    static constexpr float DAMAGE_GAIN = 1.00f;   // hurt do player
    static constexpr float GRUNT_GAIN = 1.20f;    // grunhido do player
    static constexpr int GRUNT_EVERY_N_SHOTS = 5; // a cada N tiros, toca um grunt

    // Respiração (low HP)
    static constexpr int LOW_HP_THRESHOLD = 40; // vida abaixo disso ativa respiração
    static constexpr float BREATH_GAIN = 0.95f;

    // ===== Lava (3D por proximidade) =====
    static constexpr float LAVA_GAIN = 0.85f;

    // ===== Inimigos 3D (loop base) =====
    static constexpr float ENEMY_BASE_GAIN = 0.75f; // ganho base do loop do inimigo

    // Atenuação (quanto dá pra ouvir e quão rápido cai)
    // refDist: até aqui fica "normal"
    // rolloff: quanto maior, mais rápido some
    // maxDist: depois disso praticamente não ouve
    static constexpr float ENEMY_REF_DIST = 1.20f;
    static constexpr float ENEMY_ROLLOFF = 2.85f;
    static constexpr float ENEMY_MAX_DIST = 7.00f;

    // Histerese de liga/desliga do loop (pra não ficar ligando/desligando toda hora)
    static constexpr float ENEMY_START_DIST = 5.50f;
    static constexpr float ENEMY_STOP_DIST = 6.50f;

    // ===== Grito aleatório do inimigo (3D) =====
    static constexpr float ENEMY_SCREAM_GAIN = 1.40f;         // volume do grito
    static constexpr float ENEMY_SCREAM_MIN_INTERVAL = 6.0f;  // mínimo entre gritos (s)
    static constexpr float ENEMY_SCREAM_MAX_INTERVAL = 14.0f; // máximo entre gritos (s)

    // Chance de gritar quando o timer zera (0..1)
    static constexpr float SCREAM_CHANCE = 0.80f;

    // Se estiver mais longe que isso, nem tenta tocar (economia + realismo)
    static constexpr float SCREAM_MAX_AUDIBLE_DIST = 20.0f;

    // ===== Som de kill (quando inimigo morre) =====
    static constexpr float KILL_GAIN = 1.00f;
}
