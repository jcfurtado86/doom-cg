#include "graphics/shader.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

static GLuint compilaShader(GLenum tipo, const char *src)
{
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        printf("Erro ao compilar shader: %s\n", log);
        std::exit(1);
    }
    return shader;
}

std::string leArquivo(const char *caminho)
{
    std::ifstream in(caminho, std::ios::in | std::ios::binary);
    if (!in)
    {
        printf("Erro ao abrir %s\n", caminho);
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

GLuint criaShader(const char *vertPath, const char *fragPath)
{
    GLuint program = 0;

    // lê código-fonte dos arquivos
    std::string vsCode = leArquivo(vertPath);
    std::string fsCode = leArquivo(fragPath);

    if (vsCode.empty() || fsCode.empty())
    {
        printf("Erro: shader vazio. Verifique caminhos:\n  VS: %s\n  FS: %s\n",
               vertPath, fragPath);
        std::exit(1);
    }

    const char *vsSrc = vsCode.c_str();
    const char *fsSrc = fsCode.c_str();

    // compila usando sua função auxiliar
    GLuint vs = compilaShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compilaShader(GL_FRAGMENT_SHADER, fsSrc);

    // cria e linka o programa
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        printf("Erro ao linkar programa water:\n%s\n", log);
        std::exit(1);
    }

    // já pode deletar os shaders individuais
    glDeleteShader(vs);
    glDeleteShader(fs);

     return program;
}