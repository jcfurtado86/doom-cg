#version 330 core

in vec3 aPos;
in vec2 aTex;
out vec2 vTexCoord;
uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    vTexCoord = aTex;
}
