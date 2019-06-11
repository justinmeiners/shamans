#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include "input.h"

typedef struct
{
    InputConfig config;
    
    InputInfo info;
    
    InputState last;
    InputState current;
    
    int panning;
    int panIdentifier1;
    int panIdentifier2;
} InputSystem;

extern int InputSystem_Init(InputSystem* system, InputConfig config);

extern void InputSystem_ProcessInput(InputSystem* system, const InputState* newState);



#endif
