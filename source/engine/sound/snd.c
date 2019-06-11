
#include "snd.h"
#include "platform.h"

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>

#include "stb_vorbis.h"


/*
 https://developer.valvesoftware.com/wiki/Env_speaker
 https://developer.valvesoftware.com/wiki/Env_microphone
*/

int Snd_Init(Snd* snd,
             SndFormat format,
             size_t length,
             int channelCount,
             int sampleRate,
             int bytesPerSecond,
             short bitsPerSample)
{
    assert(snd);
    assert(bitsPerSample == 8 || bitsPerSample == 16 || bitsPerSample == 32);
    
    memset(snd, 0x0, sizeof(Snd));
    
    snd->format = format;
    
    snd->bitsPerSample = bitsPerSample;
    snd->sampleCount = (uint)length;
    snd->channelCount = channelCount;
    
    snd->sampleRate = sampleRate;
    snd->bytesPerSecond = bytesPerSecond;
    
    snd->data.p = malloc((bitsPerSample / 8) * length * channelCount);
    
    if (!snd->data.p) return 0;
    
    snd->format = kSndFormatRaw;
    return 1;

}

void Snd_Shutdown(Snd* snd)
{
    if (snd->data.p)
        free(snd->data.p);
}

void Snd_GenRand(Snd* snd, float amp)
{
    for (int i = 0; i < snd->sampleCount; ++i)
    {
        for (int j = 0; j < snd->channelCount; ++j)
            Snd_PackSample(snd, i * snd->channelCount, j, (rand() / (float)INT_MAX) * amp);
    }
}

void Snd_GenSquare(Snd* snd, int frequency, float amp)
{
}

float Snd_UnpackSample(const Snd* snd, int frame, int channel)
{
    assert(snd);
    assert(frame >= 0 && frame < snd->sampleCount);
    
    if (channel >= snd->channelCount)
        channel = snd->channelCount - 1;
    
    uint index = (frame * snd->channelCount) + channel;
    
    float value = 0.0f;
    
    switch (snd->bitsPerSample)
    {
        case 8:
            value = snd->data.p8[index] / (float)CHAR_MAX;
            break;
        case 16:
            value = snd->data.p16[index] / (float)SHRT_MAX;
            break;
        case 32:
            value = snd->data.p32[index];
            break;
            
        default:
            assert(0);
            break;
    }
    assert(value <= 1.01f && value >= -1.01f);

    return value;
}

void Snd_PackSample(Snd* snd, int frame, int channel, float val)
{
    assert(snd);
    assert(frame >= 0 && frame < snd->sampleCount);
    
    if (channel >= snd->channelCount)
        channel = snd->channelCount - 1;
    
    uint index = (frame * snd->channelCount) + channel;
    
    switch (snd->bitsPerSample)
    {
        case 8:
            snd->data.p8[index] = val * (float)CHAR_MAX;
            break;
        case 16:
            snd->data.p16[index] = val * (float)SHRT_MAX;
            break;
        case 32:
            snd->data.p32[index] = val;
            break;
        default:
            assert(0);
            break;
    }
}

uint Snd_SampleBufferLength(const Snd* snd)
{
    return snd->sampleCount * (snd->bitsPerSample / 8);
}



/* https://ccrma.stanford.edu/courses/422/projects/WaveFormat/  */
/* http://www.sonicspot.com/guide/wavefiles.html */

static int Snd_FromWAV(Snd* snd, FILE* file)
{
    struct
    {
        char riff[4];      // 'RIFF'
        uint32_t chunkSize;
        char wav[4];      // 'WAVE'
    } header;
    
    struct
    {
        char format[4];
        uint32_t chunkDataSize;
        uint16_t compressionCode;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t bytesPerSecond;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    } chunkFormat;
    
    struct
    {
        char dataID[4];
        uint32_t chunkSize;
    } dataChunk;
    
    fread(&header, sizeof(header), 1, file);
    fread(&chunkFormat, sizeof(chunkFormat), 1, file);
    fread(&dataChunk, sizeof(dataChunk), 1, file);
    
    assert(chunkFormat.compressionCode == 1);
    
    size_t length = (dataChunk.chunkSize / chunkFormat.channelCount) / (chunkFormat.bitsPerSample / 8);
    
    Snd_Init(snd,
             kSndFormatRaw,
             length,
             chunkFormat.channelCount,
             chunkFormat.sampleRate,
             chunkFormat.bytesPerSecond,
             chunkFormat.bitsPerSample);
    
    fread(snd->data.p, dataChunk.chunkSize, 1, file);
    return 1;
}

#if SND_STB_VORBIS

static int Snd_FromOGG(Snd* snd, FILE* file)
{
    int error = 0;
    stb_vorbis* v = stb_vorbis_open_file(file, 1, &error, NULL);
    
    if (v == NULL || error) return 0;
    
    snd->sampleRate = v->sample_rate;
    snd->channelCount = v->channels;
    snd->bitsPerSample = 16;
    snd->format = kSndFormatRaw;
    int limit = v->channels * 4096;

    short* data = malloc(limit * sizeof(short));
    if (!data)
    {
        stb_vorbis_close(v);
        return 0;
    }
    
    int total = limit;
    int offset = 0;
    int sampleCount = 0;
    
    while (1)
    {
        int n = stb_vorbis_get_frame_short_interleaved(v, v->channels, data+offset, total-offset);
        if (n == 0) break;
        sampleCount += n;
        
        offset += n * v->channels;
        if (offset + limit > total) {
            short *data2;
            total *= 2;
            data2 = (short *) realloc(data, total * sizeof(*data));
            if (data2 == NULL) {
                free(data);
                stb_vorbis_close(v);
                return 0;
            }
            data = data2;
        }
    }
    stb_vorbis_close(v);

    snd->data.p16 = data;
    snd->sampleCount = sampleCount;
    
    return 1;
}

#endif


int Snd_FromPath(Snd* sound, const char* path)
{
    FILE* file = fopen(path, "rb");
    
    if (!file) return 0;
    
    int status = 0;
    
    if (strcmp(Filepath_Extension(path), "wav") == 0)
    {
        status = Snd_FromWAV(sound, file);
    }
    else if (strcmp(Filepath_Extension(path), "ogg") == 0)
    {
        #if SND_STB_VORBIS
            status = Snd_FromOGG(sound, file);
        #else
            status = 0;
        #endif

    }
    
    fclose(file);
    
    return status;
}


