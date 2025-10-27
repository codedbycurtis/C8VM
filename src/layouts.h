#ifndef C8VM_LAYOUTS_H
#define C8VM_LAYOUTS_H

#include "vm.h"
#include "arena.h"
#include "core.h"
#include "clay.h"

typedef enum
{
    LAYOUT_SELECT,
    LAYOUT_SETTINGS,
    LAYOUT_CONTROLS,
    LAYOUT_CREDITS,
    LAYOUT_MAIN
} Layout;

typedef struct
{
    Arena *frameArena;
    Layout *layout;
    C8VM *virtualMachine;
} LayoutData;

Clay_RenderCommandArray SelectLayout_CreateLayout(LayoutData *data);
Clay_RenderCommandArray SettingsLayout_CreateLayout(LayoutData *data);
Clay_RenderCommandArray ControlsLayout_CreateLayout(LayoutData *data);
Clay_RenderCommandArray CreditsLayout_CreateLayout(LayoutData *data);
Clay_RenderCommandArray MainLayout_CreateLayout(LayoutData *data);

#endif // C8VM_LAYOUTS_H