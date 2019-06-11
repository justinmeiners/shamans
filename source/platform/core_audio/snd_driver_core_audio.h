
#ifndef SND_DRIVER_CORE_AUDIO_H
#define SND_DRIVER_CORE_AUDIO_H

#include "snd_driver.h"

// Core Audio is an Apple API. Mac/iOS

// 44100

#define SND_DRIVER_CORE_AUDIO_RATE_DEFAULT 44100

extern int SndDriver_CoreAudio_Init(SndDriver* driver, int sampleRate);

#endif
