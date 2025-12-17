#version 330 core

in vec2 vTex;
in vec3 vColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 fragColor;

void main()
{
    if (uUseTexture)
    {
        vec4 t = texture(uTexture, vTex);
        fragColor = vec4(t.rgb * vColor, t.a);
    }
    else
    {
        fragColor = vec4(vColor, 1.0);
    }
}
