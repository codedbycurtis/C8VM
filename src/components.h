#ifndef C8VM_COMPONENTS_H
#define C8VM_COMPONENTS_H

#include "arena.h"
#include "core.h"
#include "clay.h"

typedef struct
{
    Arena *frameArena;

    C8VM *virtualMachine;
} C8DisplayData;

typedef struct
{
    Arena *frameArena;

    Clay_String text;
    Clay_Sizing sizing;

    void (*onPressed)(void *pressedData);
    void *pressedData;
} TextButtonData;

typedef struct
{
    Arena *frameArena;

    bool isChecked;
    Clay_String label;

    void (*onToggled)(void *toggledData);
    void *toggledData;
} CheckButtonData;

typedef struct
{
    Clay_ElementId elementId;
    Clay_String text;
    Clay_String content;
    Clay_Vector2 offset;
} TextTooltipData;

void C8Display(C8DisplayData c8DisplayData);
void TextButton(TextButtonData textButtonData);
void CheckButton(CheckButtonData checkButtonData);
void TextTooltip(TextTooltipData textTooltipData);

#endif // C8VM_COMPONENTS_H