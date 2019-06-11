
#ifndef PLAYER_H
#define PLAYER_H

#include "vec_math.h"
#include "scene_system.h"


typedef enum
{
    kCommandTypeEndTurn = 0,
    kCommandTypeMove,
    kCommandTypeAttackPrimary,
    kCommandTypeAttackSecondary,
    kCommandTypeTeleport,
    kCommandTypeIdle,
} CommandType;

typedef struct
{
    int playerId; /* the player that issued the command */
    CommandType type;
    const Unit* unit;
    const Unit* target;
    Vec3 position;
    int aiAction;
} Command;


typedef enum
{
    kPlayerEventJoin = 0,
    kPlayerEventStartTurn,
    kPlayerEventEndTurn,
    kPlayerEventStartCommand,
    kPlayerEventEndCommand,
    kPlayerEventTap,
    
} PlayerEventType;


typedef struct
{
    PlayerEventType type;
    const Command* command;

    /* Tap info */
    Ray3 ray;
    Vec3 intersection;
    const NavPoly* navPoly;
} PlayerEvent;


typedef struct Player
{
    struct Engine* engine;
    
    const UnitInfo* unitSpawnInfo;
    int unitSpawnInfoCount;
    
    int skulls;
    int playerId;
    int state;
    
    int unitCount;
    
    Unit* selection;
    
    int ourTurn;
    
    void* userInfo;
    
    Command previousCommand;
    
    void (*onSelect)(struct Player* controller, Unit* selection);
    void (*onEvent)(struct Player* controller, const PlayerEvent* event);
    
} Player;

extern void Player_Init(Player* controller);

extern void Player_Select(Player* controller, Unit* unit);
extern void Player_ClearSelection(Player* controller);

extern void Player_Event(Player* controller, const PlayerEvent* event);

extern int Player_GetResultUnitInfos(Player* controller, UnitInfo* buffer);

#endif /* controller_h */
