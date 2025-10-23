#pragma once
#include <math.h>
#include <xhl/debug.h>
#include <xhl/maths.h>

typedef union Coeffs
{
    struct
    {
        float a1, a2, a3, m0, m1, m2;
    };
    void* _align;
} Coeffs;

static inline float calcG(float fc, float fs_inv /*sampleRateInv*/)
{
    return xm_fasttan_normalised(XM_HALF_PIf * 1.27f * fc * fs_inv);
}

static Coeffs filter_LP(float fc, float Q, float fs_inv)
{
    float g = calcG(fc, fs_inv);
    float k = 1.0f / Q;
    // float k = XM_SQRT2f; // Butterworth

    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;

    float m0 = 0.0f;
    float m1 = 0.0f;
    float m2 = 1.0f;
    return (Coeffs){a1, a2, a3, m0, m1, m2};
}

static inline float filter_process(float v0 /*xn*/, const Coeffs* c, float* s)
{
    float v3 = v0 - s[1];
    float v1 = c->a1 * s[0] + c->a2 * v3;
    float v2 = s[1] + c->a2 * s[0] + c->a3 * v3;
    s[0]     = 2 * v1 - s[0];
    s[1]     = 2 * v2 - s[1];
    return c->m0 * v0 + c->m1 * v1 + c->m2 * v2;
}