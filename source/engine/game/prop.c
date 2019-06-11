

#include "prop.h"
#include "engine.h"

int Prop_Init(Prop* prop, struct Engine* engine, int index)
{
    prop->engine = engine;
    prop->index = index;
    strcpy(prop->identifier, "");
    prop->dead = 1;
    prop->inactive = 0;
    prop->touchEnabled = 0;
    
    prop->hp = 0;
    prop->owner = NULL;
    
    prop->position = Vec3_Zero;
    prop->forward = Vec3_Zero;
    prop->target = Vec3_Zero;
    
    prop->rotation = Quat_Identity;
    prop->spawnRotation = Vec3_Zero;
    
    prop->model.loaded = 0;
    prop->navPoly = NULL;
    
    prop->visible = 1;
    prop->timer = 0;
    
    prop->playerId = -1;
        
    prop->onAction = NULL;
    prop->onSpawn = NULL;
    prop->onTouchUnit = NULL;
    prop->onTouch = NULL;
    prop->onDamage = NULL;
    
    prop->onTick = NULL;
    prop->onEvent = NULL;
    prop->onVar = NULL;
    
    prop->speed = 0.0f;
    
    for (int i = 0; i < PROP_DATA_VAR_MAX; ++i)
        prop->data[i] = -1;
    
    for (int i = 0; i < PROP_EVENTS_MAX; ++i)
        prop->events[i].event = kEventNone;
    
    return 1;
}

void Prop_SendVar(Prop* target, const char* key, const char* val)
{
    if (target && target->onVar)
        target->onVar(target, key, val);
}

void Prop_InputEvent(Prop* target, Prop* sender, EventType event, int data)
{
    if (target)
    {
        switch (event)
        {
            case kEventDie:
                target->dead = 1;
                break;
            case kEventActivate:
                target->inactive = 0;
                break;
            case kEventDeactivate:
                target->inactive = 1;
                break;                
            default:
                break;
        }
    }
    
    Prop_OutputEvent(target, sender, event, data);
}

void Prop_OutputEvent(Prop* target, Prop* sender, EventType event, int data)
{
    if (!target)
        return;
    
    if (target->onEvent)
        target->onEvent(target, sender, event, data);
    
    for (int i = 0; i < PROP_EVENTS_MAX; ++i)
    {
        if (target->events[i].event == event)
        {
            Prop* found = SceneSystem_FindProp(&target->engine->sceneSystem, target->events[i].target);
            
            // endless loop sending events to ourselves
            if (found && !(found == target && event == target->events[i].action))
            {
                Prop_InputEvent(found, target, target->events[i].action, target->events[i].data);
            }
        }
    }
    
}

void Prop_Kill(Prop* prop)
{
    Prop_InputEvent(prop, NULL, kEventDie, 0);
}

void Prop_Damage(Prop* prop,
                 Prop* other,
                 int damage,
                 DamageType damageType)
{
    if (prop->onDamage)
        prop->onDamage(prop, other, damage, damageType);
}
