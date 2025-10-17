#pragma once
#include <stdint.h>

constexpr float PI = 3.14159265358979323846f;

// Raiz quadrada usando método de Newton
inline float sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float result = x;
    float last = 0.0f;
    while (result != last) {
        last = result;
        result = 0.5f * (result + x / result);
    }
    return result;
}

// Aproximação simples de seno (Taylor para -PI..PI)
inline float sinf(float x) {
    // Normaliza para -PI..PI
    while (x > PI) x -= 2.0f * PI;
    while (x < -PI) x += 2.0f * PI;

    // Taylor expansion: sin(x) ≈ x - x^3/6 + x^5/120 - x^7/5040
    float x2 = x * x;
    float x3 = x * x2;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    return x - x3 / 6.0f + x5 / 120.0f - x7 / 5040.0f;
}

// Coseno usando identidade cos(x) = sin(x + PI/2)
inline float cosf(float x) {
    return sinf(x + PI / 2.0f);
}

// Converte graus para radianos
inline float deg2rad(float deg) {
    return deg * (PI / 180.0f);
}

