#include "arena.h"
#include "components.h"

static const Clay_Color COLOR_FOREGROUND_PRIMARY = { 235, 235, 235, 255 };
static const Clay_Color COLOR_TEXTBUTTON_BACKGROUND = { 40, 40, 40, 255 };
static const Clay_Color COLOR_TEXTBUTTON_BACKGROUND_HOVER = { 60, 60, 60, 255 };
static const Clay_Color COLOR_TEXTBUTTON_BORDER = { 80, 80, 80, 255 };
static const Clay_Color COLOR_CHECKBUTTON_BORDER = { 160, 160, 160, 255 };
static const Clay_Color COLOR_CHECKBUTTON_CHECKED = { 66, 0, 255, 255 };
static const Clay_Color COLOR_CHECKBUTTON_UNCHECKED = { 120, 120, 120, 255 };
static const Clay_Color COLOR_TEXTTOOLTIP_BACKGROUND = { 20, 20, 20, 255 };

void C8Display(const C8DisplayData c8DisplayData)
{
    CustomElementData *customElementData = RequestAllocationFromArena(c8DisplayData.frameArena, sizeof(CustomElementData));
    customElementData->type = CUSTOM_ELEMENT_TYPE_C8DISPLAY;
    customElementData->virtualMachine = c8DisplayData.virtualMachine;

    CLAY({
        .custom = {
            .customData = customElementData
        },
        .layout = {
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    });
}

static void TextButton_OnHover(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
{
    const TextButtonData *textButtonData = (TextButtonData *)userData;
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME && textButtonData->onPressed)
        textButtonData->onPressed(textButtonData->pressedData);
}

void TextButton(const TextButtonData textButtonData)
{
    CLAY({
        .backgroundColor = Clay_Hovered() ? COLOR_TEXTBUTTON_BACKGROUND_HOVER : COLOR_TEXTBUTTON_BACKGROUND,
        .border = {
            .color = COLOR_TEXTBUTTON_BORDER,
            .width = CLAY_BORDER_ALL(1)
        },
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .padding = {
                .bottom = 8,
                .left = 16,
                .right = 16,
                .top = 8
            },
            .sizing = textButtonData.sizing
        }
    }) {
        CLAY_TEXT(
            textButtonData.text,
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_16PT,
                .fontSize = 16,
                .textColor = COLOR_FOREGROUND_PRIMARY,
            }));

        if (textButtonData.onPressed)
        {
            TextButtonData *userData = RequestAllocationFromArena(textButtonData.frameArena, sizeof(TextButtonData));
            *userData = textButtonData;

            Clay_OnHover(TextButton_OnHover, (intptr_t)userData);
        }
    }
}

static void CheckButton_OnHover(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
{
    const CheckButtonData *checkButtonData = (CheckButtonData *)userData;
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
        checkButtonData->onToggled(checkButtonData->toggledData);
}

void CheckButton(const CheckButtonData checkButtonData)
{
    CLAY({
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER,
            },
            .childGap = 8,
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        CLAY({
            .backgroundColor = checkButtonData.isChecked ? COLOR_CHECKBUTTON_CHECKED : COLOR_CHECKBUTTON_UNCHECKED,
            .border = {
                .color = COLOR_CHECKBUTTON_BORDER,
                .width = CLAY_BORDER_ALL(4)
            },
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .sizing = {
                    .height = CLAY_SIZING_FIXED(24),
                    .width = CLAY_SIZING_FIXED(24)
                }
            },
        }) {}

        CLAY_TEXT(
            checkButtonData.label,
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_16PT,
                .fontSize = 16,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        CheckButtonData *userData = RequestAllocationFromArena(checkButtonData.frameArena, sizeof(CheckButtonData));
        *userData = checkButtonData;

        Clay_OnHover(CheckButton_OnHover, (intptr_t)userData);
    }
}

void TextTooltip(const TextTooltipData textTooltipData)
{
    CLAY({
        .id = textTooltipData.elementId
    }) {
        CLAY_TEXT(
            textTooltipData.text,
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_16PT,
                .fontSize = 16,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));
    }

    if (Clay_PointerOver(textTooltipData.elementId))
    {
        CLAY({
            .backgroundColor = COLOR_TEXTTOOLTIP_BACKGROUND,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .floating = {
                .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                .parentId = textTooltipData.elementId.id,
                .offset = textTooltipData.offset
            },
            .layout = {
                .padding = CLAY_PADDING_ALL(8),
                .sizing = {
                    .width = CLAY_SIZING_FIXED(512)
                }
            }
        }) {
            CLAY_TEXT(
                textTooltipData.content,
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS
                }));
        }
    }
}