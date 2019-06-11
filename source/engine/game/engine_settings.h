
#ifndef ENGINE_SETTINGS_H
#define ENGINE_SETTINGS_H

#include "input.h"

#define BUILD_DEBUG 1
#define BUILD_SOUND 1

typedef enum
{
    kEngineResultNone = 0,
    kEngineResultQuit,
    kEngineResultRestart,
    kEngineResultVictory,
    kEngineResultDefeat,
} EngineResult;

typedef enum
{
    kEngineStateInit = 0,
    kEngineStateCommand, /* currently playing out a game command */
    kEngineStateAwait, /* awaiting network confirmaiton of completed command */
    kEngineStateIdle, /* awaiting commands, either from player, AI, or network */
    kEngineStateEnd,
} EngineState;

typedef struct
{
    const char* dataPath;
    const char* levelPath;
    
    InputConfig inputConfig;
    short guiWidth;
    short guiHeight;
    short renderWidth;
    short renderHeight;
    float renderScaleFactor;
} EngineSettings;

#endif
