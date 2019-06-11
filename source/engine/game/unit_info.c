
#include "unit_info.h"

void UnitInfo_SetName(UnitInfo* info, const char* name)
{
    strncpy(info->name, name, UNIT_NAME_MAX);
}
