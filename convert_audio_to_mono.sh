#!/usr/bin/env bash

AUDIO_DIR="assets/audio"

echo "Convertendo WAVs para MONO (OpenAL friendly)..."

for file in ambient.wav shot.wav step.wav enemy.wav; do
    if [ -f "$AUDIO_DIR/$file" ]; then
        out="${file%.wav}_mono.wav"
        echo " -> $file -> $out"
        ffmpeg -y -i "$AUDIO_DIR/$file" \
            -ac 1 \
            -ar 44100 \
            -sample_fmt s16 \
            "$AUDIO_DIR/$out"
    else
        echo " !! $file não encontrado, pulando"
    fi
done

echo "Finalizado."
