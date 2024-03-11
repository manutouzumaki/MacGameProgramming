#include <math.h>

struct Vec2 {
    float32 x, y;

    Vec2() : x(0), y(0) { }
    Vec2(float32 x_, float32 y_) : x(x_), y(y_) { }
};

Vec2 operator+(const Vec2& a, const Vec2& b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

void operator+=(Vec2& a, const Vec2& b) {
    a.x += b.x;
    a.y += b.y;
}

Vec2 operator-(const Vec2& a, const Vec2& b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

void operator-=(Vec2& a, const Vec2& b) {
    a.x -= b.x;
    a.y -= b.y;
}

Vec2 operator*(const Vec2& a, const Vec2& b) {
    Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

void operator*=(Vec2& a, const Vec2& b) {
    a.x *= b.x;
    a.y *= b.y;
}

Vec2 operator/(const Vec2& a, const Vec2& b) {
    Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

void operator/=(Vec2& a, const Vec2& b) {
    a.x /= b.x;
    a.y /= b.y;
}

Vec2 operator+(const Vec2& v, float32 s) {
    Vec2 result;
    result.x = v.x + s;
    result.y = v.y + s;
    return result;
}

void operator+=(Vec2& v, float32 s) {
    v.x += s;
    v.y += s;
}

Vec2 operator-(const Vec2& v, float32 s) {
    Vec2 result;
    result.x = v.x - s;
    result.y = v.y - s;
    return result;
}

void operator-=(Vec2& v, float32 s) {
    v.x -= s;
    v.y -= s;
}

Vec2 operator*(const Vec2& v, float32 s) {
    Vec2 result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

void operator*=(Vec2& v, float32 s) {
    v.x *= s;
    v.y *= s;
}

Vec2 operator/(const Vec2& v, float32 s) {
    Vec2 result;
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

void operator/=(Vec2& v, float32 s) {
    v.x /= s;
    v.y /= s;
}

void operator-(Vec2& v) {
    v.x = -v.x;
    v.y = -v.y;
}


float32 Dot(const Vec2& a, const Vec2& b) {
    float32 result = a.x * b.x + a.y * b.y;
    return result;
} 

float32 LenSq(const Vec2& v) {
    return Dot(v, v);
}

float32 Len(const Vec2& v) {
    return sqrtf(LenSq(v));
}

Vec2 Normalized(const Vec2& v) {
    float32 lenSq = LenSq(v);
    if(lenSq <= 0.0f) {
        return Vec2();
    }
    
    float32 invLen = 1.0f / sqrtf(lenSq);
    Vec2 result;
    result.x = v.x * invLen;
    result.y = v.y * invLen;
    return result;
}

void Normalize(Vec2 *v) {
    float32 lenSq = LenSq(*v);
    if(lenSq <= 0.0f) {
        return;
    }

    float32 invLen = 1.0f / sqrtf(lenSq);
    v->x *= invLen;
    v->y *= invLen;

}
