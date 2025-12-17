#pragma once

#include <vector>
#include <math.h>
#include <GL/glew.h>

struct Mat4
{
    float m[16];
    static Mat4 identity()
    {
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
    static Mat4 multiply(const Mat4 &a, const Mat4 &b)
    {
        Mat4 r;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k)
                    s += a.m[k * 4 + j] * b.m[i * 4 + k];
                r.m[i * 4 + j] = s;
            }
        }
        return r;
    }
    static Mat4 translate(float x, float y, float z)
    {
        Mat4 r = Mat4::identity();
        r.m[12] = x;
        r.m[13] = y;
        r.m[14] = z;
        return r;
    }
    static Mat4 scale(float x, float y, float z)
    {
        Mat4 r = Mat4::identity();
        r.m[0] = x;
        r.m[5] = y;
        r.m[10] = z;
        return r;
    }
    static Mat4 rotate(float angleDegrees, float x, float y, float z)
    {
        float a = angleDegrees * (float)M_PI / 180.0f;
        float c = cosf(a);
        float s = sinf(a);
        float len = sqrtf(x * x + y * y + z * z);
        if (len == 0.0f) return Mat4::identity();
        x /= len; y /= len; z /= len;
        Mat4 r;
        r.m[0] = x * x * (1 - c) + c;
        r.m[1] = y * x * (1 - c) + z * s;
        r.m[2] = x * z * (1 - c) - y * s;
        r.m[3] = 0.0f;

        r.m[4] = x * y * (1 - c) - z * s;
        r.m[5] = y * y * (1 - c) + c;
        r.m[6] = y * z * (1 - c) + x * s;
        r.m[7] = 0.0f;

        r.m[8] = x * z * (1 - c) + y * s;
        r.m[9] = y * z * (1 - c) - x * s;
        r.m[10] = z * z * (1 - c) + c;
        r.m[11] = 0.0f;

        r.m[12] = 0.0f;
        r.m[13] = 0.0f;
        r.m[14] = 0.0f;
        r.m[15] = 1.0f;
        return r;
    }
    static Mat4 perspective(float fovyDeg, float aspect, float znear, float zfar)
    {
        float fovy = fovyDeg * (float)M_PI / 180.0f;
        float f = 1.0f / tanf(fovy / 2.0f);
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (zfar + znear) / (znear - zfar);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
        return r;
    }
    static Mat4 lookAt(float eyeX, float eyeY, float eyeZ,
                       float centerX, float centerY, float centerZ,
                       float upX, float upY, float upZ)
    {
        float fx = centerX - eyeX;
        float fy = centerY - eyeY;
        float fz = centerZ - eyeZ;
        float flen = sqrtf(fx * fx + fy * fy + fz * fz);
        fx /= flen; fy /= flen; fz /= flen;
        // up normalized
        float ux = upX, uy = upY, uz = upZ;
        float ulen = sqrtf(ux * ux + uy * uy + uz * uz);
        ux /= ulen; uy /= ulen; uz /= ulen;
        // s = f x up
        float sx = fy * uz - fz * uy;
        float sy = fz * ux - fx * uz;
        float sz = fx * uy - fy * ux;
        float slen = sqrtf(sx * sx + sy * sy + sz * sz);
        sx /= slen; sy /= slen; sz /= slen;
        // u' = s x f
        float ux2 = sy * fz - sz * fy;
        float uy2 = sz * fx - sx * fz;
        float uz2 = sx * fy - sy * fx;
        Mat4 r = Mat4::identity();
        r.m[0] = sx; r.m[4] = sy; r.m[8] = sz; r.m[12] = 0.0f;
        r.m[1] = ux2; r.m[5] = uy2; r.m[9] = uz2; r.m[13] = 0.0f;
        r.m[2] = -fx; r.m[6] = -fy; r.m[10] = -fz; r.m[14] = 0.0f;
        r.m[3] = 0.0f; r.m[7] = 0.0f; r.m[11] = 0.0f; r.m[15] = 1.0f;
        Mat4 t = Mat4::translate(-eyeX, -eyeY, -eyeZ);
        return multiply(r, t);
    }
};

struct MatStack
{
    std::vector<Mat4> s;
    MatStack() { s.push_back(Mat4::identity()); }
    void push() { s.push_back(s.back()); }
    void pop() { if (s.size() > 1) s.pop_back(); }
    void loadIdentity() { s.back() = Mat4::identity(); }
    void translate(float x, float y, float z) { s.back() = Mat4::multiply( s.back(), Mat4::translate(x,y,z) ); }
    void scale(float x, float y, float z) { s.back() = Mat4::multiply( s.back(), Mat4::scale(x,y,z) ); }
    void rotate(float angle, float x, float y, float z) { s.back() = Mat4::multiply( s.back(), Mat4::rotate(angle,x,y,z) ); }
    const Mat4 &top() const { return s.back(); }
};

extern MatStack gMatrixStack;
extern Mat4 gProjection;

// upload MVP = projection * viewModel (viewModel is the current top of stack)
inline void uploadMVP(GLuint program)
{
    GLint loc = glGetUniformLocation(program, "uMVP");
    if (loc >= 0)
    {
        Mat4 m = Mat4::multiply(gProjection, gMatrixStack.top());
        glUniformMatrix4fv(loc, 1, GL_FALSE, m.m);
    }
}
