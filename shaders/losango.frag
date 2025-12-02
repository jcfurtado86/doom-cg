#version 120

uniform sampler2D uTexture;
uniform float uTime;   // tempo em segundos
uniform float uStrength; // força da distorção
uniform vec2 uSpeed;     // velocidade das ondas

varying vec2 vTexCoord;

void main()
{
    // coordenadas base
    vec2 uv = vTexCoord;

    // ondas em duas direções diferentes
    float wave1 = sin(uv.y * 15.0 + uTime * uSpeed.x);
    float wave2 = cos(uv.x * 20.0 + uTime * uSpeed.y);

    // mistura as ondas
    float distort = wave1 * 0.5 + wave2 * 0.5;

    // aplica distorção nas UV
    vec2 uvDistorted = uv + uStrength * vec2(distort * 0.2, distort * 0.2);

    // amostra a textura
    vec4 baseColor = texture2D(uTexture, uvDistorted);

    // deixa um pouco mais vermelho e molhado
    vec3 waterTint = vec3(0.3, 0.0, 0.0);
    vec3 finalColor = mix(baseColor.rgb, waterTint, 0.25);

    // leve “brilho” dependendo da direção da onda
    float highlight = 0.3 * abs(wave1 * wave2);
    finalColor += highlight * vec3(0.2, 0.3, 0.4);

    gl_FragColor = vec4(finalColor, baseColor.a);
    // debug: mostra as UV na tela
// gl_FragColor = vec4(uv, 0.0, 1.0);
// gl_FragColor = vec4( abs(sin(uTime)), 0.0, 0.0, 1.0 );
// gl_FragColor = vec4(uvDistorted, 0.0, 1.0);
}
