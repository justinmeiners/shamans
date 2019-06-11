

#include "player.h"
#include <string.h>
#include "engine.h"

void Player_Init(Player* controller)
{
    memset(controller, 0, sizeof(Player));
    controller->selection = NULL;
    controller->engine = NULL;
    controller->onSelect = NULL;
    controller->unitCount = 0;
}

void Player_Select(Player* controller, Unit* unit)
{
    if (unit != controller->selection)
    {
        Player_ClearSelection(controller);
        
        controller->selection = unit;
        unit->selected = 1;
        
        if (unit->onSelect)
        {
            unit->onSelect(unit);
        }
    }
    
    if (controller->onSelect)
    {
        controller->onSelect(controller, unit);
    }
}

void Player_ClearSelection(Player* controller)
{
    Unit* unit = controller->selection;
    
    if (unit != NULL)
    {
        unit->selected = 0;
        
        if (unit->onDeslect)
        {
            unit->onDeslect(unit);
        }
    }
    
    controller->selection = NULL;
}

void Player_Event(Player* controller, const PlayerEvent* event)
{
    if (controller && event && controller->onEvent)
        controller->onEvent(controller, event);
}

int Player_GetResultUnitInfos(Player* controller, UnitInfo* buffer)
{
    int count = 0;
    
    for (int i = 0; i < SCENE_SYSTEM_UNITS_MAX; ++i)
    {
        const Unit* unit = controller->engine->sceneSystem.units + i;
        if (unit->dead || unit->playerId != controller->playerId ||unit->state == kUnitStateDead) { continue; }
        
        buffer[count].crewIndex = unit->crewIndex;
        buffer[count].primaryWeapon = unit->primaryWeapon;
        buffer[count].secondaryWeapon = unit->secondaryWeapon;
        buffer[count].type = unit->type;
        strncpy(buffer[count].name, unit->name, UNIT_NAME_MAX);
        ++count;
    }
    
    // find enemies which were killed
    for (int i = 0; i < controller->unitSpawnInfoCount; ++i)
    {
        const UnitInfo* spawnInfo = controller->unitSpawnInfo + i;
        
        int found = 0;
        
        for (int j = 0; j < count; ++j)
        {
            if (buffer[j].crewIndex == spawnInfo->crewIndex)
            {
                found = 1;
                break;
            }
        }
        
        if (!found)
        {
            buffer[count].crewIndex = spawnInfo->crewIndex;
            // mark as deleted, other info not relevant
            buffer[count].type = kUnitNone;
            ++count;
        }
    }
    
    return count;
}
