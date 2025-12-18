#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"

#include <GL/glut.h>
#include <cstdio>

GLuint carregaTextura(const char *arquivo)
{
    int largura, altura, canais;

    unsigned char *dados = stbi_load(arquivo, &largura, &altura, &canais, 3);
    if (!dados)
    {
        printf("ERRO ao carregar textura: %s\n", arquivo);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // envia para GPU
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        largura,
        altura,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        dados);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, largura, altura, GL_RGB, GL_UNSIGNED_BYTE, dados);

    stbi_image_free(dados);

    return texID;
}