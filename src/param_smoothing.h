#pragma once
#include <math.h>
#include <xhl/debug.h>

typedef struct MySmoothedValue
{
    float current;
    float target;
    float inc;
    int   remaining;
} MySmoothedValue;

static inline void smoothvalue_reset(MySmoothedValue* sv, float newValue)
{
    sv->target = sv->current = newValue;
    sv->inc                  = 0;
    sv->remaining            = 0;
}

// Produces very few instructions
static inline float smoothvalue_tick(MySmoothedValue* sv)
{
    float v = sv->current;
    if (sv->remaining-- > 0)
        sv->current += sv->inc;
    return v;
}

static void smoothvalue_set_target(MySmoothedValue* sv, float newValue, int steps)
{
    xassert(steps);
    if (sv->target == newValue && sv->remaining)
        return;

    float inc  = 0;
    float diff = newValue - sv->current;

    while (inc == 0 && newValue != sv->target && steps)
    {
        inc = diff / (float)steps;
        xassert(inc == inc);
        if (fabsf(inc) < 1.0e-5f)
        {
            inc     = 0;
            steps >>= 1;
        }
    }
    if (inc == 0)
    {
        sv->current = newValue;
        steps       = 0;
    }

    sv->target    = newValue;
    sv->inc       = inc;
    sv->remaining = steps;
}