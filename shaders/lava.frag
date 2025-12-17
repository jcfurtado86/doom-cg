#version 330 core

uniform sampler2D uTexture;
uniform float uTime;        // tempo em segundos
uniform float uStrength;    // força da distorção 
uniform vec2  uScroll;      // direção de rolagem da lava
uniform float uHeat;        // intensidade do brilho

in vec2 vTexCoord;
out vec4 fragColor;

void main()
{
    // scroll da textura
    vec2 uv = vTexCoord + uScroll * uTime;

    // ondas de calor 
    float w1 = sin(uv.y * 20.0 + uTime * 3.0);
    float w2 = cos(uv.x * 15.0 + uTime * 2.0);
    float distort = (w1 + w2) * 0.5;

    vec2 uvDistorted = uv + uStrength * vec2(distort * 0.02, distort * 0.03);

    // cor base da textura
    vec4 baseColor = texture(uTexture, uvDistorted);

    // mistura com cor de lava
    vec3 lavaTint = vec3(1.0, 0.4, 0.1);         // laranja quente
    vec3 color = mix(baseColor.rgb, lavaTint, 0.6);

    //pontos iluminados
    float glowFactor = pow(abs(w1 * w2), 2.0);  
    vec3 glowColor = vec3(1.2, 0.6, 0.2);       

    color += glowFactor * glowColor * uHeat;

    color = clamp(color, 0.0, 1.0);

    fragColor = vec4(color, baseColor.a);
}
