
#ifndef HINT_BUFFER_H
#define HINT_BUFFER_H

#include "geo_math.h"
#include "skel.h"
#include "nav.h"

/* See Gui Buffer for design info.
 
 The hint buffer is for rendering all the lines/circles for 3D gameplay interface.*/


#define HINT_VERTS_MAX 4096

typedef struct
{
    Vec3 pos;
    Vec3 color;
} HintVert;

typedef struct
{
    unsigned int vaoGpuId;
    unsigned int vboGpuId;
    unsigned int iboGpuId;
    
    unsigned short indicies[HINT_VERTS_MAX];
    HintVert verts[HINT_VERTS_MAX];
    
    unsigned short indexCount;
    unsigned short vertCount;
    
} HintBuffer;

extern void HintBuffer_Init(HintBuffer* buffer);
extern void HintBuffer_Clear(HintBuffer* buffer);

extern void HintBuffer_PackCircle(HintBuffer* buffer,
                                  const float radius,
                                  const Vec3 position,
                                  const Vec3 color,
                                  const int slices);

extern void HintBuffer_PackLine(HintBuffer* buffer,
                                const Vec3 start,
                                const Vec3 end,
                                const Vec3 color);

extern void HintBuffer_PackAABB(HintBuffer* buffer,
                                const AABB aabb,
                                const Vec3 color);


extern void Hintbuffer_PackSkel(HintBuffer* buffer,
                                const Skel* skel,
                                const Vec3 position,
                                const Vec3 boneColor,
                                const Vec3 upColor);

extern void HintBuffer_PackPath(HintBuffer* buffer,
                                const NavPath* path,
                                const Vec3 color);

extern void HintBuffer_PackNav(HintBuffer* buffer,
                               const NavMesh* mesh,
                               const Vec3 color);

#endif
