
#ifndef VEC_MATH_H
#define VEC_MATH_H

// http://www.reedbeta.com/blog/2013/12/28/on-vector-math-libraries/

#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <assert.h>

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

#define DEG_TO_RAD(X) ((X * M_PI) / 180.0f)
#define RAD_TO_DEG(X) ((X * 180.0f) / M_PI)

#define CLAMP(VAL, MIN, MAX) (VAL < MIN ? MIN: (VAL > MAX ? MAX : VAL))


static const float V_EPSILON = 0.00001f;

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float x, y, z, w; } Quat;
typedef struct { float m[16]; } Mat4;

static inline float Deg_Diff(float a, float b)
{
    assert((int)fabs(a) < 360);
    assert((int)fabs(b) < 360);

    float diff = a - b;
    
    if (diff > 180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    
    assert((int)fabs(diff) <= 180);

    return diff;
}

static inline float Deg_Normalize(float a)
{
    return fmodf(a, 360.0f);
}

static inline float Interp_Lerp(float t, float min, float max)
{
    return t * max + (1.0f - t) * min;
}

static const Vec2 Vec2_Zero = {0.0f, 0.0f};

static inline Vec2 Vec2_Create(float x, float y)
{
    Vec2 v;
    v.x = x; v.y = y;
    return v;
}

static inline Vec2 Vec2_FromVec3(Vec3 v)
{
    return Vec2_Create(v.x, v.y);
}

static inline Vec2 Vec2_Clear(float a)
{
    Vec2 r;
    r.x = a; r.y = a;
    return r;
}

static inline Vec2 Vec2_Add(Vec2 v0, Vec2 v1)
{
    v0.x += v1.x;
    v0.y += v1.y;
    return v0;
}

static inline Vec2 Vec2_Sub(Vec2 v0, Vec2 v1)
{
    v0.x -= v1.x;
    v0.y -= v1.y;
    return v0;
}

static inline Vec2 Vec2_Mult(Vec2 v0, Vec2 v1)
{
    v0.x *= v1.x;
    v0.y *= v1.y;
    return v0;
}

static inline float Vec2_Dot(Vec2 v0, Vec2 v1)
{
    return v0.x * v1.x + v0.y * v1.y;
}

static inline Vec2 Vec2_Negate(Vec2 v)
{
    v.x = -v.x;
    v.y = -v.y;
    return v;
}

static inline Vec2 Vec2_Offset(Vec2 v, float x, float y)
{
    v.x += x; v.y += y;
    return v;
}

static inline Vec2 Vec2_Scale(Vec2 v, float s)
{
    v.x *= s; v.y *= s;
    return v;
}

static inline float Vec2_Length(Vec2 v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

static inline float Vec2_LengthSq(Vec2 v)
{
    return (v.x * v.x + v.y * v.y);
}

static inline float Vec2_Dist(Vec2 v0, Vec2 v1)
{
    return Vec2_Length(Vec2_Sub(v0, v1));
}

static inline Vec2 Vec2_Norm(Vec2 v)
{
    float s = 1.0f / Vec2_Length(v);
    v.x = v.x * s;
    v.y = v.y * s;
    return v;
}

static inline Vec2 Vec2_Lerp(Vec2 a, Vec2 b, float t)
{
    Vec2 v;
    v.x = Interp_Lerp(t, a.x, b.x);
    v.y = Interp_Lerp(t, a.y, b.y);
    return v;
}

#define Vec3_Get(v, i) ((&(v).x)[(i)])

static const Vec3 Vec3_Zero = {0.0f, 0.0f, 0.0f};

static inline Vec3 Vec3_Create(float x, float y, float z)
{
    Vec3 r;
    r.x = x; r.y = y; r.z = z;
    return r;
}

static inline Vec3 Vec3_FromVec2(Vec2 v)
{
    return Vec3_Create(v.x, v.y, 0.0f);
}

static inline Vec3 Vec3_FromVec4(Vec4 vec)
{
    return Vec3_Create(vec.x, vec.y, vec.z);
}

static inline Vec3 Vec3_Clear(float a)
{
    Vec3 r;
    r.x = a; r.y = a; r.z = a;
    return r;
}

static inline Vec3 Vec3_Add(Vec3 v0, Vec3 v1)
{
    v0.x += v1.x;
    v0.y += v1.y;
    v0.z += v1.z;
    return v0;
}

static inline Vec3 Vec3_Sub(Vec3 v0, Vec3 v1)
{
    v0.x -= v1.x;
    v0.y -= v1.y;
    v0.z -= v1.z;
    return v0;
}

static inline Vec3 Vec3_Mult(Vec3 v0, Vec3 v1)
{
    v0.x *= v1.x;
    v0.y *= v1.y;
    v0.z *= v1.z;
    return v0;
}

static inline float Vec3_Dot(Vec3 v0, Vec3 v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

static inline Vec3 Vec3_Cross(Vec3 v0, Vec3 v1)
{
    Vec3 r;
    r.x = v0.y * v1.z - v0.z * v1.y;
    r.y = v0.z * v1.x - v0.x * v1.z;
    r.z = v0.x * v1.y - v0.y * v1.x;
    return r;
}

static inline Vec3 Vec3_Negate(Vec3 v)
{
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
    return v;
}

static inline Vec3 Vec3_Offset(Vec3 v, float x, float y, float z)
{
    v.x += x;
    v.y += y;
    v.z += z;
    return v;
}

static inline Vec3 Vec3_Scale(Vec3 v, float s)
{
    v.x = v.x * s;
    v.y = v.y * s;
    v.z = v.z * s;
    return v;
}

static inline float Vec3_Length(Vec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float Vec3_LengthSq(Vec3 v)
{
    return (v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float Vec3_Dist(Vec3 v0, Vec3 v1)
{
    return Vec3_Length(Vec3_Sub(v0, v1));
}

static inline float Vec3_DistSq(Vec3 v0, Vec3 v1)
{
    return Vec3_LengthSq(Vec3_Sub(v0, v1));
}

static inline float Vec3_ManhatDist(Vec3 a, Vec3 b)
{
    return fabsf(a.x - b.x) + fabsf(a.y - b.y) + fabsf(a.z - b.z);
}

static inline Vec3 Vec3_Norm(Vec3 v)
{
    float s = 1.0f / Vec3_Length(v);
    v.x = v.x * s;
    v.y = v.y * s;
    v.z = v.z * s;
    return v;
}

static inline Vec3 Vec3_Lerp(Vec3 v0, Vec3 v1, float t)
{
    Vec3 r;
    r.x = Interp_Lerp(t, v0.x, v1.x);
    r.y = Interp_Lerp(t, v0.y, v1.y);
    r.z = Interp_Lerp(t, v0.z, v1.z);
    return r;
}

static const Vec4 Vec4_Zero = {0.0f, 0.0f, 0.0f, 0.0f};

static inline Vec4 Vec4_Create(float x, float y, float z, float w)
{
    Vec4 r;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

static inline Vec4 Vec4_Add(Vec4 v0, Vec4 v1)
{
    v0.x += v1.x;
    v0.y += v1.y;
    v0.z += v1.z;
    v0.w += v1.w;
    return v0;
}

static inline Vec4 Vec4_Sub(Vec4 v0, Vec4 v1)
{
    v0.x -= v1.x;
    v0.y -= v1.y;
    v0.z -= v1.z;
    v0.w -= v1.w;
    return v0;
}

#define Mat4_Get(mat, x, y) ((mat)->m[((x) * 4) + (y)])
#define Mat4_Set(mat, x, y, val) ((mat)->m[((x) * 4) + (y)] = (val))

extern Mat4 Mat4_CreateIdentity();
extern Mat4 Mat4_CreateTranslate(Vec3 v);
extern Mat4 Mat4_CreateLook(Vec3 eye, Vec3 target, Vec3 up);
extern Mat4 Mat4_CreateFrustum(float left,
                               float right,
                               float bottom,
                               float top,
                               float near,
                               float far);

extern Mat4 Mat4_CreateOrtho(float left,
                             float right,
                             float bottom,
                             float top,
                             float near,
                             float far);

extern void Mat4_Identity(Mat4* m);

static inline void Mat4_Copy(Mat4* dest, const Mat4* source)
{
    assert(dest && source);
    memcpy(dest, source, sizeof(Mat4));
}

extern Vec3 Mat4_MultVec3(const Mat4* m, Vec3 v);
extern Vec4 Mat4_MultVec4(const Mat4* m, Vec4 v);

extern void Mat4_Mult(const Mat4* a, const Mat4* b, Mat4* dest);
extern void Mat4_Transpose(Mat4* a);
extern float Mat4_Det(const Mat4* m);
extern void Mat4_Inverse(const Mat4* matrix, Mat4* dest);
extern void Mat4_Translate(Mat4* m, Vec3 v);
extern void Mat4_Scale(Mat4* m, Vec3 s);

extern Vec3 Mat4_Unproject(const Mat4* inverseMvp, Vec3 normalizePoint);
extern Vec3 Mat4_Project(const Mat4* mvp, Vec3 point);

static const Quat Quat_Identity = {0.0f, 0.0f, 0.0f, 1.0f};

static inline Quat Quat_Create(float x, float y, float z, float w)
{
    Quat q;
    q.x = x; q.y = y; q.z = z; q.w = w;
    return q;
}

extern Quat Quat_CreateLook(Vec3 dir, Vec3 up);
extern Quat Quat_CreateEuler(float roll, float pitch, float yaw);
extern Quat Quat_CreateAngle(float angle, float x, float y, float z);

static inline Quat Quat_Add(Quat q0, Quat q1)
{
    q0.x += q1.x;
    q0.y += q1.y;
    q0.z += q1.z;
    q0.w += q1.w;
    return q0;
}

static inline Quat Quat_Sub(Quat q0, Quat q1)
{
    q0.x -= q1.x;
    q0.y -= q1.y;
    q0.z -= q1.z;
    q0.w -= q1.w;
    return q0;
}

static inline Quat Quat_Mult(Quat q0, Quat q1)
{
    Quat v;
    v.x = q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y;
    v.y = q0.w * q1.y + q0.y * q1.w + q0.z * q1.x - q0.x * q1.z;
    v.z = q0.w * q1.z + q0.z * q1.w + q0.x * q1.y - q0.y * q1.x;
    v.w = q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z;
    return v;
}

static inline float Quat_Dot(Quat q0, Quat q1)
{
    return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
}

static inline Quat Quat_Negate(Quat q)
{
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;
    return q;
}

static inline Quat Quat_Scale(Quat q, float s)
{
    q.x *= s;
    q.y *= s;
    q.z *= s;
    q.w *= s;
    return q;
}

static inline Vec3 Quat_MultVec3(const Quat* q, Vec3 v)
{
    Vec3 u = Vec3_Create(q->x, q->y, q->z);
    Vec3 t = Vec3_Scale(Vec3_Cross(u, v), 2.0f);
    
    Vec3 r = Vec3_Add(v, Vec3_Scale(t, q->w));
    r = Vec3_Add(r, Vec3_Cross(u, t));
    return r;
    
    /*
     Alternate implementation
     
     Quat vecQuat, resQuat;
     
     vecQuat.x = v.x;
     vecQuat.y = v.y;
     vecQuat.z = v.z;
     vecQuat.w = 0.0f;
     
     resQuat = Quat_Mult(vecQuat, Quat_Negate(*a));
     resQuat = Quat_Mult(*a, resQuat);
     
     return Vec3_Create(resQuat.x, resQuat.y, resQuat.z);
     */
}

extern void Quat_ToMatrix( Quat a, Mat4* dest);

extern Quat Quat_Norm(Quat q);
extern Quat Quat_Slerp(Quat q0, Quat q1, float t);



#endif
