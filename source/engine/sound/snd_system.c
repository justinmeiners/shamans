#include "snd_system.h"
#include <assert.h>
#include "platform.h"

static void SndEmitter_Init(SndEmitter* source)
{
    assert(source);
    source->sound = NULL;
    source->volume = 1.0f;
    source->playing = 0;
    source->positionEnabled = 0;
    source->sampleIndex = 0;
    source->looping = 0;
    source->group = kSndGroupEffects;
}

static void SndEmitter_Restart(SndEmitter* source)
{
    source->sampleIndex = 0;
    source->subCounter = 0;
}

static int SndDriver_Prepare(SndDriver* driver)
{
    assert(!driver->running);
    
    if (driver->prime && driver->prime(driver))
    {
        return 1;
    }
    
    return 0;
}

static int SndDriver_Start(SndDriver* driver)
{
    if (driver->start && driver->start(driver))
    {
        driver->running = 1;
        return 1;
    }
    
    return 0;
}

static void SndDriver_Stop(SndDriver* driver)
{
    if (driver->running)
    {
        driver->running = 0;
        driver->stop(driver);
    }
}


static void SndDriver_Callback(void* delegate, struct SndDriver* driver, float* leftOut, float* rightOut)
{
    SndSystem* env = delegate;
    
    if (!env) return;
    
    float l = 0.0f;
    float r = 0.0f;
    
    for (int i = 0; i < SND_SYSTEM_MAX_EMITTERS; ++i)
    {
        float sourceLeft = 0.0f;
        float sourceRight = 0.0f;
        
        SndEmitter* emitter = env->emitters + i;
        
        if (!emitter->sound || !emitter->playing) continue;
        
        if (emitter->positionEnabled)
        {
            float val = Snd_UnpackSample(emitter->sound, emitter->sampleIndex, 0);
            Vec3 diff = Vec3_Norm(Vec3_Sub(emitter->position, env->listener.position));
            
            float dot = Vec3_Dot(diff, env->listener.right);
            
            if (dot > 0.0f)
            {
                sourceLeft = (val * dot);
            }
            else
            {
                sourceRight = (val * dot);
            }
        }
        else
        {
            sourceLeft = Snd_UnpackSample(emitter->sound, emitter->sampleIndex, 0);
            sourceRight = Snd_UnpackSample(emitter->sound, emitter->sampleIndex, 1);
        }
        
        ++emitter->subCounter;
        
        if (emitter->subCounter >= (driver->sampleRate / emitter->sound->sampleRate))
        {
            emitter->subCounter = 0;
            ++emitter->sampleIndex;
        }
        
        if (emitter->sampleIndex >= emitter->sound->sampleCount)
        {
            emitter->sampleIndex = 0;
            
            if (!emitter->looping)
            {
                emitter->playing = 0;
                emitter->sound = NULL;
            }
        }
        
        // apply emitter and group volume
        const SndGroupInfo* group = env->groups + emitter->group;
        
        l += sourceLeft * emitter->volume * group->volume;
        r += sourceRight * emitter->volume * group->volume;
    }
    
    l *= env->masterVolume;
    r *= env->masterVolume;
    
    // renormalize the value in case the sum of the emitters exceeded the range
    if (fabs(l) > 1.0f) l /= fabs(l);
    if (fabs(r) > 1.0f) r /= fabs(r);
    
    // assign the output
    *leftOut = l;
    *rightOut = r;
}


int SndSystem_Init(SndSystem* system, SndDriver* driver)
{
    assert(system);
    
    if (!system)
        return 0;
    
    if (driver)
    {
        system->driver = driver;
        system->driver->delegate = system;
        system->driver->callback = SndDriver_Callback;
    }
    else
    {
        system->driver = NULL;
    }
    
    // this number is < 1 because we dont want to constantly clamp the summed output
    system->masterVolume = 0.65f;
    
    for (int i = 0; i < kSndGroupCount; ++i)
        system->groups[i].volume = 1.0f;
    
    for (int i = 0; i < SND_SYSTEM_MAX_EMITTERS; ++i)
        SndEmitter_Init(system->emitters + i);
    
    system->ambient = NULL;
    
    if (driver)
    {
        SndDriver_Prepare(system->driver);
        SndDriver_Start(system->driver);
    }
    
    return 1;
}

void SndSystem_Shutdown(SndSystem* env)
{
    SndDriver_Stop(env->driver);
}


void SndSystem_LoadSound(SndSystem* system, int soundIndex, const char* path)
{
    assert(soundIndex < SND_SYSTEM_MAX_SNDS);
    Snd* sound = system->sounds + soundIndex;
    
    if (path == NULL)
    {
        Snd_Shutdown(sound);
        return;
    }
    
    char fullPath[MAX_OS_PATH];
    Filepath_Append(fullPath, Filepath_DataDir(), path);
    
    Snd_FromPath(sound, fullPath);
}

SndEmitter* SndSystem_PlaySound(SndSystem* system, int soundIndex)
{
    if (!system)
        return NULL;
    
    Snd* sndSource = system->sounds + soundIndex;
    
    SndEmitter* emitter = NULL;
    
    for (int i = 0; i < SND_SYSTEM_MAX_EMITTERS; i++)
    {
        if (system->emitters[i].sound == NULL)
        {
            emitter = system->emitters + i;
            break;
        }
    }
    
    if (!emitter)
        return NULL;
    
    emitter->sound = sndSource;
    emitter->soundIndex = soundIndex;
    
    SndEmitter_Restart(emitter);
    emitter->playing = 1;

    return emitter;
}

void SndSystem_SetAmbient(SndSystem* system, int sound)
{
    if (system->ambient)
    {
        system->ambient->playing = 0;
        SndEmitter_Restart(system->ambient);
    }
    
    system->ambient = SndSystem_PlaySound(system, sound);
    system->ambient->group = kSndGroupAmbient;
    system->ambient->looping = 1;
    system->ambient->playing = 1;
}

int SndSystem_IsPlaying(SndSystem* system, int soundIndex)
{
    for (int i = 0; i < SND_SYSTEM_MAX_EMITTERS; ++i)
    {
        const SndEmitter* emitter = system->emitters + i;
        
        if (emitter->playing && emitter->soundIndex == soundIndex)
            return 1;
    }
    
    return 0;
}
