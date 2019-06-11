
#include "human.h"

typedef enum
{
    kHumanStateIdle = 0,
    kHumanStateCommand,
    kHumanStatePreparePrimary,
    kHumanStatePrepareSecondary,
} HumanState;

enum
{
    FADE_NONE,
    FADE_IN,
    FADE_OUT,
};

static const int FADE_TICKS = 80;

typedef struct
{
    GuiView* cursor;
    
    GuiView* rangeBar;
    GuiView* hpBar;
    GuiView* nameGuiLabel;
    
    GuiView* menu;
    GuiView* dimmer;
    GuiView* controls;
    
    GuiView* deselectButton;
    
    GuiView* primaryButton;
    GuiView* secondaryButton;

    GuiView* endTurnButton;
    GuiView* actionButton;
    
    GuiView* rangePowerupMarker;
    GuiView* defensePowerupMarker;
    
    GuiView* skullLabel;
    
    Unit* previousTurnSelection;
    
    int fadeMode;
    int fadeTimer;
    
    int lastViewedPlayer;
    
} HumanContext;

enum
{
    LAYER_CURSOR,
    LAYER_END_TURN,
    LAYER_MENU_DIMMER,
    LAYER_MENU,
    LAYER_MENU_BTN,
    LAYER_MENU_RESUME,
    LAYER_MENU_CONTROLS,
    LAYER_MENU_QUIT,
    
    LAYER_CONTROLS,
    
    LAYER_ATTACK_PRIMARY,
    LAYER_ATTACK_SECONDARY,
    LAYER_UNIT_NAME,
    LAYER_RECENTER,
    LAYER_DESELECT,
    
    LAYER_ACTION,
};

static void Human_CenterOnNextUnit(Player* controller)
{
    HumanContext* ctx = controller->userInfo;
    Engine* engine = controller->engine;

    const Unit* lastViewed = engine->sceneSystem.units + ctx->lastViewedPlayer;
    
    int startIndex = lastViewed->index + 1;

    if (lastViewed->dead || lastViewed->playerId != controller->playerId)
    {
        startIndex = 0;
    }

    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        int index = (startIndex + i) % SCENE_SYSTEM_UNITS_MAX;
        
        const Unit* unit = engine->sceneSystem.units + index;
        if (!unit->dead && unit->playerId == controller->playerId)
        {
            engine->renderSystem.cam.target = unit->position;
            ctx->lastViewedPlayer = unit->index;
            break;
        }
    }
}


static void Human_OnGuiEvent(GuiSystem* system, const GuiEvent* event, void* delegate)
{
    Player* controller = delegate;
    Engine* engine = controller->engine;
    HumanContext* ctx = controller->userInfo;

    if (event->type == kGuiEventGuiViewTap)
    {
        switch (event->viewTag)
        {
            case LAYER_MENU_RESUME:
            case LAYER_MENU_BTN:
            {
                if (ctx->menu->hidden)
                {
                    Engine_Pause(engine);
                }
                else
                {
                    Engine_Resume(engine);
                }
                
                ctx->menu->hidden = !ctx->menu->hidden;
                ctx->dimmer->hidden = ctx->menu->hidden;
                GuiView_SetColor(ctx->dimmer, Vec4_Create(0.0f, 0.0f, 0.0f, 0.5f), kGuiViewStateNormal);
                ctx->fadeMode = FADE_NONE;
                break;
            }
            case LAYER_MENU_QUIT:
            {
                engine->result = kEngineResultQuit;
                Engine_End(engine);
                break;
            }
            case LAYER_MENU_CONTROLS:
            {
                ctx->menu->hidden = 1;
                ctx->controls->hidden = 0;
                break;
            }
            case LAYER_CONTROLS:
            {
                ctx->menu->hidden = 0;
                ctx->controls->hidden = 1;
                break;
            }
            case LAYER_END_TURN:
            {
                if (engine->state == kEngineStateIdle)
                {
                    int unitsDone = 1;
                    
                    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
                    {
                        const Unit* unit = engine->sceneSystem.units + i;
                        if (unit->dead) { continue; }
                        
                        if (unit->playerId == controller->playerId && unit->state != kUnitStateIdle)
                        {
                            unitsDone = 0;
                            break;
                        }
                    }
                    
                    if (unitsDone)
                    {
                        Command command;
                        command.playerId = controller->playerId;
                        command.type = kCommandTypeEndTurn;
                        Engine_RunCommand(engine, &command);
                    }

                }
                break;
            }
            case LAYER_RECENTER:
            {
                if (controller->selection != NULL)
                {
                    engine->renderSystem.cam.target = controller->selection->position;
                }
                else
                {
                    Human_CenterOnNextUnit(controller);
                }
                
                break;
            }
            case LAYER_DESELECT:
            {
                if (controller->selection->state == kUnitStateMove)
                {
                    controller->state = kHumanStateIdle;
                    controller->selection->state = kUnitStateIdle;
                }
                else
                {
                    controller->state = kHumanStateIdle;
                    controller->selection->state = kUnitStateIdle;
                    Player_ClearSelection(controller);
                }
               
                break;
            }
            case LAYER_ATTACK_PRIMARY:
            {
                if (controller->state == kHumanStateIdle ||
                    controller->state == kHumanStatePrepareSecondary)
                {
                    if (controller->selection != NULL)
                    {
                        controller->state = kHumanStatePreparePrimary;
                        controller->selection->state = kUnitStatePreparePrimary;
                    }
                }
                else if (controller->state == kHumanStatePreparePrimary)
                {
                    if (controller->selection != NULL)
                    {
                        controller->state = kHumanStateIdle;
                        controller->selection->state = kUnitStateIdle;
                    }
                }

                break;
            }
            case LAYER_ATTACK_SECONDARY:
            {
                if (controller->state == kHumanStateIdle ||
                    controller->state == kHumanStatePreparePrimary)
                {
                    if (controller->selection != NULL)
                    {
                        controller->state = kHumanStatePrepareSecondary;
                        controller->selection->state = kUnitStatePrepareSecondary;
                    }
                }
                else if (controller->state == kHumanStatePrepareSecondary)
                {
                    if (controller->selection != NULL)
                    {
                        controller->state = kHumanStateIdle;
                        controller->selection->state = kUnitStateIdle;
                    }
                }
                break;
            }
            case LAYER_ACTION:
            {
                if (controller->state == kHumanStateIdle && controller->selection != NULL)
                {
                    Prop* actionProp = controller->selection->actionProp;
                    
                    if (actionProp)
                    {
                        actionProp->onAction(actionProp, controller->selection);
                    }
                }
                break;
            }
        }
    }
}

static void Human_OnGuiTick(GuiSystem* system, void* delegate)
{
    const Player* controller = delegate;
    Engine* engine = controller->engine;
    HumanContext* ctx = controller->userInfo;
    
    Vec2 mp = Vec2_Create(engine->inputSystem.current.mouseCursor.x, engine->inputSystem.current.mouseCursor.y);
    Vec2 pp = GuiSystem_ProjectNormalized(&engine->guiSystem, mp);
    
    ctx->cursor->origin = Vec2_Offset(pp, 0.0f, -32.0f);
    
    GuiView* rangeBar = ctx->rangeBar;
    GuiView* hpBar = ctx->hpBar;
    GuiView* nameGuiLabel = ctx->nameGuiLabel;
    GuiView* attackPrimaryBtn = ctx->primaryButton;
    GuiView* attackSecondaryBtn = ctx->secondaryButton;
    GuiView* endTurnBtn = ctx->endTurnButton;
    GuiView* dimmer = ctx->dimmer;
    GuiView* deselectBtn = ctx->deselectButton;
    GuiView* skullBtn = ctx->skullLabel;

    GuiView* rangePowerupMarker = ctx->rangePowerupMarker;
    GuiView* defensePowerupMarker = ctx->defensePowerupMarker;
    
    if (ctx->fadeMode != FADE_NONE)
    {
        if (--ctx->fadeTimer < 1)
        {
            ctx->dimmer->hidden = 1;
            
            if (ctx->fadeMode == FADE_OUT)
            {
                Engine_End(engine);
            }
            
            ctx->fadeMode = FADE_NONE;
        }
        else
        {
            ctx->dimmer->hidden = 0;
        }
        
        float percent = ctx->fadeTimer / (float)FADE_TICKS;
        percent = (ctx->fadeMode == FADE_IN) ? percent : 1.0f - percent;
        
        GuiView_SetColor(dimmer, Vec4_Create(0.0f, 0.0f, 0.0f, percent), kGuiViewStateNormal);
    }
    
    int skullsRemaining = engine->sceneSystem.skullCount - controller->skulls;
    skullBtn->label.textLength = snprintf(skullBtn->label.text, GUI_LABEL_TEXT_MAX, "%i", skullsRemaining);

    GuiView* actionButton = ctx->actionButton;
    
    endTurnBtn->hidden = controller->state == kHumanStatePreparePrimary ||
                            controller->state == kHumanStatePrepareSecondary ||
                            engine->state == kEngineStateCommand ||
                                        !controller->ourTurn;

    if (controller->selection != NULL)
    {
        const Unit* unit = controller->selection;
        
        rangeBar->size.x = 270 * (unit->moveCounter / UNIT_RANGE_MAX);
        hpBar->size.x = 270 * (unit->hp / UNIT_HP_MAX);
        
        rangeBar->hidden = 0;
        nameGuiLabel->hidden = 0;
        hpBar->hidden = 0;
        
        attackPrimaryBtn->hidden = engine->state == kEngineStateCommand;
        attackSecondaryBtn->hidden = engine->state == kEngineStateCommand;
        
        deselectBtn->hidden = engine->state == kEngineStateCommand && controller->selection->state != kUnitStateMove;
        
        if (unit->actionCounter < 1 || unit->primaryWeapon == -1)
        {
            attackPrimaryBtn->state = kGuiViewStateDisabled;
        }
        else
        {
            attackPrimaryBtn->state = controller->state == kHumanStatePreparePrimary ? kGuiViewStateSelected : kGuiViewStateNormal;
        }
        
        if (unit->actionCounter < 1 || unit->secondaryWeapon == -1)
        {
            attackSecondaryBtn->state = kGuiViewStateDisabled;
        }
        else
        {
            attackSecondaryBtn->state = controller->state == kHumanStatePrepareSecondary ? kGuiViewStateSelected : kGuiViewStateNormal;
        }

        if (unit->actionProp != NULL)
        {
            const Frustum* cam = &engine->renderSystem.cam;
            
            Mat4 mvp;
            Mat4_Mult(Frustum_ProjMatrix(cam), Frustum_ViewMatrix(cam), &mvp);
            
            Vec3 normalized = Mat4_Project(&mvp, Vec3_Add(unit->position, Vec3_Create(0.0f, 0.0f, 2.0f)));
            normalized.x = (normalized.x * 0.5f) + 0.5f;
            normalized.y = (normalized.y * 0.5f) + 0.5f;
            actionButton->origin = Vec2_Create(normalized.x * engine->guiSystem.guiWidth, normalized.y * engine->guiSystem.guiHeight);
            
            actionButton->hidden = 0;
        }
        else
        {
            actionButton->hidden = 1;
        }
        
        rangePowerupMarker->hidden = !(unit->powerups & kUnitPowerupRange);
        defensePowerupMarker->hidden = !(unit->powerups & kUnitPowerupDefense);
    }
    else
    {
        rangeBar->hidden = 1;
        nameGuiLabel->hidden = 1;
        hpBar->hidden = 1;
        
        attackPrimaryBtn->hidden = 1;
        attackSecondaryBtn->hidden = 1;
        actionButton->hidden = 1;
        deselectBtn->hidden = 1;
        
        rangePowerupMarker->hidden = 1;
        defensePowerupMarker->hidden = 1;
    }
}

static void Human_PrepareGui(Player* controller)
{
    Engine* engine = controller->engine;
    HumanContext* ctx = controller->userInfo;
    
    GuiView* statBg = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    statBg->origin = Vec2_Create(0, 0);
    statBg->size = Vec2_Create(360, 140);
    GuiView_SetUvsRotated(statBg, Vec2_Create(0, 1024-720), Vec2_Create(288, 720), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    
    GuiView* hpBar = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    hpBar->origin = Vec2_Create(68, 58);
    hpBar->size = Vec2_Create(270, 13);
    GuiView_SetColor(hpBar, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStateNormal);
    GuiView_SetUvs(hpBar, Vec2_Create(1024-260, 6), Vec2_Create(260, 46), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->hpBar = hpBar;
    
    GuiView* rangeBar = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    rangeBar->origin = Vec2_Create(68, 33);
    rangeBar->size = Vec2_Create(270, 13);
    GuiView_SetColor(rangeBar, Vec4_Create(0.0f, 1.0f, 0.0f, 1.0f), kGuiViewStateNormal);
    GuiView_SetUvs(rangeBar, Vec2_Create(1024-260, 6), Vec2_Create(260, 46), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->rangeBar = rangeBar;
    
    GuiView* nameLabel = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    nameLabel->tag = LAYER_UNIT_NAME;
    nameLabel->origin = Vec2_Create(16, 98);
    nameLabel->label.align = kTextAlignLeft;
    nameLabel->drawViewEnabled = 0;
    ctx->nameGuiLabel = nameLabel;
    
    GuiView* hpLabel = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    hpLabel->origin = Vec2_Create(68, 58);
    hpLabel->label.align = kTextAlignRight;
    hpLabel->drawViewEnabled = 0;
    GuiLabel_SetText(&hpLabel->label, "Hp:");
    
    GuiView* rangeLabel = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    rangeLabel->origin = Vec2_Create(68, 33);
    rangeLabel->label.align = kTextAlignRight;
    rangeLabel->drawViewEnabled = 0;
    GuiLabel_SetText(&rangeLabel->label, "Mv:");
    
    GuiView* rangePowerupMarker = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    rangePowerupMarker->atlas = kGuiAtlasItems;
    rangePowerupMarker->origin = Vec2_Create(272, 90);
    rangePowerupMarker->size = Vec2_Create(38, 38);
    GuiView_SetUvs(rangePowerupMarker, Vec2_Create(1024-76, 1024-76), Vec2_Create(76, 76), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->rangePowerupMarker = rangePowerupMarker;
    
    GuiView* defensePowerupMarker = GuiSystem_SpawnView(&engine->guiSystem, statBg);
    defensePowerupMarker->atlas = kGuiAtlasItems;
    defensePowerupMarker->origin = Vec2_Create(310, 90);
    defensePowerupMarker->size = Vec2_Create(38, 38);
    GuiView_SetUvs(defensePowerupMarker, Vec2_Create(1024-76, 1024-152), Vec2_Create(76, 76), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->defensePowerupMarker = defensePowerupMarker;
    
    GuiView* buttonBg = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    buttonBg->origin = Vec2_Create(360, 0);
    buttonBg->size = Vec2_Create(447, 73);
    GuiView_SetUvsRotated(buttonBg, Vec2_Create(1024 - 146, 1024 - 894), Vec2_Create(146, 894), Vec2_Create(1024, 1024), kGuiViewStateNormal);

    GuiView* endTurnButton = GuiSystem_SpawnView(&engine->guiSystem, buttonBg);
    endTurnButton->origin = Vec2_Create(380, 10);
    endTurnButton->size = Vec2_Create(54, 54);
    endTurnButton->tag = LAYER_END_TURN;
    endTurnButton->pressEnabled = 1;
    GuiView_SetUvs(endTurnButton, Vec2_Create(466, 206), Vec2_Create(108, 108), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    GuiView_SetColor(endTurnButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    ctx->endTurnButton = endTurnButton;
    
    GuiView* menuButton = GuiSystem_SpawnView(&engine->guiSystem, buttonBg);
    menuButton->origin = Vec2_Create(162, 10);
    menuButton->size = Vec2_Create(54, 54);
    menuButton->tag = LAYER_MENU_BTN;
    menuButton->pressEnabled = 1;
    GuiView_SetUvs(menuButton, Vec2_Create(704, 206), Vec2_Create(108, 108), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    GuiView_SetColor(menuButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    
    GuiView* recenterButton = GuiSystem_SpawnView(&engine->guiSystem, buttonBg);
    recenterButton->origin = Vec2_Create(310, 10);
    recenterButton->size = Vec2_Create(54, 54);
    recenterButton->tag = LAYER_RECENTER;
    recenterButton->pressEnabled = 1;
    GuiView_SetUvs(recenterButton, Vec2_Create(592, 206), Vec2_Create(108, 108), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    GuiView_SetColor(recenterButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);

    GuiView* skullView = GuiSystem_SpawnView(&engine->guiSystem, buttonBg);
    skullView->origin = Vec2_Create(0.0f, 8.0f);
    skullView->size = Vec2_Create(54, 54);
    skullView->label.anchor = Vec2_Create(0.9f, 0.4f);
    skullView->label.align = kTextAlignLeft;
    skullView->drawViewEnabled = 1;
    skullView->atlas = kGuiAtlasItems;
    GuiView_SetUvs(skullView, Vec2_Create(1024-108, 0), Vec2_Create(108, 108), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->skullLabel = skullView;
    
    GuiView* itemBg = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    itemBg->origin = Vec2_Create(807, 0);
    itemBg->size = Vec2_Create(216, 143);
    GuiView_SetUvs(itemBg, Vec2_Create(0, 0), Vec2_Create(432, 286), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    
    GuiView* deselectButton = GuiSystem_SpawnView(&engine->guiSystem, buttonBg);
    deselectButton->origin = Vec2_Create(234, 10);
    deselectButton->size = Vec2_Create(54, 54);
    deselectButton->tag = LAYER_DESELECT;
    deselectButton->pressEnabled = 1;
    GuiView_SetUvs(deselectButton, Vec2_Create(704, 98), Vec2_Create(108, 108), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    GuiView_SetColor(deselectButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    ctx->deselectButton = deselectButton;
    
    GuiView* primaryButton = GuiSystem_SpawnView(&engine->guiSystem, itemBg);
    primaryButton->origin = Vec2_Create(0, 0);
    primaryButton->size = Vec2_Create(105, 142);
    primaryButton->tag = LAYER_ATTACK_PRIMARY;
    primaryButton->pressEnabled = 1;
    primaryButton->atlas = kGuiAtlasItems;
    primaryButton->hidden = 1;
    GuiView_SetColor(primaryButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    GuiView_SetColor(primaryButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStateSelected);
    GuiView_SetColor(primaryButton, Vec4_Create(0.5f, 0.5f, 0.5f, 1.0f), kGuiViewStateDisabled);
    ctx->primaryButton = primaryButton;
    
    GuiView* secondaryButton = GuiSystem_SpawnView(&engine->guiSystem, itemBg);
    secondaryButton->origin = Vec2_Create(110, 0);
    secondaryButton->size = Vec2_Create(105, 142);
    secondaryButton->tag = LAYER_ATTACK_SECONDARY;
    secondaryButton->pressEnabled = 1;
    secondaryButton->atlas = kGuiAtlasItems;
    secondaryButton->hidden = 1;
    GuiView_SetColor(secondaryButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    GuiView_SetColor(secondaryButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStateSelected);
    GuiView_SetColor(secondaryButton, Vec4_Create(0.5f, 0.5f, 0.5f, 1.0f), kGuiViewStateDisabled);
    ctx->secondaryButton = secondaryButton;
    
    GuiView* actionButton = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    actionButton->origin = Vec2_Create(0, 0);
    actionButton->size = Vec2_Create(65, 65);
    actionButton->pressEnabled = 1;
    actionButton->tag = LAYER_ACTION;
    GuiView_SetUvs(actionButton, Vec2_Create(458, 60), Vec2_Create(130, 130), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    GuiView_SetColor(actionButton, Vec4_Create(1.0f, 0.0f, 0.0f, 1.0f), kGuiViewStatePress);
    ctx->actionButton = actionButton;

    GuiView* dimmer = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    dimmer->tag = LAYER_MENU_DIMMER;
    dimmer->size = Vec2_Create(1024, 768);
    dimmer->origin = Vec2_Zero;
    dimmer->pressEnabled = 1;
    dimmer->hidden = 0;
    GuiView_SetUvs(dimmer, Vec2_Zero, Vec2_Zero, Vec2_Zero, kGuiViewStateNormal);
    GuiView_SetColor(dimmer, Vec4_Create(0.0f, 0.0f, 0.0f, 1.0f), kGuiViewStateNormal);
    ctx->dimmer = dimmer;
    
    
    GuiView* menuBg = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    menuBg->tag = LAYER_MENU;
    menuBg->size = Vec2_Create(269, 330);
    menuBg->hidden = 1;
    menuBg->origin = Vec2_Create(512 - menuBg->size.x / 2.0, 384 - menuBg->size.y / 2.0);
    GuiView_SetUvs(menuBg, Vec2_Create(308, 1024-660), Vec2_Create(538, 660), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->menu = menuBg;
    
    
    GuiView* controls = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    controls->tag = LAYER_CONTROLS;
    controls->size = Vec2_Create(1024, 768);
    controls->origin = Vec2_Zero;
    controls->pressEnabled = 1;
    controls->hidden = 1;
    controls->atlas = kGuiAtlasInfo;
    GuiView_SetUvs(controls, Vec2_Create(0.0f, 256), Vec2_Create(1024.0f, 768.0f), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->controls = controls;
    
    GuiView* resumeButton = GuiSystem_SpawnView(&engine->guiSystem, menuBg);
    resumeButton->origin = Vec2_Create(0, 330-80);
    resumeButton->size = Vec2_Create(268, 65);
    resumeButton->label.anchor = Vec2_Create(0.5f, 0.6f);
    resumeButton->label.align = kTextAlignCenter;
    resumeButton->tag = LAYER_MENU_RESUME;
    resumeButton->pressEnabled = 1;
    resumeButton->atlas = kGuiAtlasNone;
    GuiLabel_SetText(&resumeButton->label, "Resume");
    
    GuiView* controlButton = GuiSystem_SpawnView(&engine->guiSystem, menuBg);
    controlButton->origin = Vec2_Create(0, 330-160);
    controlButton->size = Vec2_Create(268, 65);
    controlButton->label.anchor = Vec2_Create(0.5f, 0.6f);
    controlButton->label.align = kTextAlignCenter;
    controlButton->tag = LAYER_MENU_CONTROLS;
    controlButton->pressEnabled = 1;
    controlButton->atlas = kGuiAtlasNone;
    GuiLabel_SetText(&controlButton->label, "Controls");
    
    GuiView* quitButton = GuiSystem_SpawnView(&engine->guiSystem, menuBg);
    quitButton->origin = Vec2_Create(0, 6);
    quitButton->size = Vec2_Create(268, 65);
    quitButton->label.anchor = Vec2_Create(0.5f, 0.6f);
    quitButton->label.align = kTextAlignCenter;
    quitButton->tag = LAYER_MENU_QUIT;
    quitButton->pressEnabled = 1;
    quitButton->atlas = kGuiAtlasNone;
    GuiLabel_SetText(&quitButton->label, "Quit");

    GuiView* cursor = GuiSystem_SpawnView(&engine->guiSystem, engine->guiSystem.root);
    cursor->origin = Vec2_Create(0, 0);
    cursor->size = Vec2_Create(24, 24);
    cursor->tag = LAYER_CURSOR;
    GuiView_SetUvs(cursor, Vec2_Create(1024-338, 8), Vec2_Create(32, 32), Vec2_Create(1024, 1024), kGuiViewStateNormal);
    ctx->cursor = cursor;
    
    if (engine->inputSystem.config != kInputConfigMouseKeyboard)
    {
        cursor->hidden = 1;
    }
    
    ctx->fadeMode = FADE_IN;
    ctx->fadeTimer = FADE_TICKS;
}

static void Human_OnSelect(Player* controller, Unit* unit)
{
    Engine* engine = controller->engine;
    
    GuiView* primaryBtn = GuiSystem_FindView(&engine->guiSystem, LAYER_ATTACK_PRIMARY);
    GuiView* secondaryBtn = GuiSystem_FindView(&engine->guiSystem, LAYER_ATTACK_SECONDARY);

    if (unit != NULL)
    {
        GuiLabel_SetText(&GuiSystem_FindView(&engine->guiSystem, LAYER_UNIT_NAME)->label, unit->name);
        
        if (unit->primaryWeapon != -1)
        {
            const WeaponInfo* primaryWeapon = engine->weaponTable + unit->primaryWeapon;
            int column = primaryWeapon->guiIndex % 4;
            int row = primaryWeapon->guiIndex / 4;
            
            GuiView_SetUvs(primaryBtn, Vec2_Create(column * 210, 1024 - row * 284 - 284), Vec2_Create(210, 284), Vec2_Create(1024, 1024), kGuiViewStateNormal);
            primaryBtn->drawViewEnabled = 1;
        }
        else
        {
            primaryBtn->drawViewEnabled = 0;
        }
    
        if (unit->secondaryWeapon != -1)
        {
            const WeaponInfo* secondaryWeapon = engine->weaponTable + unit->secondaryWeapon;
            int column = secondaryWeapon->guiIndex % 4;
            int row = secondaryWeapon->guiIndex / 4;
            
            GuiView_SetUvs(secondaryBtn, Vec2_Create(column * 210, 1024 - row * 284 - 284), Vec2_Create(210, 284), Vec2_Create(1024, 1024), kGuiViewStateNormal);
            secondaryBtn->drawViewEnabled = 1;
        }
        else
        {
            GuiView_SetUvs(secondaryBtn, Vec2_Create(0, 0), Vec2_Create(210, 284), Vec2_Create(1024, 1024), kGuiViewStateNormal);
            secondaryBtn->drawViewEnabled = 0;
        }
    }
}


static void Human_OnTap(Player* controller, const PlayerEvent* event)
{
    Engine* engine = controller->engine;

    int moving = controller->state == kHumanStateCommand && engine->command.type == kCommandTypeMove;
    
    if (controller->state == kHumanStateIdle || moving)
    {
        int changeSelection = 0;
        Unit* target = NULL;
        float closestDistSq = 0.0f;

        for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        {
            Unit* unit = engine->sceneSystem.units + i;
            if (unit->dead) continue;
            
            if (AABB_IntersectsRay(unit->bounds, event->ray))
            {
                float distSq = Vec3_DistSq(unit->position, event->ray.origin);
                
                if (!target || distSq < closestDistSq)
                {
                    if (unit->playerId != controller->playerId && controller->selection != NULL)
                    {
                        target = unit;
                        closestDistSq = distSq;
                        changeSelection = 0;
                    }
                    else if (unit->playerId == controller->playerId)
                    {
                        target = unit;
                        closestDistSq = distSq;
                        changeSelection = 1;
                    }
                }
            }
        }
        
        if (changeSelection)
        {
            Player_ClearSelection(controller);
            Player_Select(controller, target);
        }
        else if (controller->selection != NULL)
        {
            if (event->navPoly && controller->selection->moveCounter > V_EPSILON)
            {
                Command command;
                command.playerId = controller->playerId;
                command.type = kCommandTypeMove;
                command.unit = controller->selection;
                command.position = event->intersection;
                command.target = target;
                
                Engine_RunCommand(engine, &command);
                controller->state = kHumanStateCommand;
            }
            else
            {
                SndSystem_PlaySound(&engine->soundSystem, SND_INVALID);
            }
        }
    }
    else if (controller->state == kHumanStatePreparePrimary || controller->state == kHumanStatePrepareSecondary)
    {
        const Unit* target = NULL;
        float closestDistSq = 0.0f;
        
        // see if we clicked on a unit
        for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
        {
            Unit* unit = engine->sceneSystem.units + i;
            if (unit->dead) continue;
            
            if (AABB_IntersectsRay(unit->bounds, event->ray))
            {
                if (unit->playerId != controller->playerId)
                {
                    const float distSq = Vec3_DistSq(unit->position, event->ray.origin);
                    if (!target || distSq < closestDistSq)
                    {
                        target = unit;
                        closestDistSq = distSq;
                    }
                }
            }
        }
        
        Vec3 p = Vec3_Zero;
        
        if (target)
        {
            p = target->position;
        }
        else if (event->navPoly)
        {
            // use nav poly of click
            p = event->intersection;
        }
        else
        {
            // use plane of player
            Plane castPlane = Plane_Create(Vec3_Zero, Vec3_Create(0.0f, 0.0f, 1.0f));
            
            if (controller->selection->navPoly)
            {
                castPlane = controller->selection->navPoly->plane;
            }
            
            float t;
            if (Plane_IntersectRay(castPlane, event->ray, &t))
            {
                p = Ray3_Slide(event->ray, t);
            }
        }
        
        Command command;
        command.playerId = controller->playerId;
        command.type = (controller->state == kHumanStatePreparePrimary) ? kCommandTypeAttackPrimary : kCommandTypeAttackSecondary;
        command.unit = controller->selection;
        command.position = Vec3_Create(p.x, p.y, p.z);

        Engine_RunCommand(engine, &command);
        controller->state = kHumanStateCommand;
    }
}


static void Human_OnEvent(Player* controller, const PlayerEvent* event)
{
    HumanContext* ctx = controller->userInfo;
    switch (event->type)
    {
        case kPlayerEventJoin:
        {
            controller->engine->guiSystem.delegate = controller;
            controller->engine->guiSystem.onEvent = Human_OnGuiEvent;
            controller->engine->guiSystem.onTick = Human_OnGuiTick;
            
            Human_PrepareGui(controller);
            break;
        }
        case kPlayerEventStartTurn:
        {
            controller->state = kHumanStateIdle;

            if (ctx->previousTurnSelection != NULL &&
                ctx->previousTurnSelection->dead == 0 &&
                ctx->previousTurnSelection->state != kUnitStateDead &&
                ctx->previousTurnSelection->playerId == controller->playerId)
            {
                Player_Select(controller, ctx->previousTurnSelection);
                
            }
            
            SndSystem_PlaySound(&controller->engine->soundSystem, SND_TURN_ALERT);
            break;
        }
        case kPlayerEventEndTurn:
        {
            ctx->previousTurnSelection = controller->selection;

            if (controller->engine->result == kEngineResultNone)
            {
                Player_ClearSelection(controller);
            }
            else
            {
                if (controller->engine->result == kEngineResultDefeat)
                {
                    SndSystem_PlaySound(&controller->engine->soundSystem, SND_LOSE_GAME);
                }
                
                ctx->fadeTimer = FADE_TICKS;
                ctx->fadeMode = FADE_OUT;
            }
            break;
        }
        case kPlayerEventStartCommand:
        {
            break;
        }
        case kPlayerEventEndCommand:
        {
            /* decide what to do next - issue command or end turn */
            controller->state = kHumanStateIdle;
            
            break;
        }
        case kPlayerEventTap:
        {
            Human_OnTap(controller, event);
            break;
        }
    }
}

void Human_Init(Player* controller,
                const UnitInfo* unitSpawnInfo,
                int unitSpawnInfoCount)
{
    Player_Init(controller);

    
    controller->onEvent = Human_OnEvent;
    controller->onSelect = Human_OnSelect;
    
    controller->unitSpawnInfo = unitSpawnInfo;
    controller->unitSpawnInfoCount = unitSpawnInfoCount;
    
    HumanContext* ctx = malloc(sizeof(HumanContext));
    ctx->lastViewedPlayer = 0;
    ctx->previousTurnSelection = NULL;
    controller->userInfo = ctx;
}
