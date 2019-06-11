

#include "gl_3.h"
#include "gl_compat.h"
#include "gl_prog.h"
#include <string.h>


/*
 I had some painful bugs working with VBOs and IBOs.
 Apparently IBOs are stored in the VAO state but VBOs are not.
 glVertexAttribPointer still keeps a reference to the VBO however.
 This has the following consequences:
 - rendering requires nothing to be bound but the VAO
 - updating the IBO requires the VAO to be bound
 - updating the VBO requires itself to be bound

 */

#define VBO_OFFSET(i) ((char *)NULL + (i))

enum
{
    kProgramWorld,
    kProgramObjectLit,
    kProgramObjectSolid,
    kProgramSkelLit,
    kProgramSkelSolid,
    kProgramPart,
    kProgramGui,
    kProgramHint,
    kProgramCount,
} ShaderType;

/*
 I now use separate model, view and projection matrices instead of a combined MVP. 
  Most often the model and view are neede separately for lighting and normal matrix construction.
 I was also worried that separate added 2 matrix multiplies to the shaders,
 but then realized they are only matrix * vector multiplies:
    projection * view * model * vertex
 is evaluated right to left
 */

enum
{
    kProgLocModel,
    kProgLocView,
    kProgLocProjection,
    kProgLocAlbedo,
    kProgLocLightmap,
    kProgLocObserverPositions,
    kProgLocObserverRadiiSq,
    kProgLocLightmapEnabled,
    kProgLocLightEnabled,
    
    kProgLocColor,
    kProgLocLightPoints,
    kProgLocLightRadii,
    kProgLocLightColors,
    kProgLocJointRotations,
    kProgLocJointOrigins,
    kProgLocVisibility,
    
    kProgLocAtlas,
    kProgLocNearPlane,
    kProgLocControlPoints,
    kProgLocPartCount,
    kProgLocPartEffect,
    kProgLocTime,
};


typedef struct
{
    GlProg programs[kProgramCount];
    
    int partVao;
    int partVbo;
} Gl2Context;


static int Gl_UploadSkelSkin(Renderer* gl, SkelSkin* skin)
{
    GLuint vboId, vaoId;
    
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);
    
    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    
    skin->vboGpuId = vboId;
    skin->vaoGpuId = vaoId;
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(SkelSkinVert) * skin->vertCount, skin->verts, GL_STATIC_DRAW);
    
    size_t offset = 0;
    
    glEnableVertexAttribArray(kGlAttribNormal);
    glVertexAttribPointer(kGlAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(SkelSkinVert), VBO_OFFSET(offset));
    offset += sizeof(Vec3);
    
    glEnableVertexAttribArray(kGlAttribUv0);
    glVertexAttribPointer(kGlAttribUv0, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(SkelSkinVert), VBO_OFFSET(offset));
    offset += sizeof(SkelSkinVertUv);
    
    for (int i = 0; i < SKEL_WEIGHTS_PER_VERT; ++i)
    {
        glEnableVertexAttribArray(kGlAttribWeight0 + i);
        glVertexAttribPointer(kGlAttribWeight0 + i, 4, GL_FLOAT, GL_FALSE, sizeof(SkelSkinVert), VBO_OFFSET(offset));
        offset += sizeof(Vec4);
    }
    
    glEnableVertexAttribArray(kGlAttribWeightJoints);
    glVertexAttribPointer(kGlAttribWeightJoints, SKEL_WEIGHTS_PER_VERT, GL_SHORT, GL_FALSE, sizeof(SkelSkinVert), VBO_OFFSET(offset));
    //offset += sizeof(short) * SKEL_WEIGHTS_PER_VERT;
    
    if (skin->purgeable)
    {
        SkelSkin_Purge(skin);
    }

    return 1;
}

static int Gl_CleanupSkelSkin(Renderer* gl, SkelSkin* skin)
{
    glDeleteBuffers(1, &skin->vboGpuId);
    glDeleteVertexArrays(1, &skin->vaoGpuId);

    return 1;
}

static int Gl_UploadMesh(Renderer* gl, StaticMesh* mesh)
{
    GLuint vboId, vaoId;
    
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);
    
    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    
    mesh->vboGpuId = vboId;
    mesh->vaoGpuId = vaoId;
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(StaticMeshVert) * mesh->vertCount, mesh->verts, GL_STATIC_DRAW);

    size_t offset = 0;
    
    glEnableVertexAttribArray(kGlAttribVertex);
    glVertexAttribPointer(kGlAttribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(StaticMeshVert), VBO_OFFSET(offset));
    offset += sizeof(Vec3);
    
    glEnableVertexAttribArray(kGlAttribNormal);
    glVertexAttribPointer(kGlAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(StaticMeshVert), VBO_OFFSET(offset));
    offset += sizeof(Vec3);
    
    for (int i = 0; i < mesh->uvChannelCount; ++i)
    {
        glEnableVertexAttribArray(kGlAttribUv0 + i);
        glVertexAttribPointer(kGlAttribUv0 + i, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(StaticMeshVert), VBO_OFFSET(offset));
        offset += sizeof(StaticMeshVertUv);
    }
    
    if (mesh->purgeable)
    {
        StaticMesh_Purge(mesh);
    }
    
    return 1;
}

static int Gl_CleanupMesh(Renderer* gl, StaticMesh* mesh)
{
    glDeleteVertexArrays(1, &mesh->vaoGpuId);
    glDeleteBuffers(1, &mesh->vboGpuId);
    
    return 1;
}

static GLenum Gl_GetTextureFormat(TextureFormat textureFormat)
{
    GLenum format;
    switch (textureFormat)
    {
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        case kTextureFormatDxt1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case kTextureFormatDxt3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case kTextureFormatDxt5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc
            /* I had an issue where PVR compressed textures were appearing especially dark.
             It turns out I specifying the sRGB gamma corrected format instead of the standard RGB format */
            
        case kTextureFormatPvr2:
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            break;
        case kTextureFormatPvr2a:
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            break;
        case kTextureFormatPvr4:
            format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            break;
        case kTextureFormatPvr4a:
            format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            break;
#endif
        case kTextureFormatRGB:
            format = GL_RGB8;
            break;
        case kTextureFormatRGBA:
            format = GL_RGBA8;
            break;
        case kTextureFormatG:
            format = GL_LUMINANCE;
            break;
        case kTextureFormatGA:
            format = GL_LUMINANCE_ALPHA;
            break;
        default:
            format = GL_RGBA;
            break;
    }
    
    return format;
}


static int Gl_UploadTexture(Renderer* gl, Texture* texture)
{
    if (texture->gpuId != 0)
    {
        GLuint gpuID = (GLuint)texture->gpuId;
        glDeleteTextures(1, &gpuID);
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum format = Gl_GetTextureFormat(texture->format);
    
    if (texture->subimageCount > 1)
    {
        short width = texture->width;
        short height = texture->height;
        
        int i;
        for (i = 0; i < texture->subimageCount; ++i)
        {
            glCompressedTexImage2D(GL_TEXTURE_2D, i, format, width, height, 0, (GLint)texture->subimageInfo[i].length, texture->data + texture->subimageInfo[i].offset);
            width = MAX((width >> 1), 1);
            height = MAX((height >> 1), 1);
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i - 1);
    }
    else
    {
        GLenum internalFormat = GL_RGBA;
        if (texture->format == kTextureFormatRGB)
        {
            internalFormat = GL_RGB;
        }
        else if (texture->format == kTextureFormatG)
        {
            internalFormat = GL_LUMINANCE;
        }
        else if (texture->format == kTextureFormatGA)
        {
            internalFormat = GL_LUMINANCE_ALPHA;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, internalFormat, GL_UNSIGNED_BYTE, texture->data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    if (texture->flags & kTextureFlagAtlas)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    texture->gpuId = tex;
    
    if (texture->purgeable)
    {
        Texture_Purge(texture);
        texture->data = NULL;
    }
    
    return 1;
}

static int Gl_CleanupTexture(Renderer* gl, Texture* texture)
{
    glDeleteTextures(1, &texture->gpuId);
    return 1;
}

static int Gl_PrepareHintBuffer(Renderer* gl, HintBuffer* buffer)
{
    GLuint vao, vbo, ibo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * HINT_VERTS_MAX, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, HINT_VERTS_MAX * sizeof(HintVert), NULL, GL_DYNAMIC_DRAW);
    
    size_t offset = 0;
    glEnableVertexAttribArray(kGlAttribVertex);
    glVertexAttribPointer(kGlAttribVertex, 3, GL_FLOAT, GL_FALSE, sizeof(HintVert), VBO_OFFSET(offset));
    offset += sizeof(Vec3);
    
    glEnableVertexAttribArray(kGlAttribColor);
    glVertexAttribPointer(kGlAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(HintVert), VBO_OFFSET(offset));
    //offset += sizeof(Vec3);
        
    buffer->vaoGpuId = vao;
    buffer->vboGpuId = vbo;
    buffer->iboGpuId = ibo;
    
    return 1;
}

static int Gl_CleanupHintBuffer(Renderer* gl, HintBuffer* buffer)
{
    glDeleteVertexArrays(1, &buffer->vaoGpuId);
    glDeleteBuffers(1, &buffer->iboGpuId);
    glDeleteBuffers(1, &buffer->vboGpuId);
    return 1;
}

static int Gl_PrepareGuiBuffer(Renderer* gl, GuiBuffer* buffer)
{
    GLuint vao, vbo, ibo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * GUI_VERTS_MAX, 0, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, GUI_VERTS_MAX * sizeof(GuiVert), 0, GL_DYNAMIC_DRAW);
    
    size_t offset = 0;
    glEnableVertexAttribArray(kGlAttribVertex);
    glVertexAttribPointer(kGlAttribVertex, 2, GL_FLOAT, GL_FALSE, sizeof(GuiVert), VBO_OFFSET(offset));
    offset += sizeof(Vec2);
    
    glEnableVertexAttribArray(kGlAttribUv0);
    glVertexAttribPointer(kGlAttribUv0, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(GuiVert), VBO_OFFSET(offset));
    offset += sizeof(GuiVertUv);
    
    glEnableVertexAttribArray(kGlAttribColor);
    glVertexAttribPointer(kGlAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GuiVert), VBO_OFFSET(offset));
    offset += sizeof(GuiVertColor);
    
    glEnableVertexAttribArray(kGlAttribAtlasId);
    glVertexAttribPointer(kGlAttribAtlasId, 1, GL_BYTE, GL_FALSE, sizeof(GuiVert), VBO_OFFSET(offset));

    buffer->vaoGpuId = vao;
    buffer->vboGpuId = vbo;
    buffer->iboGpuId = ibo;

    return 1;
}

static int Gl_CleanupGuiBuffer(Renderer* gl, GuiBuffer* buffer)
{
    glDeleteVertexArrays(1, &buffer->vaoGpuId);
    glDeleteBuffers(1, &buffer->iboGpuId);
    glDeleteBuffers(1, &buffer->vboGpuId);
    return 1;
}

static void Gl_InitPart(Renderer* gl)
{
    Gl2Context* ctx = gl->context;

    const size_t partBufferSize = 1024;
    unsigned short indexBuffer[partBufferSize];
    
    /* glVertexAttribIPointer is available in GL 3.0 but not ES 2.0 */
    
    for (int i = 0; i < partBufferSize; ++i)
        indexBuffer[i] = (unsigned short)i;
    
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, partBufferSize * sizeof(unsigned short), indexBuffer, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(kGlAttribIndex);
    glVertexAttribPointer(kGlAttribIndex, 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, VBO_OFFSET(0));
    
    ctx->partVao = vao;
    ctx->partVbo = vbo;
}

static void Gl_UpdateBuffers(Renderer* gl,
                              const Engine* engine,
                              const HintBuffer* hintBuffer,
                              const GuiBuffer* guiBuffer)
{
    
    /* upload hint buffer data */
    glBindVertexArray(hintBuffer->vaoGpuId);

    glBindBuffer(GL_ARRAY_BUFFER, hintBuffer->vboGpuId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned short) * hintBuffer->indexCount, hintBuffer->indicies);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(HintVert) * hintBuffer->vertCount, hintBuffer->verts);
    
    glBindVertexArray(guiBuffer->vaoGpuId);

    /* upload gui buffer data */
    glBindBuffer(GL_ARRAY_BUFFER, guiBuffer->vboGpuId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned short) * guiBuffer->indexCount, guiBuffer->indicies);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GuiVert) * guiBuffer->vertCount, guiBuffer->verts);
}

static void Gl_RenderWorld(Renderer* gl,
                           const Engine* engine,
                           const Frustum* cam,
                           const RenderList* renderList)
{
    glPushGroupMarkerEXT(0, "World");

    Gl2Context* ctx = gl->context;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GlProg* worldProg = ctx->programs + kProgramWorld;
    glUseProgram(worldProg->programId);
    
    Mat4 identity = Mat4_CreateIdentity();
    glUniformMatrix4fv(GlProg_UniformLoc(worldProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(worldProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(worldProg, kProgLocModel), 1, GL_FALSE, identity.m);
    
    glUniform3fv(GlProg_UniformLoc(worldProg, kProgLocObserverPositions), OBSERVERS_MAX, (float*)renderList->observerList.points);
    glUniform1fv(GlProg_UniformLoc(worldProg, kProgLocObserverRadiiSq), OBSERVERS_MAX, renderList->observerList.radiiSq);

    glUniform1i(GlProg_UniformLoc(worldProg, kProgLocAlbedo), 0);
    glUniform1i(GlProg_UniformLoc(worldProg, kProgLocLightmap), 1);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_WORLD].gpuId);
    
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_CULL_FACE);
    
    for (int i = 0; i < renderList->chunkCount; ++i)
    {
        const Chunk* chunk = engine->sceneSystem.chunks + renderList->chunks[i];
        const StaticModel* model = engine->renderSystem.models + chunk->model;
        
        glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[chunk->texture].gpuId);
        
        glBindVertexArray(model->mesh.vaoGpuId);
        glDrawArrays(GL_TRIANGLES, 0, model->mesh.vertCount);
    }
    
    glActiveTexture(GL_TEXTURE0);
    glPopGroupMarkerEXT();
}

static void Gl_RenderObjectOutlines(Renderer* gl,
                                    const Engine* engine,
                                    const Frustum* cam,
                                    const RenderList* renderList)
{
    glPushGroupMarkerEXT(0, "Outlines");

    Gl2Context* ctx = gl->context;
    const FogView* fogView = &engine->fogView;

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glStencilFunc(GL_EQUAL, 0, 0xFF);

    // Units
    // ------------------------------
    GlProg* skelProg = ctx->programs + kProgramSkelSolid;
    glUseProgram(skelProg->programId);
    
    glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);
    
    for (int i = 0; i < renderList->unitCount; ++i)
    {
        int unitIndex = renderList->units[i];
        const Unit* unit = engine->sceneSystem.units + unitIndex;
        
        glUniform4fv(GlProg_UniformLoc(skelProg, kProgLocJointRotations), unit->skelModel.skel.jointCount, (float*)unit->skelModel.skel.renderJointRotations);
        glUniform3fv(GlProg_UniformLoc(skelProg, kProgLocJointOrigins), unit->skelModel.skel.jointCount, (float*)unit->skelModel.skel.renderJointOrigins);

        // start with hidden pass
        glDepthFunc(GL_GEQUAL);
        glUniform4f(GlProg_UniformLoc(skelProg, kProgLocColor), 0.0f, 1.0f, 0.0f, 1.0f);
        
        Mat4 translate =  Mat4_CreateTranslate(unit->position);
        glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocModel), 1, GL_FALSE, translate.m);
        glUniform1f(GlProg_UniformLoc(skelProg, kProgLocVisibility), fogView->unitVisibility[unitIndex]);

        glBindVertexArray(unit->skelModel.skin.vaoGpuId);
        glDrawArrays(GL_TRIANGLES, 0, unit->skelModel.skin.vertCount);
        
        // switch to shadow (projection with stencil to avoid blending overlap)
        glDepthFunc(GL_LESS);
        glEnable(GL_STENCIL_TEST);

        glUniform4f(GlProg_UniformLoc(skelProg, kProgLocColor), 0.0f, 0.0f, 0.0f, 0.4f);
        glUniform1f(GlProg_UniformLoc(skelProg, kProgLocVisibility), 1.0f);

        Vec3 shadowLightPoint = Vec3_Create(2.0f, -2.0f, 20.0f);
        Vec3 shadowNormal = Vec3_Create(0.0f, 0.0f, 1.0f);
        
        if (unit->navPoly != NULL)
            shadowNormal = unit->navPoly->plane.normal;
        
        Mat4 object;
        Mat4 shadowTransform = Mat4_CreateShadow(Plane_Create(Vec3_Zero, shadowNormal), shadowLightPoint);
        Mat4_Mult(&translate, &shadowTransform, &object);
        
        glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocModel), 1, GL_FALSE, object.m);
        glDrawArrays(GL_TRIANGLES, 0, unit->skelModel.skin.vertCount);
        glDisable(GL_STENCIL_TEST);
    }
    
    glDepthFunc(GL_GEQUAL);
    glDisable(GL_BLEND);

    // Props
    // ------------------------------
    GlProg* objectProg = ctx->programs + kProgramObjectSolid;
    glUseProgram(objectProg->programId);
    
    glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);
    glUniform4f(GlProg_UniformLoc(objectProg, kProgLocColor), 0.0f, 1.0f, 0.0f, 1.0f);
    
    for (int i = 0; i < renderList->propCount; ++i)
    {
        int propIndex = renderList->props[i];
        const Prop* prop = engine->sceneSystem.props + propIndex;
        
        glUniform1f(GlProg_UniformLoc(objectProg, kProgLocVisibility), fogView->propVisibility[propIndex]);
        glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocModel), 1, GL_FALSE, prop->worldMatrix.m);

        glBindVertexArray(prop->model.mesh.vaoGpuId);
        glDrawArrays(GL_TRIANGLES, 0, prop->model.mesh.vertCount);
    }
    
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    glPopGroupMarkerEXT();
}

static void Gl_RenderObjects(Renderer* gl,
                             const Engine* engine,
                             const Frustum* cam,
                             const RenderList* renderList)
{
    glPushGroupMarkerEXT(0, "Objects");

    Gl2Context* ctx = gl->context;
    const FogView* fogView = &engine->fogView;
    
    Vec3 points[LIGHTS_PER_OBJECT];
    Vec3 colors[LIGHTS_PER_OBJECT];
    float radii[LIGHTS_PER_OBJECT];


    // Props
    // ------------------------------
    GlProg* objectProg = ctx->programs + kProgramObjectLit;
    glUseProgram(objectProg->programId);
    glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);

    for (int i = 0; i < renderList->propCount; ++i)
    {
        int propIndex = renderList->props[i];;
        const Prop* prop = engine->sceneSystem.props + propIndex;
        int lit = !(prop->model.material.flags & kMaterialFlagUnlit);
        
        glUniformMatrix4fv(GlProg_UniformLoc(objectProg, kProgLocModel), 1, GL_FALSE, prop->worldMatrix.m);
        glUniform1f(GlProg_UniformLoc(objectProg, kProgLocVisibility), fogView->propVisibility[propIndex]);
        glUniform1i(GlProg_UniformLoc(objectProg, kProgLocLightEnabled), lit);
        
        for (int j = 0; j < LIGHTS_PER_OBJECT; ++j)
        {
            int lightIndex = renderList->propLights[i].lights[j];
            
            if (lightIndex == -1)
            {
                colors[j] = Vec3_Zero;
                radii[j] = 1.0f;
            }
            else
            {
                const Light* light = engine->sceneSystem.lights + lightIndex;
                points[j] = light->point;
                colors[j] = light->color;
                radii[j] = light->radius;
            }
        }
        
        glUniform3fv(GlProg_UniformLoc(objectProg, kProgLocLightPoints), LIGHTS_PER_OBJECT, &points[0].x);
        glUniform3fv(GlProg_UniformLoc(objectProg, kProgLocLightColors), LIGHTS_PER_OBJECT, &colors[0].x);
        glUniform1fv(GlProg_UniformLoc(objectProg, kProgLocLightRadii), LIGHTS_PER_OBJECT, radii);
        
        glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[prop->model.material.diffuseMap].gpuId);

        glBindVertexArray(prop->model.mesh.vaoGpuId);
        glDrawArrays(GL_TRIANGLES, 0, prop->model.mesh.vertCount);
    }
   
    glDisable(GL_CULL_FACE);

    // Units
    // ------------------------------
    GlProg* skelProg = ctx->programs + kProgramSkelLit;
    glUseProgram(skelProg->programId);
    glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);

    for (int i = 0; i < renderList->unitCount; ++i)
    {
        int unitIndex = renderList->units[i];
        const Unit* unit = engine->sceneSystem.units + unitIndex;
        
        glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[unit->skelModel.material.diffuseMap].gpuId);
        glBindVertexArray(unit->skelModel.skin.vaoGpuId);

        glUniform4fv(GlProg_UniformLoc(skelProg, kProgLocJointRotations), unit->skelModel.skel.jointCount, (float*)unit->skelModel.skel.renderJointRotations);
        glUniform3fv(GlProg_UniformLoc(skelProg, kProgLocJointOrigins), unit->skelModel.skel.jointCount, (float*)unit->skelModel.skel.renderJointOrigins);
        glUniform1f(GlProg_UniformLoc(skelProg, kProgLocVisibility), fogView->unitVisibility[unitIndex]);
        
        for (int j = 0; j < LIGHTS_PER_OBJECT; ++j)
        {
            int lightIndex = renderList->unitLights[i].lights[j];
            
            if (lightIndex == -1)
            {
                colors[j] = Vec3_Zero;
                radii[j] = 1.0f;
            }
            else
            {
                const Light* light = engine->sceneSystem.lights + lightIndex;
                points[j] = light->point;
                colors[j] = light->color;
                radii[j] = light->radius;
            }
        }
        
        glUniform3fv(GlProg_UniformLoc(skelProg, kProgLocLightPoints), LIGHTS_PER_OBJECT, &points[0].x);
        glUniform3fv(GlProg_UniformLoc(skelProg, kProgLocLightColors), LIGHTS_PER_OBJECT, &colors[0].x);
        glUniform1fv(GlProg_UniformLoc(skelProg, kProgLocLightRadii), LIGHTS_PER_OBJECT, radii);

        Mat4 translate = Mat4_CreateTranslate(unit->position);
        glUniformMatrix4fv(GlProg_UniformLoc(skelProg, kProgLocModel), 1, GL_FALSE, translate.m);

        glDrawArrays(GL_TRIANGLES, 0, unit->skelModel.skin.vertCount);
    }
    
    glEnable(GL_CULL_FACE);
    
    glPopGroupMarkerEXT();


}

static void Gl_RenderPart(Renderer* gl,
                          const Engine* engine,
                          const Frustum* cam,
                          const RenderList* renderList)
{
    /*
     glDrawArraysInstanced may be a fit here, but it is not compatible with ES 2.0.
     GL 3.1 > and ES 3.0 >
     */
    if (renderList->emitterCount < 1)
        return;
    
    glPushGroupMarkerEXT(0, "Particles");
    
    Gl2Context* ctx = gl->context;
    
    glEnable(GL_BLEND);
    
#ifdef GL_VERTEX_PROGRAM_POINT_SIZE
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    
    glDepthMask(GL_FALSE);

    GlProg* partProgram = ctx->programs + kProgramPart;
    glUseProgram(partProgram->programId);
    glUniformMatrix4fv(GlProg_UniformLoc(partProgram, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(partProgram, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);

    glUniform1i(GlProg_UniformLoc(partProgram, kProgLocAtlas), 0);
    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_PART].gpuId);
    glUniform1f(GlProg_UniformLoc(partProgram, kProgLocNearPlane), cam->nearHeight * engine->renderSystem.scaleFactor);
    
    glBindVertexArray(ctx->partVao);
    
    for (int i = 0; i < renderList->emitterCount; ++i)
    {
        const PartEmitter* emitter = engine->partSystem.emitters + renderList->emitters[i];
        
        glUniform3fv(GlProg_UniformLoc(partProgram, kProgLocControlPoints), PART_EMITTER_NODES_MAX, (float*)emitter->controlPoints);
        glUniformMatrix4fv(GlProg_UniformLoc(partProgram, kProgLocModel), 1, GL_FALSE, emitter->worldMatrix.m);
        
        glUniform1i(GlProg_UniformLoc(partProgram, kProgLocPartCount), emitter->partCount);
        glUniform1i(GlProg_UniformLoc(partProgram, kProgLocPartEffect), emitter->effect);
        glUniform1f(GlProg_UniformLoc(partProgram, kProgLocTime), PartEmitter_CalcTime(emitter));
        
        glDrawArrays(GL_POINTS, 0, emitter->partCount);
    }
    
    glDepthMask(GL_TRUE);
    
    glPopGroupMarkerEXT();
}

static void Gl_RenderGui(Renderer* gl,
                         const Engine* engine,
                         const Frustum* cam,
                         const GuiBuffer* buffer)
{
    glPushGroupMarkerEXT(0, "Gui");

    Gl2Context* ctx = gl->context;
    
    Mat4 ortho = Mat4_CreateOrtho(0.0f, (float)engine->guiSystem.guiWidth, 0.0f, (float)engine->guiSystem.guiHeight, -1.0f, 1.0f);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    GlProg* gui = ctx->programs + kProgramGui;
    glUseProgram(gui->programId);
    glUniformMatrix4fv(GlProg_UniformLoc(gui, kProgLocProjection), 1, GL_FALSE, ortho.m);

    GLint uniformIndices[] = {0, 1, 2, 3};
    glUniform1iv(GlProg_UniformLoc(gui, kProgLocAtlas), 4, uniformIndices);

    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_GUI_SKIN].gpuId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_GUI_FONT].gpuId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_GUI_ITEMS].gpuId);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, engine->renderSystem.textures[TEX_GUI_CONTROLS].gpuId);

    glBindVertexArray(buffer->vaoGpuId);
    glDrawElements(GL_TRIANGLES, buffer->indexCount, GL_UNSIGNED_SHORT, NULL);
    
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_BLEND);
    
    glPopGroupMarkerEXT();
}

static void Gl_RenderHints(Renderer* gl,
                           const Engine* engine,
                           const Frustum* cam,
                           const HintBuffer* buffer)
{
    if (buffer->vertCount < 1)
        return;
    
    if (gl->debug)
        glDisable(GL_DEPTH_TEST);
    
    Gl2Context* ctx = gl->context;

    GlProg* hintProg = ctx->programs + kProgramHint;
    glUseProgram(hintProg->programId);
    glUniformMatrix4fv(GlProg_UniformLoc(hintProg, kProgLocProjection), 1, GL_FALSE, Frustum_ProjMatrix(cam)->m);
    glUniformMatrix4fv(GlProg_UniformLoc(hintProg, kProgLocView), 1, GL_FALSE, Frustum_ViewMatrix(cam)->m);

    glBindVertexArray(buffer->vaoGpuId);
    glDrawElements(GL_LINES, buffer->indexCount, GL_UNSIGNED_SHORT, NULL);
}

static void Gl_Render(Renderer* gl,
                      const Engine* engine,
                      const Frustum* cam,
                      const RenderList* renderList)
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glScissor(0, 0, engine->renderSystem.viewportWidth * engine->renderSystem.scaleFactor, engine->renderSystem.viewportHeight * engine->renderSystem.scaleFactor);

    /* "When you need to modify OpenGL ES resources, schedule those modifications at the beginning or end of a frame." */
    /* profiling shows UpdateBuffers is more effecient at the beginning than the end */
    Gl_UpdateBuffers(gl, engine, &engine->renderSystem.hintBuffer, &engine->guiSystem.buffer);

    Gl_RenderWorld(gl, engine, cam, renderList);
    Gl_RenderObjectOutlines(gl, engine, cam, renderList);
    Gl_RenderObjects(gl, engine, cam, renderList);
    Gl_RenderHints(gl, engine, cam, &engine->renderSystem.hintBuffer);
    Gl_RenderPart(gl, engine, cam, renderList);
    Gl_RenderGui(gl, engine, cam, &engine->guiSystem.buffer);
}

static void Gl_BeginLoading(Renderer* gl)
{
    glPushGroupMarkerEXT(0, "Loading");
}

static void Gl_EndLoading(Renderer* gl)
{
    glFlush();
    glPopGroupMarkerEXT();
}

static int Gl_CheckExtension(Renderer* gl, const char* extension, const char* extensionString)
{
    if (!extension || !extensionString)
        return 0;
    
    const char* p = extensionString;
    
    size_t extNameLen = strlen(extension);
    
    while (*p != '\0')
    {
        size_t n = strcspn(p, " ");
        
        if ((extNameLen == n) && (strncmp(extension, p, n) == 0)) {
            return 1;
        }
        p += (n + 1);
    }
    
    return 0;
}

static int Gl_CheckCompatibility(Renderer* gl, RendererLimits* limits)
{
    printf("vendor: %s\n", glGetString(GL_VENDOR));
    printf("renderer: %s\n", glGetString(GL_RENDERER));
    printf("version: %s\n", glGetString(GL_VERSION));
    
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    printf("%s\n", extensions);
    
    if (extensions)
    {
        if (Gl_CheckExtension(gl, "OES_vertex_array_object", extensions))
        {
            printf("missing OES_vertex_array_object\n");
            return 0;
        }
    }
    
    GLint components = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &components);
    printf("vertex uniform components: %i\n", components);
    
    limits->maxSkelJoints = components / 4;
    
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    limits->maxTextureSize = maxTextureSize;
    
    return 1;
}

static void Gl_Shutdown(Renderer* gl)
{
    Gl2Context* ctx = gl->context;
    
    for (int i = 0; i < kProgramCount; ++ i)
        GlProg_Shutdown(ctx->programs + i);
}

int Gl2_Init(Renderer* gl, const char* shaderDirectory)
{
    Gl2Context* ctx = malloc(sizeof(Gl2Context));

    if (!ctx) return 0;
    
    gl->context = ctx;
    gl->shutdown = Gl_Shutdown;
    gl->render = Gl_Render;
    gl->uploadTexture = Gl_UploadTexture;
    gl->uploadMesh = Gl_UploadMesh;
    gl->uploadSkelSkin = Gl_UploadSkelSkin;
    gl->cleanupSkelSkin = Gl_CleanupSkelSkin;
    gl->cleanupTexture = Gl_CleanupTexture;
    gl->cleanupMesh = Gl_CleanupMesh;
    
    gl->prepareGuiBuffer = Gl_PrepareGuiBuffer;
    gl->cleanupGuiBuffer = Gl_CleanupGuiBuffer;
    gl->prepareHintBuffer = Gl_PrepareHintBuffer;
    gl->cleanupHintBuffer = Gl_CleanupHintBuffer;
    
    gl->beginLoading = Gl_BeginLoading;
    gl->endLoading = Gl_EndLoading;
    
    
    if (!Gl_CheckCompatibility(gl, &gl->limits))
    {
        printf("hardware does not meet basic OpenGL requirements.\n");
        return 0;
    }
    
    glPushGroupMarkerEXT(0, "Loading Shaders");
    
    glCullFace(GL_BACK);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char vertPath[MAX_OS_PATH];
    char fragPath[MAX_OS_PATH];
    
    // world shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "world.vs");
    Filepath_Append(fragPath, shaderDirectory, "world.fs");
    
    GlProg* world = ctx->programs + kProgramWorld;
    GlProg_InitWithPaths(world, vertPath, fragPath);
    
    GlProg_BindAttrib(world, kGlAttribVertex, "a_vertex");
    GlProg_BindAttrib(world, kGlAttribUv0, "a_uv0");
    GlProg_BindAttrib(world, kGlAttribUv1, "a_uv1");
    GlProg_Link(world, 1);
    
    GlProg_MapUniform(world, "u_albedo", kProgLocAlbedo);
    GlProg_MapUniform(world, "u_lightmap", kProgLocLightmap);
    GlProg_MapUniform(world, "u_model", kProgLocModel);
    GlProg_MapUniform(world, "u_view", kProgLocView);
    GlProg_MapUniform(world, "u_projection", kProgLocProjection);
    GlProg_MapUniform(world, "u_observerPositions[0]", kProgLocObserverPositions);
    GlProg_MapUniform(world, "u_observerRadiiSq[0]", kProgLocObserverRadiiSq);
    
    // object shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "object_lit.vs");
    Filepath_Append(fragPath, shaderDirectory, "object_lit.fs");
    
    GlProg* object = ctx->programs + kProgramObjectLit;
    GlProg_InitWithPaths(object, vertPath, fragPath);
    
    GlProg_BindAttrib(object, kGlAttribVertex, "a_vertex");
    GlProg_BindAttrib(object, kGlAttribNormal, "a_normal");
    GlProg_BindAttrib(object, kGlAttribUv0, "a_uv0");
    GlProg_Link(object, 1);
    
    GlProg_MapUniform(object, "u_model", kProgLocModel);
    GlProg_MapUniform(object, "u_view", kProgLocView);
    GlProg_MapUniform(object, "u_projection", kProgLocProjection);
    GlProg_MapUniform(object, "u_albedo", kProgLocAlbedo);
    GlProg_MapUniform(object, "u_visibility", kProgLocVisibility);
    GlProg_MapUniform(object, "u_lightEnabled", kProgLocLightEnabled);
    GlProg_MapUniform(object, "u_lightPoints[0]", kProgLocLightPoints);
    GlProg_MapUniform(object, "u_lightRadii[0]", kProgLocLightRadii);
    GlProg_MapUniform(object, "u_lightColors[0]", kProgLocLightColors);
    
    
    // object soild shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "object_solid.vs");
    Filepath_Append(fragPath, shaderDirectory, "object_solid.fs");
    
    GlProg* objectSolid = ctx->programs + kProgramObjectSolid;
    GlProg_InitWithPaths(objectSolid, vertPath, fragPath);
    
    GlProg_BindAttrib(objectSolid, kGlAttribVertex, "a_vertex");
    GlProg_Link(objectSolid, 1);
    
    GlProg_MapUniform(objectSolid, "u_model", kProgLocModel);
    GlProg_MapUniform(objectSolid, "u_view", kProgLocView);
    GlProg_MapUniform(objectSolid, "u_projection", kProgLocProjection);
    GlProg_MapUniform(objectSolid, "u_color", kProgLocColor);
    GlProg_MapUniform(objectSolid, "u_visibility", kProgLocVisibility);
    
    // Skeleton shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "skel_lit.vs");
    Filepath_Append(fragPath, shaderDirectory, "skel_lit.fs");
    
    GlProg* skel = ctx->programs + kProgramSkelLit;
    GlProg_InitWithPaths(skel, vertPath, fragPath);
    
    GlProg_BindAttrib(skel, kGlAttribNormal, "a_normal");
    GlProg_BindAttrib(skel, kGlAttribUv0, "a_uv0");
    GlProg_BindAttrib(skel, kGlAttribWeightJoints, "a_weightJoints");
    GlProg_BindAttrib(skel, kGlAttribWeight0, "a_weight0");
    GlProg_BindAttrib(skel, kGlAttribWeight1, "a_weight1");
    GlProg_BindAttrib(skel, kGlAttribWeight2, "a_weight2");
    
    GlProg_Link(skel, 1);
    
    GlProg_MapUniform(skel, "u_model", kProgLocModel);
    GlProg_MapUniform(skel, "u_view", kProgLocView);
    GlProg_MapUniform(skel, "u_projection", kProgLocProjection);
    GlProg_MapUniform(skel, "u_albedo", kProgLocAlbedo);
    GlProg_MapUniform(skel, "u_visibility", kProgLocVisibility);
    GlProg_MapUniform(skel, "u_lightPoints[0]", kProgLocLightPoints);
    GlProg_MapUniform(skel, "u_lightRadii[0]", kProgLocLightRadii);
    GlProg_MapUniform(skel, "u_lightColors[0]", kProgLocLightColors);
    GlProg_MapUniform(skel, "u_jointRotations[0]", kProgLocJointRotations);
    GlProg_MapUniform(skel, "u_jointOrigins[0]", kProgLocJointOrigins);
    
    // Skeleton solid shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "skel_solid.vs");
    Filepath_Append(fragPath, shaderDirectory, "skel_solid.fs");
    
    GlProg* skelSolid = ctx->programs + kProgramSkelSolid;
    GlProg_InitWithPaths(skelSolid, vertPath, fragPath);
    
    GlProg_BindAttrib(skelSolid, kGlAttribWeightJoints, "a_weightJoints");
    GlProg_BindAttrib(skelSolid, kGlAttribWeight0, "a_weight0");
    GlProg_BindAttrib(skelSolid, kGlAttribWeight1, "a_weight1");
    GlProg_BindAttrib(skelSolid, kGlAttribWeight2, "a_weight2");
    
    GlProg_Link(skelSolid, 1);
    
    GlProg_MapUniform(skelSolid, "u_model", kProgLocModel);
    GlProg_MapUniform(skelSolid, "u_view", kProgLocView);
    GlProg_MapUniform(skelSolid, "u_projection", kProgLocProjection);
    GlProg_MapUniform(skelSolid, "u_color", kProgLocColor);
    GlProg_MapUniform(skelSolid, "u_visibility", kProgLocVisibility);
    GlProg_MapUniform(skelSolid, "u_jointRotations[0]", kProgLocJointRotations);
    GlProg_MapUniform(skelSolid, "u_jointOrigins[0]", kProgLocJointOrigins);
    
    // GUI shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "gui.vs");
    Filepath_Append(fragPath, shaderDirectory, "gui.fs");
    
    GlProg* gui = &ctx->programs[kProgramGui];
    GlProg_InitWithPaths(gui, vertPath, fragPath);
    
    GlProg_BindAttrib(gui, kGlAttribVertex, "a_vertex");
    GlProg_BindAttrib(gui, kGlAttribColor, "a_color");
    GlProg_BindAttrib(gui, kGlAttribUv0, "a_uv0");
    GlProg_BindAttrib(gui, kGlAttribAtlasId, "a_atlasId");
    
    GlProg_Link(gui, 1);
    
    GlProg_MapUniform(gui, "u_projection", kProgLocProjection);
    GlProg_MapUniform(gui, "u_atlas[0]", kProgLocAtlas);
    
    
    // ------------------------------------
    // Hint shader
    Filepath_Append(vertPath, shaderDirectory, "hint.vs");
    Filepath_Append(fragPath, shaderDirectory, "hint.fs");
    
    GlProg* hint = &ctx->programs[kProgramHint];
    GlProg_InitWithPaths(hint, vertPath, fragPath);
    
    GlProg_BindAttrib(hint, kGlAttribVertex, "a_vertex");
    GlProg_BindAttrib(hint, kGlAttribColor, "a_color");
    
    GlProg_Link(hint, 1);
    
    GlProg_MapUniform(hint, "u_view", kProgLocView);
    GlProg_MapUniform(hint, "u_projection", kProgLocProjection);
    
    // Particle shader
    // ------------------------------------
    Filepath_Append(vertPath, shaderDirectory, "part.vs");
    Filepath_Append(fragPath, shaderDirectory, "part.fs");
    
    GlProg* part = &ctx->programs[kProgramPart];
    GlProg_InitWithPaths(part, vertPath, fragPath);
    
    GlProg_BindAttrib(part, kGlAttribIndex, "a_index");
    GlProg_Link(part, 1);
    
    GlProg_MapUniform(part, "u_model", kProgLocModel);
    GlProg_MapUniform(part, "u_view", kProgLocView);
    GlProg_MapUniform(part, "u_projection", kProgLocProjection);
    GlProg_MapUniform(part, "u_atlas", kProgLocAtlas);
    GlProg_MapUniform(part, "u_nearPlane", kProgLocNearPlane);
    GlProg_MapUniform(part, "u_controlPoints[0]", kProgLocControlPoints);
    GlProg_MapUniform(part, "u_partCount", kProgLocPartCount);
    GlProg_MapUniform(part, "u_partEffect", kProgLocPartEffect);
    GlProg_MapUniform(part, "u_time", kProgLocTime);
    
    Gl_InitPart(gl);
    
    glPopGroupMarkerEXT();
    
    return 1;
}

void Gl2_Shutdown(Renderer* renderer)
{
    if (renderer->context)
        free(renderer->context);
}
