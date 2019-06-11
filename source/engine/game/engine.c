

#include "engine.h"
#include "platform.h"
#include "human.h"

#include <assert.h>
#include <time.h>

Engine g_engine;

extern const WeaponInfo g_engineWeaponTable[];
extern const SpawnTable g_engineSpawnTable[];
extern const LevelLoadTable g_engineLevelLoadTable[];

extern void Engine_LevelLoadPath(Engine* engine, const char* path);
extern void Engine_UnloadLevel();
extern void Engine_LoadAssets(Engine* engine);
extern void Engine_UnloadAssets(Engine* engine);

static void Engine_BuildFogView(const Engine* engine, int playerId, FogView* view)
{
    int observers[SCENE_SYSTEM_UNITS_MAX];
    short observerCount = 0;
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = engine->sceneSystem.units + i;
        if (unit->dead) continue;
        
        if (unit->playerId == playerId)
        {
            observers[observerCount] = unit->index;
            ++observerCount;
        }
    }
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = engine->sceneSystem.units + i;
        if (unit->dead) continue;
        
        if (unit->playerId != playerId)
        {
            float visibilty = 0.0f;
            
            for (int j = 0; j < observerCount; ++j)
            {
                const Unit* observer = engine->sceneSystem.units + observers[j];
                
                Vec3 vec = Vec3_Sub(unit->position, observer->position);
                float distSq = Vec3_Dot(vec, vec);
                float rSq = (observer->viewRadius * observer->viewRadius);
                
                float power = (rSq) / (distSq * 1.8f);
                visibilty += power * power;
            }
            
            view->unitVisibility[i] = MIN(visibilty, 1.0f);
        }
        else
        {
            view->unitVisibility[i] = 1.0f;
        }
    }
    
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        const Prop* prop = engine->sceneSystem.props + i;
        if (prop->dead || prop->inactive) continue;
        
        float visibilty = 0.0f;
        
        for (int j = 0; j < observerCount; ++j)
        {
            const Unit* observer = engine->sceneSystem.units + observers[j];
            
            Vec3 vec = Vec3_Sub(prop->position, observer->position);
            float distSq = Vec3_Dot(vec, vec);
            float rSq = (observer->viewRadius * observer->viewRadius);
            
            float power = (rSq) / (distSq * 1.8f);
            visibilty += power * power;
        }
        
        view->propVisibility[i] = MIN(visibilty, 1.0f);
    }
}

static void Engine_CheckEndGame(Engine* engine)
{
    int localUnitCount = 0;
    int otherCount = 0;
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = engine->sceneSystem.units + i;
        if (unit->dead) continue;
        
        if (unit->playerId == ENGINE_PLAYER_LOCAL)
        {
            localUnitCount++;
        }
        else
        {
            otherCount++;;
        }
    }
    
    engine->players[ENGINE_PLAYER_LOCAL]->unitCount = localUnitCount;
    engine->players[ENGINE_PLAYER_AI]->unitCount = otherCount;
    
    if (localUnitCount < 1)
    {
        engine->result = kEngineResultDefeat;
    }
    
    int skullsRemaining = engine->sceneSystem.skullCount - engine->players[ENGINE_PLAYER_LOCAL]->skulls;
    
    if (skullsRemaining < 1)
        engine->result = kEngineResultVictory;
}


static void Engine_EndTurn(Engine* engine)
{
    Engine_CheckEndGame(engine);
    
    if (engine->turn != -1)
    {
        Player* oldPlayer = engine->players[engine->turn];
        
        if (oldPlayer != NULL)
        {
            PlayerEvent event;
            event.type = kPlayerEventEndTurn;
            Player_Event(oldPlayer, &event);
            
            oldPlayer->ourTurn = 0;
        }
    }
        
    int newTurn = (engine->turn + 1) % ENGINE_PLAYER_COUNT;
    
    /* find an available player */
    while (engine->players[newTurn] == NULL)
    {
        newTurn = (newTurn + 1) % ENGINE_PLAYER_COUNT;
    }
    
    engine->turn = newTurn;

    Player* newPlayer = engine->players[engine->turn];
    if (newPlayer != NULL)
    {
        newPlayer->ourTurn = 1;
        
        for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        {
            Unit* unit = engine->sceneSystem.units + i;
            
            if (unit->playerId == engine->turn)
            {
                int newMoveCounter = unit->moveRange;
                
                
                if (unit->powerups & kUnitPowerupRange)
                    newMoveCounter = (unit->moveRange * 7) / 4; // 1.75 *;

                unit->moveCounter = MIN(newMoveCounter, UNIT_RANGE_MAX);
                
                unit->actionCounter = 1;
                
                if (unit->onStartTurn)
                    unit->onStartTurn(unit);
            }
        }

        PlayerEvent event;
        event.type = kPlayerEventStartTurn;
        Player_Event(newPlayer, &event);
    }
    
    {
        for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
        {
            Prop* prop = engine->sceneSystem.props + i;
            if (prop->dead) continue;
            
            Prop_InputEvent(prop, NULL, kEventStartTurn, engine->turn);
        }
    }
}

int Engine_Init(Engine* engine,
                Renderer* renderer,
                SndDriver* sndDriver,
                EngineSettings engineSettings,
                Player* localPlayer,
                Player* aiPlayer)
{
    assert(renderer);
    assert(sndDriver);
    
    if (!renderer) return 0;

    renderer->debug = BUILD_DEBUG;
    
    Filepath_SetDirectory(kDirectoryData, engineSettings.dataPath);
    
    SndSystem_Init(&engine->soundSystem, sndDriver);
    NavSystem_Init(&engine->navSystem);
    PartSystem_Init(&engine->partSystem);
    GuiSystem_Init(&engine->guiSystem, engineSettings.guiWidth, engineSettings.guiHeight);
    InputSystem_Init(&engine->inputSystem, engineSettings.inputConfig);
    SceneSystem_Init(&engine->sceneSystem, engine, g_engineSpawnTable);
    RenderSystem_Init(&engine->renderSystem, engine, renderer, engineSettings.renderWidth, engineSettings.renderHeight);

    engine->renderSystem.scaleFactor = engineSettings.renderScaleFactor;
    
    memset(engine->players, 0, sizeof(engine->players));
    srand((int)time(NULL));
    
    engine->state = kEngineStateInit;
    engine->result = kEngineResultNone;
    
    engine->weaponTable = g_engineWeaponTable;
    engine->levelLoadTable = g_engineLevelLoadTable;
    
    engine->paused = 0;
    engine->turn = 1;
    
    engine->renderSystem.renderer->beginLoading(engine->renderSystem.renderer);
    Engine_LoadAssets(engine);
    Engine_LevelLoadPath(engine, engineSettings.levelPath);
    engine->renderSystem.renderer->endLoading(engine->renderSystem.renderer);

    
    SndSystem_SetAmbient(&engine->soundSystem, SND_AMBIENT);

    // prepare players
    localPlayer->engine = engine;
    aiPlayer->engine = engine;
    engine->players[ENGINE_PLAYER_LOCAL] = localPlayer;
    engine->players[ENGINE_PLAYER_AI] = aiPlayer;
    engine->players[ENGINE_PLAYER_LOCAL]->playerId = ENGINE_PLAYER_LOCAL;
    engine->players[ENGINE_PLAYER_AI]->playerId = ENGINE_PLAYER_AI;
    
    PlayerEvent event;
    event.type = kPlayerEventJoin;
    Player_Event(localPlayer, &event);
    Player_Event(aiPlayer, &event);

    return 1;
}


void Engine_Shutdown(Engine* engine)
{
    engine->paused = 1;
    
    SndSystem_Shutdown(&engine->soundSystem);
    Engine_UnloadLevel(engine);
    Engine_UnloadAssets(engine);
    SceneSystem_Clear(&engine->sceneSystem);

    RenderSystem_Shutdown(&engine->renderSystem, engine);
    GuiSystem_Shutdown(&engine->guiSystem);
}

void Engine_End(Engine* engine)
{
    engine->state = kEngineStateEnd;
    engine->paused = 1;
}

void Engine_Pause(Engine* engine)
{
    engine->paused = 1;
}

void Engine_Resume(Engine* engine)
{
    engine->paused = 0;
}

void Engine_RunCommand(Engine* engine, const Command* command)
{
    //_Engine_TransmitCommand(engine, command);
    
    engine->state = kEngineStateCommand;
    
    Player* controller = engine->players[engine->turn];
    
    PlayerEvent event;
    event.command = command;
    event.type = kPlayerEventStartCommand;
    Player_Event(controller, &event);

    engine->command = *command;
    controller->previousCommand = *command;
    
    switch (command->type)
    {
        case kCommandTypeEndTurn:
        {
            Engine_EndTurn(engine);
            break;
        }
        case kCommandTypeMove:
        {
            // find unit at index to avoid const
            Unit* unit = engine->sceneSystem.units + command->unit->index;
            
            if (command->target != NULL)
            {
                unit->target = engine->sceneSystem.units + command->target->index;
            }
            else
            {
                unit->target = NULL;
            }
            
            Unit_StartPath(unit, command->position);
            break;
        }
        case kCommandTypeAttackPrimary:
        {
            // find unit at index to avoid const
            Unit* unit = engine->sceneSystem.units + command->unit->index;
            unit->state = kUnitStateAttackPrimary;
            break;
        }
        case kCommandTypeAttackSecondary:
        {
            // find unit at index to avoid const
            Unit* unit = engine->sceneSystem.units + command->unit->index;
            unit->state = kUnitStateAttackSecondary;
            break;
        }
        case kCommandTypeTeleport:
        {
            // find unit at index to avoid const
            Unit* unit = engine->sceneSystem.units + command->unit->index;
            unit->state = kUnitStateTeleport;
            unit->previousTeleportHp = unit->hp;
            break;
        }
        default: break;
    }
}

Player* Engine_LocalPlayer(Engine* engine)
{
    return engine->players[ENGINE_PLAYER_LOCAL];
}


static void Engine_TickFrustum(Engine* engine)
{
    float viewportWidth = (float)engine->renderSystem.viewportWidth;
    float viewportHeight = (float)engine->renderSystem.viewportHeight;
    
    Vec3 target = engine->renderSystem.cam.target;
    
    Vec3 pos = Vec3_Add(target, engine->renderSystem.camOffset);
    
    Vec3 dir = Vec3_Norm(Vec3_Sub(target, pos));
    Ray3 viewRay = Ray3_Create(engine->renderSystem.cam.position, dir);
    
    /* if we don't intersect with a poly, then the camera will continue to use the last z
     this prevents the camera from abruptly falling of edges of nav mesh */
    
    NavRaycastResult hitInfo;
    if (NavSystem_Raycast(&engine->navSystem, viewRay, &hitInfo))
    {
        target.z = Interp_Lerp(0.1, target.z, hitInfo.point.z);
    }
    
    pos.z = target.z + engine->renderSystem.camOffset.z;
    
    engine->renderSystem.cam.position = pos;
    engine->renderSystem.cam.target = target;

    Frustum_UpdateTransform(&engine->renderSystem.cam, viewportWidth, viewportHeight);
}

static void Engine_TickUnits(Engine* engine)
{
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        Unit* unit = engine->sceneSystem.units + i;
        if (unit->dead) continue;
    
        // alerted for AI
        if (!unit->isAlerted)
        {
            for (int j = 0; j < SCENE_SYSTEM_UNITS_MAX; ++j)
            {
                const Unit* other = engine->sceneSystem.units + j;
                if (other->dead || other->playerId == unit->playerId) continue;
                
                if (Unit_CanSee(unit, other, unit->viewRadius * 0.65f))
                    unit->isAlerted = 1;
            }
        }
    
        if (unit->onTick)
            unit->onTick(unit);
        
        unit->skelModel.skel.rotation = Quat_CreateAngle(unit->angle, 0.0f, 0.0f, 1.0f);
        
        float radians = DEG_TO_RAD(unit->angle);
        unit->forward = Vec3_Create(cosf(radians), sinf(radians), 0.0f);
        
        // touching
        unit->actionProp = NULL;
        
        for (int j = 0; j < SCENE_SYSTEM_PROPS_MAX; ++j)
        {
            Prop* prop = engine->sceneSystem.props + j;
            if (prop->dead || prop->inactive) continue;
            
            if (AABB_IntersectsAABB(unit->bounds, prop->bounds))
            {
                if (prop->onTouchUnit)
                    prop->onTouchUnit(prop, unit);
                
                if (prop->onAction)
                    unit->actionProp = prop;
            }
        }
        
        
        // Raycast from unit loctation to determine which nav polygon the unit is on
        Ray3 ray = Ray3_Create(Vec3_Add(unit->position, Vec3_Create(0.0f, 0.0f, 2.0f)), Vec3_Create(0.0f, 0.0f, -1.0f));
        
        NavRaycastResult hitInfo;
        if (NavSystem_Raycast(&engine->navSystem, ray, &hitInfo))
        {
            unit->position.z = hitInfo.point.z;
            unit->navPoly = hitInfo.poly;
        }
        else
        {
            unit->navPoly = NULL;
        }
        
        // draw selection ring
        if (unit->selected)
        {
            Vec3 start = Vec3_Offset(unit->position, 0.0f, 0.0f, .1f);
            HintBuffer_PackCircle(&engine->renderSystem.hintBuffer, unit->radius, start, Vec3_Create(0.0f, 1.0f, 0.0f), 32);
            
            // draw attack ring
            if (unit->state == kUnitStatePreparePrimary || unit->state == kUnitStatePrepareSecondary)
            {
                const WeaponInfo* weapon = engine->weaponTable + unit->primaryWeapon;
                
                if (unit->state == kUnitStatePrepareSecondary)
                {
                    weapon = engine->weaponTable + unit->secondaryWeapon;
                }
                
                Vec3 start = Vec3_Offset(unit->position, 0.0f, 0.0f, .1f);
                HintBuffer_PackCircle(&engine->renderSystem.hintBuffer, weapon->range, start, Vec3_Create(1.0f, 1.0f, 0.0f), 36);
            }
            
            if (BUILD_DEBUG)
            {
                HintBuffer_PackPath(&engine->renderSystem.hintBuffer, &unit->path, Vec3_Create(1.0f, 0.25f, 0.25f));
            }
        }
        
        if (BUILD_DEBUG)
        {
            Hintbuffer_PackSkel(&engine->renderSystem.hintBuffer, &unit->skelModel.skel, unit->position, Vec3_Create(1.0f, 1.0f, 0.0f),  Vec3_Create(1.0f, 0.0f, 0.0f));
            HintBuffer_PackAABB(&engine->renderSystem.hintBuffer, unit->bounds, Vec3_Create(0.5f, 0.5f, 0.5f));
        }
    }
}

static void Engine_TickProps(Engine* engine)
{
    for (int i = 0; i < SCENE_SYSTEM_PROPS_MAX; ++i)
    {
        Prop* prop = engine->sceneSystem.props + i;
        if (prop->dead || prop->inactive) continue;
        
        prop->forward = Quat_MultVec3(&prop->rotation, Vec3_Create(1.0f, 0.0f, 0.0f));

        if (prop->onTick)
            prop->onTick(prop);
        
        Mat4 rot;
        Mat4 translate = Mat4_CreateTranslate(prop->position);
        Quat_ToMatrix(prop->rotation, &rot);
        Mat4_Mult(&translate, &rot, &prop->worldMatrix);
        
        for (int j = i + 1; j < SCENE_SYSTEM_PROPS_MAX; ++j)
        {
            Prop* other = engine->sceneSystem.props + j;
            if (other->dead || other->inactive) continue;
            
            if (AABB_IntersectsAABB(prop->bounds, other->bounds))
            {
                if (other->touchEnabled && prop->onTouch)
                    prop->onTouch(prop, other);
                
                if (prop->touchEnabled && other->onTouch)
                    other->onTouch(other, prop);
            }
        }
        
        if (BUILD_DEBUG)
        {
            HintBuffer_PackAABB(&engine->renderSystem.hintBuffer, prop->bounds, Vec3_Create(0.8f, 0.8f, 0.8f));
        }
    }
}

static void Engine_TickPlayers(Engine* engine)
{
    Player* controller = engine->players[engine->turn];
    
    if (engine->state == kEngineStateCommand)
    {
        switch (engine->command.type)
        {
            case kCommandTypeMove:
            {
                const Unit* unit = engine->command.unit;
                
                if (unit->state == kUnitStateIdle)
                    engine->state = kEngineStateAwait;

                break;
            }
            case kCommandTypeAttackSecondary:
            case kCommandTypeAttackPrimary:
            case kCommandTypeTeleport:
            {
                const Unit* unit = engine->command.unit;
                
                if (unit->state == kUnitStateIdle)
                {
                    engine->state = kEngineStateAwait;
                }
                break;
            }
            case kCommandTypeEndTurn:
            {
                engine->state = kEngineStateAwait;
                break;
            }
            default: break;
        }
    }
    else if (engine->state == kEngineStateAwait)
    {
        /* wait for command to be acknowledged */
        engine->state = kEngineStateIdle;
        
        PlayerEvent event;
        event.type = kPlayerEventEndCommand;
        event.command = &engine->command;
        
        Player_Event(controller, &event);
    }
    
    if (!engine->paused)
    {
        Engine_TickUnits(engine);
        Engine_TickProps(engine);
    }
}

static void Engine_RecieveMouseKeyboard(Engine* engine, const InputState* state, const InputState* last, InputInfo* info)
{
    if (state->mouseButtons[kMouseButtonLeft].phase == kInputPhaseBegin && !info->mouseButtons[kMouseButtonLeft].handled)
    {
        Ray3 mouseRay;
        mouseRay.origin = engine->renderSystem.cam.position;
        
        Vec3 p2 = Frustum_Unproject(&engine->renderSystem.cam, Vec3_Create(state->mouseCursor.x, state->mouseCursor.y, 0.999f));
        mouseRay.dir = Vec3_Norm(Vec3_Sub(p2, engine->renderSystem.cam.position));
        
        PlayerEvent event;
        event.type = kPlayerEventTap;
        event.ray = mouseRay;
        
        NavRaycastResult hitInfo;
        
        if (NavSystem_Raycast(&engine->navSystem, mouseRay, &hitInfo))
        {
            event.navPoly = hitInfo.poly;
            event.intersection = hitInfo.point;
        }
        else
        {
            event.navPoly = NULL;
            event.intersection = Vec3_Zero;
        }

        Player* controller = engine->players[engine->turn];
        Player_Event(controller, &event);
        
        info->mouseButtons[kMouseButtonLeft].handled = 1;
    }
    else if (state->mouseButtons[kMouseButtonLeft].down == 0)
    {
        /* scrolling around with the mouse */

        Vec3 target = engine->renderSystem.cam.target;
        
        Vec2 sp = Vec2_Create(state->mouseCursor.x, state->mouseCursor.y);
        
        float threshold = 0.9f;
        float length = 1.0f - threshold;
        
        if (sp.x > threshold) {
            target.x += (sp.x - threshold) / length;
        }
        else if (sp.x < -threshold)
        {
            target.x += (sp.x + threshold) / length;
        }
        
        if (sp.y > threshold) {
            target.y += (sp.y - threshold) / length;
        }
        else if (sp.y < -threshold)
        {
            target.y += (sp.y + threshold) / length;
        }
        
        engine->renderSystem.cam.target = target;
    }
}

static void Engine_RecieveMultitouch(Engine* engine, const InputState* state, const InputState* last, InputInfo* info)
{
    InputSystem* system = &engine->inputSystem;
    
    int activeTouches = 0;
    
    for (int i = 0; i < state->touchCount; ++i)
    {
        if (state->touches[i].phase != kInputPhaseNone)
            ++activeTouches;
    }
    
    if (activeTouches > 1)
    {
        if (!system->panning)
        {
            system->panIdentifier1 = state->touches[0].identifier;
            system->panIdentifier2 = state->touches[1].identifier;
            
            info->touches[0].handled = 1;
            info->touches[1].handled = 1;
            system->panning = 1;
        }
        
        if (state->touchCount > 1 && last->touchCount > 1  &&
            state->touches[0].identifier == last->touches[0].identifier &&
            state->touches[1].identifier == last->touches[1].identifier &&
            state->touches[0].identifier == system->panIdentifier1 &&
            state->touches[1].identifier == system->panIdentifier2)
        {
            Vec2 avg1 = Vec2_Create(state->touches[0].x, state->touches[0].y);
            avg1 = Vec2_Add(avg1, Vec2_Create(state->touches[1].x, state->touches[1].y));
            avg1 = Vec2_Scale(avg1, 0.5f);
            
            Vec2 avg2 = Vec2_Create(last->touches[0].x, last->touches[0].y);
            avg2 = Vec2_Add(avg2, Vec2_Create(last->touches[1].x, last->touches[1].y));
            avg2 = Vec2_Scale(avg2, 0.5f);
            
            Vec2 diff = Vec2_Scale(Vec2_Sub(avg1, avg2), 40.0f);
            
            engine->renderSystem.cam.target = Vec3_Offset(engine->renderSystem.cam.target, -diff.x, -diff.y, 0.0f);
        }
    }
    else if (activeTouches == 1)
    {
        system->panning = 0;
        
        for (int i = 0; i < state->touchCount; ++i)
        {
            if (!info->touches[i].handled && state->touches[i].phase == kInputPhaseEnd)
            {
                PlayerEvent event;
                event.type = kPlayerEventTap;
                
                Vec3 p2 = Frustum_Unproject(&engine->renderSystem.cam, Vec3_Create(state->touches[i].x, state->touches[i].y, 0.999f));
                Vec3 dir = Vec3_Norm(Vec3_Sub(p2, engine->renderSystem.cam.position));
                
                event.ray = Ray3_Create(engine->renderSystem.cam.position, dir);
                
                NavRaycastResult hitInfo;
                if (NavSystem_Raycast(&engine->navSystem, event.ray, &hitInfo))
                {
                    event.intersection = hitInfo.point;
                    event.navPoly = hitInfo.poly;
                }
                else
                {
                    event.intersection = Vec3_Zero;
                    event.navPoly = NULL;
                }

                Player* controller = engine->players[engine->turn];

                Player_Event(controller, &event);
                break;
            }
        }
    }
    else
    {
        system->panning = 0;
    }
}

EngineState Engine_Tick(Engine* engine, const InputState* newState)
{
    InputSystem_ProcessInput(&engine->inputSystem, newState);
    
    const InputState* currentInput = &engine->inputSystem.current;
    const InputState* lastInput = &engine->inputSystem.last;
    
    GuiSystem_ReceiveInput(&engine->guiSystem, currentInput, &engine->inputSystem.last, &engine->inputSystem.info);
    
    if (!engine->paused)
    {
        if (engine->inputSystem.config == kInputConfigMouseKeyboard)
        {
            Engine_RecieveMouseKeyboard(engine, currentInput, lastInput, &engine->inputSystem.info);
        }
        else
        {
            Engine_RecieveMultitouch(engine, currentInput, lastInput, &engine->inputSystem.info);
        }
        
        Engine_TickFrustum(engine);
        
        HintBuffer_Clear(&engine->renderSystem.hintBuffer);
        
        if (BUILD_DEBUG)
        {
            HintBuffer_PackNav(&engine->renderSystem.hintBuffer, &engine->navSystem.navMesh, Vec3_Create(0.0f, 0.0f, 1.0f));
        }
        
        PartSystem_Tick(&engine->partSystem);
        
        Engine_TickPlayers(engine);
        
        if (BUILD_DEBUG)
        {
            for (int i = 0; i < engine->sceneSystem.lightCount; ++i)
            {
                const Light* light = engine->sceneSystem.lights + i;
                HintBuffer_PackLine(&engine->renderSystem.hintBuffer, light->point, Vec3_Add(light->point, light->forward), Vec3_Create(1.0f, 1.0f, 0.0f));
            }
        }
        
    }
    
    GuiSystem_Tick(&engine->guiSystem);
    
    if (engine->state == kEngineStateInit)
    {
        Engine_TickUnits(engine);
        Engine_TickProps(engine);
        
        engine->paused = 0;
        engine->turn = -1;
        Engine_EndTurn(engine);
        engine->state = kEngineStateIdle;
    }
    
    Engine_BuildFogView(engine, ENGINE_PLAYER_LOCAL, &engine->fogView);
    
    return engine->state;
}

void Engine_Render(Engine* engine)
{
    RenderSystem_Render(&engine->renderSystem, &engine->renderSystem.cam, engine);
}




