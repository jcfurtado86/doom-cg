// Modernized draw.cpp
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include "scene.h"
#include <cstdio>
#include <vector>

#include "transform.h"

#define NUM_TORRES 5
#define RAIO 15.0f // raio das torres ao redor do centro

extern GLuint texChao;
extern GLuint texTorre;
extern GLuint texDegrau;
extern GLuint texEsfera;
extern GLuint texLava;
extern GLuint progEsfera;
extern GLuint progLava;
extern GLuint progModern;

// VAOs/VBOs
static GLuint vaoGround = 0, vboGround = 0, eboGround = 0;
static GLuint vaoLosango = 0, vboLosango = 0, eboLosango = 0;
static GLuint vaoCube = 0, vboCube = 0, eboCube = 0;
static GLuint vaoCircle = 0, vboCircle = 0;
static GLuint vaoSphere = 0, vboSphere = 0, eboSphere = 0;
static int sphereIndexCount = 0;

static void ensureGround()
{
    if (vaoGround)
        return;
    float tiles = 75.0f;
    float verts[] = {
           -80.0f,
           0.0f,
           -80.0f,
        0.0f,
        0.0f,
        1,
        1,
        1,
        80.0f,
           0.0f,
        -80.0f,
        tiles,
        0.0f,
        1,
        1,
        1,
        80.0f,
           0.0f,
        80.0f,
        tiles,
        tiles,
        1,
        1,
        1,
           -80.0f,
           0.0f,
        80.0f,
        0.0f,
        tiles,
        1,
        1,
        1,
    };
    unsigned int inds[] = {0, 1, 2, 2, 3, 0};
    glGenVertexArrays(1, &vaoGround);
    glBindVertexArray(vaoGround);
    glGenBuffers(1, &vboGround);
    glBindBuffer(GL_ARRAY_BUFFER, vboGround);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glGenBuffers(1, &eboGround);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGround);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glBindVertexArray(0);
}

static void ensureLosango()
{
    if (vaoLosango)
        return;
    // build a diamond (octahedron) centered at origin: 6 vertices, 8 triangles
    float h = 0.5f, s = 0.333333f;
    float claroR = 0.3f, claroG = 1.0f, claroB = 0.3f;
    float escuroR = 0.0f, escuroG = 0.6f, escuroB = 0.0f;
    float verts[] = {
        // x,    y,    z,    u,    v,    r,       g,       b
        0.0f,  h,  0.0f,  0.5f, 1.0f,  claroR, claroG, claroB, // 0 top (light green)
        -s,   0.0f, 0.0f,  0.0f, 0.5f,  escuroR, escuroG, escuroB, // 1 left (dark)
        0.0f, 0.0f,  s,   0.5f, 0.5f,  claroR, claroG, claroB, // 2 front (light)
        s,    0.0f, 0.0f,  1.0f, 0.5f,  escuroR, escuroG, escuroB, // 3 right (dark)
        0.0f, 0.0f, -s,   0.5f, 0.0f,  claroR, claroG, claroB, // 4 back (light)
        0.0f, -h, 0.0f,   0.5f, 0.0f,  escuroR, escuroG, escuroB  // 5 bottom (dark)
    };
    unsigned int inds[] = {
        // top four triangles (CCW when viewed from outside)
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 1,
        // bottom four triangles
        5, 2, 1,
        5, 3, 2,
        5, 4, 3,
        5, 1, 4
    };
    glGenVertexArrays(1, &vaoLosango);
    glBindVertexArray(vaoLosango);
    glGenBuffers(1, &vboLosango);
    glBindBuffer(GL_ARRAY_BUFFER, vboLosango);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glGenBuffers(1, &eboLosango);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboLosango);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glBindVertexArray(0);
}

static void ensureCube()
{
    if (vaoCube)
        return;
    float half = 0.5f;
    // 24 vertices (4 per face) so each face can have independent UVs
    float vertices[] = {
        // Front face (+Z)
        -half, -half,  half,  0.0f, 0.0f, 1, 1, 1,
         half, -half,  half,  1.0f, 0.0f, 1, 1, 1,
         half,  half,  half,  1.0f, 1.0f, 1, 1, 1,
        -half,  half,  half,  0.0f, 1.0f, 1, 1, 1,
        // Back face (-Z)
         half, -half, -half,  0.0f, 0.0f, 1, 1, 1,
        -half, -half, -half,  1.0f, 0.0f, 1, 1, 1,
        -half,  half, -half,  1.0f, 1.0f, 1, 1, 1,
         half,  half, -half,  0.0f, 1.0f, 1, 1, 1,
        // Left face (-X)
        -half, -half, -half,  0.0f, 0.0f, 1, 1, 1,
        -half, -half,  half,  1.0f, 0.0f, 1, 1, 1,
        -half,  half,  half,  1.0f, 1.0f, 1, 1, 1,
        -half,  half, -half,  0.0f, 1.0f, 1, 1, 1,
        // Right face (+X)
         half, -half,  half,  0.0f, 0.0f, 1, 1, 1,
         half, -half, -half,  1.0f, 0.0f, 1, 1, 1,
         half,  half, -half,  1.0f, 1.0f, 1, 1, 1,
         half,  half,  half,  0.0f, 1.0f, 1, 1, 1,
        // Top face (+Y)
        -half,  half,  half,  0.0f, 0.0f, 1, 1, 1,
         half,  half,  half,  1.0f, 0.0f, 1, 1, 1,
         half,  half, -half,  1.0f, 1.0f, 1, 1, 1,
        -half,  half, -half,  0.0f, 1.0f, 1, 1, 1,
        // Bottom face (-Y)
        -half, -half, -half,  0.0f, 0.0f, 1, 1, 1,
         half, -half, -half,  1.0f, 0.0f, 1, 1, 1,
         half, -half,  half,  1.0f, 1.0f, 1, 1, 1,
        -half, -half,  half,  0.0f, 1.0f, 1, 1, 1,
    };
    unsigned int inds[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
    glGenVertexArrays(1, &vaoCube);
    glBindVertexArray(vaoCube);
    glGenBuffers(1, &vboCube);
    glBindBuffer(GL_ARRAY_BUFFER, vboCube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &eboCube);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboCube);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glBindVertexArray(0);
}

static void ensureCircle(int segmentos = 64, float raio = 12.0f, float tiles = 6.0f)
{
    if (vaoCircle)
        return;
    std::vector<float> verts;
    verts.reserve((segmentos + 2) * 8);
    verts.push_back(0.0f);
    verts.push_back(0.001f);
    verts.push_back(0.0f);
    verts.push_back(0.5f);
    verts.push_back(0.5f);
    verts.push_back(1.0f);
    verts.push_back(1.0f);
    verts.push_back(1.0f);
    for (int i = 0; i <= segmentos; ++i)
    {
        float ang = (float)i / segmentos * 2.0f * M_PI;
        float x = cosf(ang) * raio;
        float z = sinf(ang) * raio;
        float u = 0.5f + cosf(ang) * 0.5f * tiles;
        float v = 0.5f + sinf(ang) * 0.5f * tiles;
        verts.push_back(x);
        verts.push_back(0.001f);
        verts.push_back(z);
        verts.push_back(u);
        verts.push_back(v);
        verts.push_back(1.0f);
        verts.push_back(1.0f);
        verts.push_back(1.0f);
    }
    glGenVertexArrays(1, &vaoCircle);
    glBindVertexArray(vaoCircle);
    glGenBuffers(1, &vboCircle);
    glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glBindVertexArray(0);
}

static void ensureSphere(int slices = 40, int stacks = 40, float radius = 3.0f)
{
    if (vaoSphere)
        return;
    std::vector<float> verts;
    std::vector<unsigned int> inds;
    for (int y = 0; y <= stacks; ++y)
    {
        float v = (float)y / stacks;
        float phi = (v - 0.5f) * M_PI;
        for (int x = 0; x <= slices; ++x)
        {
            float u = (float)x / slices;
            float theta = u * 2.0f * M_PI;
            float px = cosf(phi) * cosf(theta) * radius;
            float py = sinf(phi) * radius;
            float pz = cosf(phi) * sinf(theta) * radius;
            verts.push_back(px);
            verts.push_back(py);
            verts.push_back(pz);
            verts.push_back(u);
            verts.push_back(v);
            verts.push_back(1.0f);
            verts.push_back(1.0f);
            verts.push_back(1.0f);
        }
    }
    for (int y = 0; y < stacks; ++y)
    {
        for (int x = 0; x < slices; ++x)
        {
            unsigned int i0 = y * (slices + 1) + x;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + (slices + 1);
            unsigned int i3 = i2 + 1;
            inds.push_back(i0);
            inds.push_back(i2);
            inds.push_back(i1);
            inds.push_back(i1);
            inds.push_back(i2);
            inds.push_back(i3);
        }
    }
    sphereIndexCount = inds.size();
    glGenVertexArrays(1, &vaoSphere);
    glBindVertexArray(vaoSphere);
    glGenBuffers(1, &vboSphere);
    glBindBuffer(GL_ARRAY_BUFFER, vboSphere);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &eboSphere);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSphere);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glBindVertexArray(0);
}

void desenhaChao()
{
    ensureGround();
    glBindTexture(GL_TEXTURE_2D, texChao);
    if (progModern)
        glUseProgram(progModern);
    GLint locUseTex = (progModern ? glGetUniformLocation(progModern, "uUseTexture") : -1);
    if (locUseTex >= 0)
        glUniform1i(locUseTex, 1);
    GLint locTexScaleGround = (progModern ? glGetUniformLocation(progModern, "uTexScale") : -1);
    if (locTexScaleGround >= 0)
        glUniform2f(locTexScaleGround, 1.0f, 1.0f);
    glBindVertexArray(vaoGround);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    if (progModern)
        glUseProgram(0);
}

void desenhaTorresELosangos()
{
    ensureCube();
    ensureLosango();
    float alturaTorre = 2.5f;
    float w = 0.7f;
    float ang0 = -M_PI / 2.0f;
    float passo = 2.0f * M_PI / NUM_TORRES;
    for (int i = 0; i < NUM_TORRES; i++)
    {
        float ang = ang0 + passo * i;
        float x = RAIO * cosf(ang);
        float z = RAIO * sinf(ang);
        gMatrixStack.push();
        gMatrixStack.translate(x, 0.0f, z);
        // torre
        gMatrixStack.push();
        glBindTexture(GL_TEXTURE_2D, texTorre);
        gMatrixStack.translate(0, alturaTorre / 2.0f, 0);
        gMatrixStack.scale(w, alturaTorre, w);
        if (progModern)
            glUseProgram(progModern);
        if (progModern) uploadMVP(progModern);
        GLint locUseTex = (progModern ? glGetUniformLocation(progModern, "uUseTexture") : -1);
        if (locUseTex >= 0)
            glUniform1i(locUseTex, 1);
        // set tex scale uniform so the shader tiles the texture instead of stretching
        GLint locTexScale = (progModern ? glGetUniformLocation(progModern, "uTexScale") : -1);
        glBindVertexArray(vaoCube);
        if (locTexScale >= 0)
        {
            // sides: keep vertical tiling according to tower height
            glUniform2f(locTexScale, 1.0f, alturaTorre);
            glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, (void *)(0));
            // top: tile using horizontal size so texture doesn't stretch on top
            glUniform2f(locTexScale, w, w);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(24 * sizeof(unsigned int)));
        }
        else
        {
            // fallback: draw whole cube
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        if (progModern)
            glUseProgram(0);
        gMatrixStack.pop();
        // losango
        gMatrixStack.push();
        gMatrixStack.translate(0.0f, alturaTorre + 1.2f, 0.0f);
        gMatrixStack.rotate(anguloPiramide, 0, 1, 0);
        if (progModern)
            glUseProgram(progModern);
        if (progModern) uploadMVP(progModern);
        GLint locUseTex2 = (progModern ? glGetUniformLocation(progModern, "uUseTexture") : -1);
        if (locUseTex2 >= 0)
            glUniform1i(locUseTex2, 0);
        // make sure losango draws double-sided in case face culling is enabled
        GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
        if (wasCull)
            glDisable(GL_CULL_FACE);
        glBindVertexArray(vaoLosango);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (wasCull)
            glEnable(GL_CULL_FACE);
        if (progModern)
            glUseProgram(0);
        gMatrixStack.pop();
        gMatrixStack.pop();
    }
}

void desenhaPiramideDegraus()
{
    float alturaDegrau = 0.5f, tamanhoBase = 6.0f, reducao = 0.65f, raioLava = 12.0f;
    int segmentos = 64;
    float tilesLava = 6.0f;
    gMatrixStack.push();
    ensureCircle(segmentos, raioLava, tilesLava);
    glUseProgram(progLava);
    GLint locTimeLava = glGetUniformLocation(progLava, "uTime");
    GLint locStrLava = glGetUniformLocation(progLava, "uStrength");
    GLint locScrollLava = glGetUniformLocation(progLava, "uScroll");
    GLint locHeatLava = glGetUniformLocation(progLava, "uHeat");
    GLint locTexLava = glGetUniformLocation(progLava, "uTexture");
    glUniform1f(locTimeLava, tempoEsfera);
    glUniform1f(locStrLava, 1.0f);
    glUniform2f(locScrollLava, 0.1f, 0.0f);
    glUniform1f(locHeatLava, 0.5f);
    glBindTexture(GL_TEXTURE_2D, texLava);
    glUniform1i(locTexLava, 0);
    // upload MVP for lava shader
    if (progLava) uploadMVP(progLava);
    // use VAO attribute setup (modern pipeline)
    glBindVertexArray(vaoCircle);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segmentos + 2);
    glBindVertexArray(0);
    glUseProgram(0);
    ensureCube();
    glBindTexture(GL_TEXTURE_2D, texDegrau);
    // degraus usando cube VAO scaled
    gMatrixStack.push();
    gMatrixStack.translate(0.0f, alturaDegrau / 2.0f, 0.0f);
    gMatrixStack.scale(tamanhoBase, alturaDegrau, tamanhoBase);
    if (progModern)
        glUseProgram(progModern);
    if (progModern) uploadMVP(progModern);
    GLint locUseTex = (progModern ? glGetUniformLocation(progModern, "uUseTexture") : -1);
    if (locUseTex >= 0)
        glUniform1i(locUseTex, 1);
    GLint locTexScaleStep = (progModern ? glGetUniformLocation(progModern, "uTexScale") : -1);
        if (locTexScaleStep >= 0)
            glUniform2f(locTexScaleStep, tamanhoBase, alturaDegrau);
    glBindVertexArray(vaoCube);
    // draw sides (faces 0..3)
    if (locTexScaleStep >= 0)
        glUniform2f(locTexScaleStep, tamanhoBase, alturaDegrau);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, (void *)(0));
    // draw top face (face 4) with different scale (width x width)
        if (locTexScaleStep >= 0)
            glUniform2f(locTexScaleStep, tamanhoBase, tamanhoBase);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(24 * sizeof(unsigned int)));
    glBindVertexArray(0);
    if (progModern)
        glUseProgram(0);
    gMatrixStack.pop();
    gMatrixStack.push();
    gMatrixStack.translate(0.0f, alturaDegrau + alturaDegrau / 2.0f, 0.0f);
    gMatrixStack.scale(tamanhoBase * reducao, alturaDegrau, tamanhoBase * reducao);
    if (progModern)
        glUseProgram(progModern);
    if (locUseTex >= 0)
        glUniform1i(locUseTex, 1);
    if (locTexScaleStep >= 0)
        glUniform2f(locTexScaleStep, tamanhoBase * reducao, alturaDegrau);
    if (progModern) uploadMVP(progModern);
    glBindVertexArray(vaoCube);
    // sides
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, (void *)(0));
    // top: ensure we use base width scale so texel density matches large step
    if (locTexScaleStep >= 0)
        glUniform2f(locTexScaleStep, tamanhoBase, tamanhoBase);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(24 * sizeof(unsigned int)));
    glBindVertexArray(0);
    if (progModern)
        glUseProgram(0);
    gMatrixStack.pop();
    gMatrixStack.push();
    gMatrixStack.translate(0.0f, 2 * alturaDegrau + alturaDegrau / 2.0f, 0.0f);
    gMatrixStack.scale(tamanhoBase * reducao * reducao, alturaDegrau, tamanhoBase * reducao * reducao);
    if (progModern)
        glUseProgram(progModern);
    if (locUseTex >= 0)
        glUniform1i(locUseTex, 1);
    if (locTexScaleStep >= 0)
        glUniform2f(locTexScaleStep, tamanhoBase * reducao * reducao, alturaDegrau);
    if (progModern) uploadMVP(progModern);
    glBindVertexArray(vaoCube);
    // sides
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, (void *)(0));
    // top: keep base width scale so small top keeps same texel density
    if (locTexScaleStep >= 0)
        glUniform2f(locTexScaleStep, tamanhoBase, tamanhoBase);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(24 * sizeof(unsigned int)));
    glBindVertexArray(0);
    if (progModern)
        glUseProgram(0);
    gMatrixStack.pop();
    // esfera
    ensureSphere(40, 40, 3.0f);
    glUseProgram(progEsfera);
    GLint locTimeBlood = glGetUniformLocation(progEsfera, "uTime");
    GLint locStrBlood = glGetUniformLocation(progEsfera, "uStrength");
    GLint locSpeedBlood = glGetUniformLocation(progEsfera, "uSpeed");
    GLint locTexBlood = glGetUniformLocation(progEsfera, "uTexture");
    glUniform1f(locTimeBlood, tempoEsfera);
    glUniform1f(locStrBlood, 1.0f);
    glUniform2f(locSpeedBlood, 3.0f, 1.7f);
    gMatrixStack.push();
    gMatrixStack.translate(0.0f, 5.0f * alturaDegrau + 3.0f + 0.2f, 0.0f);
    gMatrixStack.rotate(anguloEsfera, 1.0f, 1.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, texEsfera);
    glUniform1i(locTexBlood, 0);
    // draw sphere using VAO attribute setup (modern pipeline)
    if (progEsfera) uploadMVP(progEsfera);
    glBindVertexArray(vaoSphere);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    gMatrixStack.pop();
    glUseProgram(0);
    gMatrixStack.pop();
}
