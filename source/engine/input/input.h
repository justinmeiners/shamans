
#ifndef INPUT_H
#define INPUT_H

typedef enum
{
    kInputConfigMouseKeyboard = 0,
    kInputConfigMultitouch,
} InputConfig;

typedef enum
{
    kInputPhaseNone = 0,
    kInputPhaseBegin,
    kInputPhaseEnd,
    kInputPhaseMove,
} InputPhase;

#define INPUT_TOUCHES_MAX 5
#define MOUSE_BUTTONS_MAX 3

typedef enum
{
    kMouseButtonLeft = 0,
    kMouseButtonMid,
    kMouseButtonRight,
    kMouseButtonCount,
} MouseButton;

typedef struct
{
    struct
    {
        float x;
        float y;
    } mouseCursor;
    
    struct
    {
        short down;
        short phase;
    } mouseButtons[kMouseButtonCount];
    
    struct
    {
        float x;
        float y;
        
        int identifier;
        InputPhase phase;
    } touches[INPUT_TOUCHES_MAX];
    
    int touchCount;
    
    
} InputState;

extern void InputState_Init(InputState* state);


typedef struct
{
    struct
    {
        short handled;
    } mouseButtons[kMouseButtonCount];
    
    struct
    {
        short handled;
    } touches[INPUT_TOUCHES_MAX];
    
} InputInfo;

#endif
