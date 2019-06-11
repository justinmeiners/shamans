

#include "texture.h"
#include "platform.h"
#include "vec_math.h"

#if TEXTURE_STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR 1
#define STBI_NO_PSD 1
#define STBI_NO_GIF 1
#define STBI_NO_HDR 1
#define STBI_NO_PIC 1
#define STBI_NO_PNM 1
#define STBI_NO_BMP 1

#include "stb_image.h"
#endif

static void Texture_Init(Texture* texture, TextureFlags flags)
{
    assert(texture);

    texture->width = 0;
    texture->height = 0;
    texture->gpuId = 0;
    texture->purgeable = 1;
    
    texture->data = NULL;
    texture->dataLength = 0;
    texture->subimageCount = 0;
    
    texture->format = kTextureFormatUnknown;
    texture->type = kTexture2D;
    texture->flags = flags;
}

#if TEXTURE_STB
static int Texture_FromImage(Texture* texture, FILE* file)
{
    int x, y, comp;
    unsigned char* data = stbi_load_from_file(file, &x, &y, &comp, 0);
    
    if (!data)
    {
        printf("%s\n", stbi_failure_reason());
        return 0;
    }
        
    switch (comp)
    {
        case 4:
            texture->format = kTextureFormatRGBA;
            break;
        case 3:
            texture->format = kTextureFormatRGB;
            break;
        case 2:
            texture->format = kTextureFormatGA;
            break;
        case 1:
            texture->format = kTextureFormatG;
            break;
        default:
            stbi_image_free(data);
            return 0;
            break;
    }

    texture->dataLength = comp * x * y;
    texture->data = data;
    
    texture->width = x;
    texture->height = y;
    
    texture->subimageCount = 1;
    texture->subimageInfo[0].length = texture->dataLength;
    texture->subimageInfo[0].offset = 0;
    return 1;
}

#endif


// Nvidia Texture Tools - https://github.com/castano/nvidia-texture-tools
// S3 Texture Compression - https://www.opengl.org/wiki/S3_Texture_Compression

int Texture_FromDds(Texture* texture, FILE* file)
{
    enum
    {
        DDS_MAGIC = 542327876,
        FOURCC_DXT1 = 0x31545844,
        FOURCC_DXT3 = 0x33545844,
        FOURCC_DXT5 = 0x35545844,
        
        DDPF_FOURCC = 0x4, // for compressed
        DDPF_RGB = 0x40, // for uncompressed
        
        CAPS2_CUBEMAP = 0x200,
        CAPS2_CUBEMAP_XP = 0x400,
        CAPS2_CUBEMAP_XN = 0x800,
        CAPS2_CUBEMAP_YP = 0x1000,
        CAPS2_CUBEMAP_YN = 0x2000,
        CAPS2_CUBEMAP_ZP = 0x4000,
        CAPS2_CUBEMAP_ZN = 0x8000,
        
        CAPS2_CUBEMAP_ALL = CAPS2_CUBEMAP_XP |
        CAPS2_CUBEMAP_XN |
        CAPS2_CUBEMAP_YP |
        CAPS2_CUBEMAP_YN |
        CAPS2_CUBEMAP_ZP,
    };
    
    // https://msdn.microsoft.com/en-us/library/bb943982(v=vs.85).aspx
    struct
    {
        uint32_t magic;
        uint32_t size;
        uint32_t flags;
        uint32_t height;
        uint32_t width;
        uint32_t linearSize;
        uint32_t depth;
        uint32_t mipMapCount;
        uint32_t reserved1[11];
        
        // https://msdn.microsoft.com/en-us/library/bb943984(v=vs.85).aspx
        struct
        {
            uint32_t size;
            uint32_t flags;
            uint32_t fourCC;
            uint32_t rgbBitCount;
            uint32_t rBitMask;
            uint32_t gBitMask;
            uint32_t bBitMask;
            uint32_t aBitMask;
        } pixelFormat;
        
        uint32_t caps;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
        uint32_t reserved2;
    } ddsHeader;
    
    // begin reading header
    fread(&ddsHeader, sizeof(ddsHeader), 1, file);
    
    if (ddsHeader.magic != DDS_MAGIC)
        return 0;
    
    texture->width = End_ReadLittle32(ddsHeader.width);
    texture->height = End_ReadLittle32(ddsHeader.height);
    
    uint32_t caps2 = End_ReadLittle32(ddsHeader.caps2);
    
    if (caps2 & CAPS2_CUBEMAP)
    {
        if (caps2 & CAPS2_CUBEMAP_ALL)
        {
            texture->type = kTextureCube;
        }
        else
        {
            // partial cube map
            return 0;
        }
    }
    
    uint32_t flags = End_ReadLittle32(ddsHeader.pixelFormat.flags);
    uint32_t bitCount = End_ReadLittle32(ddsHeader.pixelFormat.rgbBitCount);
    size_t blockSize = 0;

    if (flags & DDPF_FOURCC)
    {
        int compressFormat = End_ReadLittle32(ddsHeader.pixelFormat.fourCC);
        
        if (compressFormat == FOURCC_DXT1)
        {
            texture->format = kTextureFormatDxt1;
            blockSize = 8;
        }
        else if (compressFormat == FOURCC_DXT3)
        {
            texture->format = kTextureFormatDxt3;
            blockSize = 16;
        }
        else if (compressFormat == FOURCC_DXT5)
        {
            texture->format = kTextureFormatDxt5;
            blockSize = 16;
        }
    }
    else if (flags & DDPF_RGB)
    {
        if (bitCount == 24)
        {
            texture->format = kTextureFormatRGB;
            bitCount = 24;
        }
        else if (bitCount == 32)
        {
            texture->format = kTextureFormatRGBA;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    
    if (texture->type == kTextureCube)
    {
        size_t imageLength = bitCount * texture->width * texture->height / 8;

        texture->dataLength = imageLength * kTextureCubeFaceCount;
        texture->subimageCount = kTextureCubeFaceCount;
        texture->data = malloc(texture->dataLength);

        size_t offset = 0;
        
        for (int i = 0; i < texture->subimageCount; ++i)
        {
            texture->subimageInfo[i].offset = offset;
            texture->subimageInfo[i].length = imageLength;
            fread(texture->data + offset, imageLength, 1, file);
            offset += imageLength;
        }
    }
    else
    {
        texture->subimageCount = End_ReadLittle32(ddsHeader.mipMapCount);
        
        if (texture->subimageCount >= TEXTURE_SUBIMAGE_MAX)
            return 0;
        
        texture->dataLength = 0;
        
        short width = texture->width;
        short height = texture->height;
        
        for (int i = 0; i < texture->subimageCount; ++i)
        {
            size_t mipLength = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
            
            texture->subimageInfo[i].length = mipLength;
            texture->subimageInfo[i].offset = texture->dataLength;
            texture->dataLength += mipLength;
            
            width = MAX(width >> 1, 1);
            height = MAX(height >> 1, 1);
        }
        
        texture->data = malloc(texture->dataLength);
        if (!texture->data)
            return 0;
        
        fread(texture->data, texture->dataLength, 1, file);
    }
    
    return 1;
}

/* https://developer.apple.com/library/ios/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/TextureTool/TextureTool.html */


static int Texture_FromPvr(Texture* texture, FILE* file)
{
    struct
    {
        uint32_t headerLength;
        uint32_t height;
        uint32_t width;
        uint32_t numMipmaps;
        uint32_t flags;
        uint32_t dataLength;
        uint32_t bpp;
        uint32_t bitmaskRed;
        uint32_t bitmaskGreen;
        uint32_t bitmaskBlue;
        uint32_t bitmaskAlpha;
        uint32_t pvrTag;
        uint32_t numSurfs;
    } pvrHeader;
    
    enum
    {
        kPvrTextureFlagTypeMask = 0xFF,
    };
    
    enum
    {
        kPvrTextureFlagPvrtc2 = 24,
        kPvrTextureFlagPvrtc4
    };
    
    fread(&pvrHeader, sizeof(pvrHeader), 1, file);
    
    uint32_t pvrTag = End_ReadLittle32(pvrHeader.pvrTag);
    
    const char* pvrIdentifier = "PVR!";
    
    if (pvrIdentifier[0] != ((pvrTag >>  0) & 0xff) ||
        pvrIdentifier[1] != ((pvrTag >>  8) & 0xff) ||
        pvrIdentifier[2] != ((pvrTag >> 16) & 0xff) ||
        pvrIdentifier[3] != ((pvrTag >> 24) & 0xff))
    {
        return 0;
    }
    
    texture->width = End_ReadLittle32(pvrHeader.width);
    texture->height = End_ReadLittle32(pvrHeader.height);
    
    int alpha = End_ReadLittle32(pvrHeader.bitmaskAlpha);

    uint32_t flags = End_ReadLittle32(pvrHeader.flags);
    uint32_t formatFlags = flags & kPvrTextureFlagTypeMask;
    switch (formatFlags)
    {
        case kPvrTextureFlagPvrtc2:
            texture->format = kTextureFormatPvr2;
            break;
        case kPvrTextureFlagPvrtc4:
            texture->format = kTextureFormatPvr4;
            break;
        default:
            return 0;
            
    }
    
    // alpha enums of these formats immediately follow their non alpha
    if (alpha) ++texture->format;
    
    texture->dataLength = End_ReadLittle32(pvrHeader.dataLength);
    texture->subimageCount = End_ReadLittle32(pvrHeader.numMipmaps) + 1;
    
    if (texture->subimageCount >= TEXTURE_SUBIMAGE_MAX)
        return 0;

    texture->data = malloc(texture->dataLength);

    if (!texture->data)
        return 0;
    
    int blockSize;
    int widthBlocks;
    int heightBlocks;
    int bpp;
    short width = texture->width;
    short height = texture->height;
    
    size_t offset = 0;
    
    int subimage = 0;
    
    /* read each mip */
    while (offset < texture->dataLength)
    {
        if (formatFlags == kPvrTextureFlagPvrtc4)
        {
            blockSize = 4 * 4;
            widthBlocks = width / 4;
            heightBlocks = height / 4;
            bpp = 4;
        }
        else
        {
            blockSize = 8 * 4;
            widthBlocks = width / 8;
            heightBlocks = height / 4;
            bpp = 2;
        }
        
        widthBlocks = MAX(widthBlocks, 2);
        heightBlocks = MAX(heightBlocks, 2);
        
        size_t mipLength = widthBlocks * heightBlocks * ((blockSize * bpp) / 8);
        
        texture->subimageInfo[subimage].offset = offset;
        texture->subimageInfo[subimage].length = mipLength;

        offset += mipLength;

        width = MAX(width >> 1, 1);
        height = MAX(height >> 1, 1);
        ++subimage;
    }
    
    fread(texture->data, 1, texture->dataLength, file);

    return 1;
}

int Texture_FromPath(Texture* texture, TextureFlags flags, const char* path)
{
    FILE* file = fopen(path, "rb");
    
    if (!file) return 0;
    
    int status = 0;
    
    Texture_Init(texture, flags);
    
    if (strcmp(Filepath_Extension(path), "dds") == 0)
    {
        status = Texture_FromDds(texture, file);
    }
    else if (strcmp(Filepath_Extension(path), "pvr") == 0)
    {
        status = Texture_FromPvr(texture, file);
    }
    else
    {
        status = Texture_FromImage(texture, file);
    }
    
    fclose(file);
    
    if (!status)
        return 0;
        
    return 1;
}

void Texture_Shutdown(Texture* texture)
{
    if (texture)
        Texture_Purge(texture);
}

void Texture_Purge(Texture* texture)
{
    if (texture->data)
    {
        free(texture->data);
        texture->data = NULL;
    }
}

