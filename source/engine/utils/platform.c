
#include "platform.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>

const int Utils_endian = 1;
char Filepath_directories[kDirectoryCount][MAX_OS_PATH];

char* Filepath_Extension(const char* path)
{
    assert(path);
    
    static char exten[8];
    char* j;
    char* i;
    
    i = (char*)path + strnlen(path, MAX_OS_PATH);
    j = (char*)exten + 7;
    
    exten[7] = '\0';
    
    while (i != path && *i != '.')
    {
        j--;
        i--;
        *j = *i;
    }
    ++j;
    
    return j;
}

void Filepath_Append(char* dest, const char* base, const char* sub)
{
    assert(dest && base && sub);
    
    size_t baseLength = strnlen(base, MAX_OS_PATH);
    
    int baseHasDivider = (baseLength != 0 && base[baseLength - 1] == '/');
    int subHasDivider = (sub[0] == '/');
    
    const char* format = (baseHasDivider || subHasDivider) ? "%s%s" : "%s/%s";

    snprintf(dest, MAX_OS_PATH, format, base, sub);

}

void Filepath_SetDirectory(Directory directory, const char* path)
{
    strncpy(Filepath_directories[directory], path, MAX_OS_PATH);
}

char* Filepath_DataDir()
{
    return Filepath_directories[kDirectoryData];
}


int String_IsEmpty(const char* s)
{
    if (!s) return 1;
    
    while (*s != '\0')
    {
        if (!isspace(*s)) return 0;
        ++s;
    }
    return 1;
}

int String_Prefix(const char* str, const char* pre, size_t maxSize)
{
    return strncmp(pre, str, maxSize) == 0;
}

unsigned long String_Hash(const char* string)
{
    assert(string);
    
    unsigned long hash = (unsigned long)*string;
    
    for (string += 1; *string != '\0'; ++string)
        hash = (hash << 5) - hash + (unsigned long)*string;
    
    return hash;
}
