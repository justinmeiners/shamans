

#include <stdio.h>
#include <OpenGL/gl3.h>

#include "SDL2/SDL.h"
#include "engine.h"
#include "gl_3.h"
#include "snd_driver_core_audio.h"
#include "snd_driver_null.h"
#include "ai.h"
#include "human.h"
#include "utils.h"

int g_windowWidth = 800;
int g_windowHeight = 600;


int main(int argc, const char * argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        printf("failed to init SDL\n");
        return 1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

    if (!BUILD_DEBUG)
    {
        SDL_ShowCursor(SDL_DISABLE);
    }
    
    SDL_Window* window = NULL;
    SDL_GLContext* context = NULL;
    
    window = SDL_CreateWindow("Master",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              g_windowWidth,
                              g_windowHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    /* Create our opengl context and attach it to our window */
    context = SDL_GL_CreateContext(window);
    
    if (!window || !context)
    {
        printf("failed to create window and context\n");
        return 1;
    }
    
    SDL_GL_SetSwapInterval(1);
    
    glViewport(0, 0, g_windowWidth, g_windowHeight);

    Path_InitGamePath("data/");
    
    
    Renderer gl;
    Gl2_Init(&gl);
    
    SndDriver* driver = NULL;
    
    if (BUILD_SOUND)
    {
        driver = SndDriver_CoreAudio_Create(44100);
    }
    else
    {
        driver = SndDriver_Null_Create();
    }
    
    EngineSettings engineSettings;
    engineSettings.levelPath = "/levels/storage_4/storage_4.lvl";
    engineSettings.inputConfig = kInputConfigMouseKeyboard;
    engineSettings.guiWidth = 1024;
    engineSettings.guiHeight = 768;
    engineSettings.renderWidth = g_windowWidth;
    engineSettings.renderHeight = g_windowHeight;
    engineSettings.renderScaleFactor = 1.0f;
    
    Engine* engine = GetEngine();
    
    if (!Engine_Init(engine, &gl, driver,engineSettings))
    {
        return 2;
    }
    
    Player ai;
    Ai_Init(&ai, engine);
    
    Player player;
    Human_Init(&player, engine);
    
    Engine_JoinPlayer(engine, 1, &player);
    Engine_JoinPlayer(engine, 0, &ai);
    
    InputState inputState;
    InputState_Init(&inputState);

    int quit = 0;

    while (!quit)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        
        inputState.mouseCursor.x = (x / (float)engine->renderSystem.viewportWidth) * 2.0f - 1.0f;
        inputState.mouseCursor.y = -((y / (float)engine->renderSystem.viewportHeight) * 2.0f - 1.0f);
        
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_KEYUP:
                {
                    if (e.key.keysym.sym == SDLK_q)
                    {
                        quit = 1;
                    }
                    if (e.key.keysym.sym == SDLK_p)
                    {
                        if (engine->paused)
                        {
                            Engine_Resume(engine);
                        }
                        else
                        {
                            Engine_Pause(engine);
                        }
                    }
                }
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {inputState.mouseBtns[kMouseBtnLeft].down = 1; }
                    if (e.button.button == SDL_BUTTON_RIGHT) {inputState.mouseBtns[kMouseBtnRight].down = 1; }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {inputState.mouseBtns[kMouseBtnLeft].down = 0; }
                    if (e.button.button == SDL_BUTTON_RIGHT) {inputState.mouseBtns[kMouseBtnRight].down = 0; }
                    break;
            }
        }
        
        EngineState state =  Engine_Tick(engine, &inputState);
        
        if (state == kEngineStateQuit)
        {
            quit = 1;
        }
        else
        {
            Engine_Render(engine);
            SDL_GL_SwapWindow(window);
        }
    }
    
    Engine_Shutdown(engine);
    SDL_GL_DeleteContext(context);
    
    return 0;
}

