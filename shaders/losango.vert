#version 120

varying vec2 vTexCoord;

void main()
{
    // posição padrão usando a matriz fixa
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // pega as coordenadas de textura do pipeline fixo
    vTexCoord = gl_MultiTexCoord0.st;
}
