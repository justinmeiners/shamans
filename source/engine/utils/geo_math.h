
#ifndef GEO_MATH_H
#define GEO_MATH_H

#include "vec_math.h"

typedef struct
{
    Vec3 origin;
    Vec3 dir;
} Ray3;

typedef struct
{
    Vec3 point;
    Vec3 normal;
} Plane;

typedef struct
{
    Vec3 min;
    Vec3 max;
} AABB;

typedef struct
{
    Vec3 origin;
    float radius;
} Sphere;

typedef enum
{
    kFrustumPlaneTop = 0,
    kFrustumPlaneBottom = 1,
    kFrustumPlaneLeft = 2,
    kFrustumPlaneRight = 3,
    kFrustumPlaneNear = 4,
    kFrustumPlaneFar = 5,
    kFrustumPlaneCount = 6,
} FrutumPlane;

/* 3D perspective frustum with frustum culling */
typedef struct
{
    Vec3 position;
    Vec3 target;
    Vec3 orientation;
    
    Vec3 right;
    Vec3 up;
    Vec3 view;
    
    float fov;
    float near;
    float far;
    
    float nearHeight;
    
    Mat4 projMatrix;
    Mat4 viewMatrix;
    
    Plane planes[kFrustumPlaneCount];
} Frustum;

static inline Ray3 Ray3_Create(Vec3 origin, Vec3 dir)
{
    Ray3 ray;
    ray.origin = origin;
    ray.dir = dir;
    return ray;
}

static inline Vec3 Ray3_Slide(Ray3 ray, float t)
{
    return Vec3_Add(ray.origin, Vec3_Scale(ray.dir, t));
}

static inline Plane Plane_Create(Vec3 point, Vec3 normal)
{
    Plane p;
    p.point = point;
    p.normal = normal;
    return p;
}

static inline float Plane_SignedDist(Plane p, Vec3 a)
{
    return Vec3_Dot(p.normal, Vec3_Sub(a, p.point));
}

static inline int Plane_IntersectRay(Plane plane, Ray3 ray, float* t)
{
    float d = Vec3_Dot(plane.point, plane.normal);
    
    float num = Vec3_Dot(plane.normal, ray.origin) - d;
    float denom = Vec3_Dot(plane.normal, ray.dir);
    
    if (fabsf(denom) < 0.00001f)
        return 0;
    
    *t = -(num / denom);
    
    return 1;
}

static inline AABB AABB_Zero()
{
    AABB a;
    a.min = Vec3_Zero;
    a.max = Vec3_Zero;
    return a;
}

static inline AABB AABB_Create(Vec3 min, Vec3 max)
{
    AABB a;
    a.min = min;
    a.max = max;
    return a;
}

static inline AABB AABB_CreateAnchored(Vec3 center, Vec3 size, Vec3 anchor)
{
    Vec3 anchorOffset = Vec3_Mult(anchor, size);
    AABB a;
    a.min = Vec3_Sub(center, anchorOffset);
    a.max = Vec3_Sub(Vec3_Add(center, size), anchorOffset);
    return a;
}

static inline AABB AABB_CreateCentered(Vec3 center, Vec3 size)
{
    return AABB_CreateAnchored(center, size, Vec3_Create(0.5f, 0.5f, 0.5f));
}

static inline int AABB_IntersectsAABB(AABB a, AABB b)
{
    for (int i = 0; i < 3; ++i)
    {
        if (Vec3_Get(a.max, i) < Vec3_Get(b.min, i))
            return 0;
        if (Vec3_Get(a.min, i) > Vec3_Get(b.max, i))
            return 0;
    }
    return 1;
}

static inline int AABB_IntersectsPoint(AABB a, Vec3 p)
{
    for (int i = 0; i < 3; ++i)
    {
        if (Vec3_Get(p, i) < Vec3_Get(a.min, i))
            return 0;
        if (Vec3_Get(p, i) > Vec3_Get(a.max, i))
            return 0;
    }
    return 1;
}

extern int AABB_IntersectsRay(AABB b, Ray3 ray);
extern int AABB_IntersectsPlane(AABB aabb, Plane plane);


static inline Vec3 AABB_Center(AABB a)
{
    return Vec3_Lerp(a.min, a.max, 0.5f);
}


static inline Vec3 AABB_Size(AABB a)
{
    return Vec3_Sub(a.max, a.min);
}

static inline Sphere Sphere_Create(Vec3 origin, float radius)
{
    Sphere s;
    s.origin = origin;
    s.radius = radius;
    return s;
}

extern int Sphere_IntersectsPoint(Sphere a, Vec3 p);
extern int Sphere_IntersectsAABB(Sphere a, AABB b);
extern int Sphere_IntersectsSphere(Sphere a, Sphere b);
extern int Sphere_IntersectsPlane(Sphere a, Plane p);
extern int Sphere_IntersectsRay(Sphere a, Ray3 ray, float* t);

// position.z of the plane is the offset along the normal
extern Mat4 Mat4_CreateShadow(Plane plane, Vec3 light);


extern void Frustum_Init(Frustum* frustum, float fov, float near, float far);

extern void Frustum_UpdateTransform(Frustum* frustum, short viewWidth, short viewHeight);
extern Vec3 Frustum_Unproject(const Frustum* frustum, Vec3 normalizedPoint);

extern const Mat4* Frustum_ViewMatrix(const Frustum* frustum);
extern const Mat4* Frustum_ProjMatrix(const Frustum* frustum);

extern int Frustum_SphereVisible(const Frustum* frustum, Sphere sphere);
extern int Frustum_PointVisible(const Frustum* frustum, Vec3 point);
extern int Frustum_AabbVisible(const Frustum* frustum, AABB aabb);

extern int Geo_PointInPoly(int nvert, Vec2* verts, Vec2 test);
extern Vec3 Geo_BlendBarcentric(Vec3 p1, Vec3 p2, Vec3 p3, Vec2 i);
extern int Geo_LineSegCross(Vec2 a, Vec2 b, Vec2 c, Vec2 d);

#endif
