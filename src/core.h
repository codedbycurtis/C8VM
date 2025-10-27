#ifndef C8VM_CORE_H
#define C8VM_CORE_H

#include <stdint.h>

#include "vm.h"

typedef struct
{
    bool isRunning;
    char *programPath;
    uint16_t cyclesPerSecond;
    C8_Instance instance;
} C8VM;

typedef enum
{
    FONT_PIXELOID_SANS_16PT,
    FONT_PIXELOID_SANS_BOLD_32PT,
    FONT_PIXELOID_SANS_BOLD_48PT
} Font;

typedef enum
{
    CUSTOM_ELEMENT_TYPE_C8DISPLAY
} CustomElementType;

typedef struct
{
    CustomElementType type;
    union
    {
        C8VM *virtualMachine;
    };
} CustomElementData;

#endif // C8VM_CORE_H