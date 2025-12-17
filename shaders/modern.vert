#version 330 core

in vec3 aPos;
in vec2 aTex;
in vec3 aColor;

out vec2 vTex;
out vec3 vColor;

uniform vec2 uTexScale;
uniform mat4 uMVP;

void main()
{
    vTex = aTex * uTexScale;
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
