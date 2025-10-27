#include <SDL3/SDL.h>

#include "components.h"
#include "layouts.h"

#include <stdio.h>

static constexpr Clay_Color COLOR_BACKGROUND_PRIMARY = { 10, 10, 10, 255 };
static constexpr Clay_Color COLOR_BACKGROUND_SEMI_TRANSPARENT = { 0, 0, 0, 230 };
static constexpr Clay_Color COLOR_FOREGROUND_PRIMARY = { 245, 245, 245, 255 };

static void SelectLayout_OpenFileDialogCallback(void *userdata, const char * const *filelist, int filter)
{
    const LayoutData *data = userdata;

    if (!filelist)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_ShowOpenFileDialog failed: %s\n", SDL_GetError());
        return;
    }

    // User did not select a file or cancelled the dialog.
    if (!*filelist)
        return;

    char *error;
    if (!C8_LoadProgram(&data->virtualMachine->instance, filelist[0], &error))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "C8_LoadProgram failed: %s\n", error);
        return;
    }

    if (data->virtualMachine->programPath)
        SDL_free(data->virtualMachine->programPath);
    data->virtualMachine->programPath = SDL_strdup(filelist[0]);
    data->virtualMachine->isRunning = true;
    *data->layout = LAYOUT_MAIN;
}

static void SelectLayout_OnSelectProgramPressed(void *pressedData)
{
    SDL_ShowOpenFileDialog(SelectLayout_OpenFileDialogCallback, pressedData, nullptr, nullptr, 0, nullptr, false);
}

static void SelectLayout_OnSettingsPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    *data->layout = LAYOUT_SETTINGS;
}

static void SelectLayout_OnControlsPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    *data->layout = LAYOUT_CONTROLS;
}

static void SelectLayout_OnCreditsPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    *data->layout = LAYOUT_CREDITS;
}

static void SelectLayout_OnExitPressed(void *pressedData)
{
    exit(0);
}

Clay_RenderCommandArray SelectLayout_CreateLayout(LayoutData *data) {
    ResetArena(data->frameArena);

    Clay_BeginLayout();

    CLAY({
        .backgroundColor = COLOR_BACKGROUND_PRIMARY,
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .childGap = 32,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    }) {
        CLAY({
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("C8VM"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_BOLD_48PT,
                    .fontSize = 48,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));

            CLAY_TEXT(
                CLAY_STRING("A CHIP-8 virtual machine."),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));
        }

        CLAY({
            .layout = {
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_FIT()
                }
            }
        }) {
            TextButton((TextButtonData){
                .frameArena = data->frameArena,
                .text = CLAY_STRING("Select Program"),
                .sizing = {
                    .width = CLAY_SIZING_GROW()
                },
                .onPressed = SelectLayout_OnSelectProgramPressed,
                .pressedData = data
            });

            TextButton((TextButtonData){
                .frameArena = data->frameArena,
                .text = CLAY_STRING("Settings"),
                .sizing = {
                    .width = CLAY_SIZING_GROW()
                },
                .onPressed = SelectLayout_OnSettingsPressed,
                .pressedData = data
            });

            TextButton((TextButtonData){
                .frameArena = data->frameArena,
                .text = CLAY_STRING("Controls"),
                .sizing = {
                    .width = CLAY_SIZING_GROW()
                },
                .onPressed = SelectLayout_OnControlsPressed,
                .pressedData = data
            });

            TextButton((TextButtonData){
                .frameArena = data->frameArena,
                .text = CLAY_STRING("Credits"),
                .sizing = {
                    .width = CLAY_SIZING_GROW()
                },
                .onPressed = SelectLayout_OnCreditsPressed,
                .pressedData = data
            });

            TextButton((TextButtonData){
                .frameArena = data->frameArena,
                .text = CLAY_STRING("Exit to Desktop"),
                .sizing = {
                    .width = CLAY_SIZING_GROW()
                },
                .onPressed = SelectLayout_OnExitPressed
            });
        }

        CLAY({
            .floating = {
                .attachTo = CLAY_ATTACH_TO_PARENT,
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
            },
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_LEFT,
                    .y = CLAY_ALIGN_Y_BOTTOM
                },
                .padding = CLAY_PADDING_ALL(8),
                .sizing = {
                    .height = CLAY_SIZING_GROW(),
                    .width = CLAY_SIZING_GROW()
                }
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("Version: 1.0.0"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));
        }
    }

    return Clay_EndLayout();
}

static void SettingsLayout_OnShiftInPlaceToggled(void *toggledData)
{
    const LayoutData *data = toggledData;
    data->virtualMachine->instance.config.useParameterisedShift = !data->virtualMachine->instance.config.useParameterisedShift;
}

static void SettingsLayout_OnUseParameterisedJumpToggled(void *toggledData)
{
    const LayoutData *data = toggledData;
    data->virtualMachine->instance.config.useParameterisedJump = !data->virtualMachine->instance.config.useParameterisedJump;
}

static void SettingsLayout_OnUseTemporaryIndexToggled(void *toggledData)
{
    const LayoutData *data = toggledData;
    data->virtualMachine->instance.config.useTemporaryIndex = !data->virtualMachine->instance.config.useTemporaryIndex;
}

static void SettingsLayout_OnIncreaseCyclesPressed(void *toggledData)
{
    const LayoutData *data = toggledData;
    data->virtualMachine->cyclesPerSecond = SDL_min(1000, data->virtualMachine->cyclesPerSecond + 100);
}

static void SettingsLayout_OnDecreaseCyclesPressed(void *toggledData)
{
    const LayoutData *data = toggledData;
    data->virtualMachine->cyclesPerSecond = SDL_max(100, data->virtualMachine->cyclesPerSecond - 100);
}

static void SettingsLayout_OnBackPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    *data->layout = LAYOUT_SELECT;
}

Clay_RenderCommandArray SettingsLayout_CreateLayout(LayoutData *data)
{
    ResetArena(data->frameArena);

    Clay_BeginLayout();

    CLAY({
        .backgroundColor = COLOR_BACKGROUND_PRIMARY,
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .childGap = 32,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    }) {
        CLAY_TEXT(
            CLAY_STRING("Settings"),
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_BOLD_32PT,
                .fontSize = 32,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        CLAY({
            .layout = {
                .childGap = 8
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("Configure Quirks"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));

            TextTooltip((TextTooltipData){
                .elementId = CLAY_ID("ConfigureQuirks"),
                .text = CLAY_STRING("(?)"),
                .content = CLAY_STRING("Quirks are differences in instruction implementation between CHIP-8 interpreters.\nNewer programs typically expect all quirks to be enabled, whereas older programs may require specific quirks disabled to function."),
                .offset = {
                    .x = 0.0f,
                    .y = -120.0f
                }
            });
        }

        CLAY({
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_LEFT,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            CheckButton((CheckButtonData){
                .frameArena = data->frameArena,
                .isChecked = data->virtualMachine->instance.config.useParameterisedShift,
                .label = CLAY_STRING("Use Parameterised Shift"),
                .onToggled = SettingsLayout_OnShiftInPlaceToggled,
                .toggledData = data
            });

            CheckButton((CheckButtonData){
                .frameArena = data->frameArena,
                .isChecked = data->virtualMachine->instance.config.useParameterisedJump,
                .label = CLAY_STRING("Use Parameterised Jump"),
                .onToggled = SettingsLayout_OnUseParameterisedJumpToggled,
                .toggledData = data
            });

            CheckButton((CheckButtonData){
                .frameArena = data->frameArena,
                .isChecked = data->virtualMachine->instance.config.useTemporaryIndex,
                .label = CLAY_STRING("Use Temporary Index"),
                .onToggled = SettingsLayout_OnUseTemporaryIndexToggled,
                .toggledData = data
            });
        }

        CLAY({
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 32
            }
        }) {
            CLAY({
                .layout = {
                    .childGap = 8
                }
            }) {
                CLAY_TEXT(
                    CLAY_STRING("Clock Rate"),
                    CLAY_TEXT_CONFIG({
                        .fontId = FONT_PIXELOID_SANS_16PT,
                        .fontSize = 16,
                        .textColor = COLOR_FOREGROUND_PRIMARY
                    }));

                TextTooltip((TextTooltipData){
                    .elementId = CLAY_ID("ClockRate"),
                    .text = CLAY_STRING("(?)"),
                    .content = CLAY_STRING("Determines how many CHIP-8 instructions are executed per second.\nSome programs may require a different clock rate, but 600Hz should be suitable in most cases.\nNote: the virtual machine will attempt to run at this speed, but the actual clock rate may vary."),
                    .offset = {
                        .x = 0.0f,
                        .y = -140.0f
                    }
                });
            }

            CLAY({
                .layout = {
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER
                    },
                    .childGap = 16
                }
            }) {
                TextButton((TextButtonData){
                    .frameArena = data->frameArena,
                    .text = CLAY_STRING("<"),
                    .onPressed = SettingsLayout_OnDecreaseCyclesPressed,
                    .pressedData = data
                });

                // Maximum cycle count = 1000 (4 chars + 1 null terminator)
                char *cycleText = RequestAllocationFromArena(data->frameArena, sizeof(char) * 5);
                sprintf_s(cycleText, sizeof(char) * 5, "%d", data->virtualMachine->cyclesPerSecond);

                const Clay_String crString = {
                    .chars = cycleText,
                    .length = (int32_t)strlen(cycleText),
                    .isStaticallyAllocated = false
                };

                CLAY_TEXT(
                    crString,
                    CLAY_TEXT_CONFIG({
                        .fontId = FONT_PIXELOID_SANS_16PT,
                        .fontSize = 16,
                        .textColor = COLOR_FOREGROUND_PRIMARY
                    }));

                TextButton((TextButtonData){
                    .frameArena = data->frameArena,
                    .text = CLAY_STRING(">"),
                    .onPressed = SettingsLayout_OnIncreaseCyclesPressed,
                    .pressedData = data
                });
            }
        }

        TextButton((TextButtonData){
            .frameArena = data->frameArena,
            .text = CLAY_STRING("Back"),
            .onPressed = SettingsLayout_OnBackPressed,
            .pressedData = data
        });
    }

    return Clay_EndLayout();
}

static void ControlsLayout_OnBackPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    *data->layout = LAYOUT_SELECT;
}

Clay_RenderCommandArray ControlsLayout_CreateLayout(LayoutData *data)
{
    ResetArena(data->frameArena);

    Clay_BeginLayout();

    CLAY({
        .backgroundColor = COLOR_BACKGROUND_PRIMARY,
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .childGap = 32,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    }) {
        CLAY_TEXT(
            CLAY_STRING("Controls"),
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_BOLD_32PT,
                .fontSize = 32,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        CLAY({
            .layout = {
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 16,
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("You:\n1 2 3 4\nQ W E R\nA S D F\nZ X C V"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));

            CLAY_TEXT(
                CLAY_STRING("->"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));

            CLAY_TEXT(
                CLAY_STRING("CHIP-8:\n1 2 3 C\n4 5 6 D\n7 8 9 E\nA 0 B F"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));
        }

        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("[ESC] Pause/Resume Program"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));

            CLAY_TEXT(
                CLAY_STRING("[F11] Toggle Fullscreen"),
                CLAY_TEXT_CONFIG({
                    .fontId = FONT_PIXELOID_SANS_16PT,
                    .fontSize = 16,
                    .textColor = COLOR_FOREGROUND_PRIMARY
                }));
        }

        TextButton((TextButtonData){
            .frameArena = data->frameArena,
            .text = CLAY_STRING("Back"),
            .onPressed = ControlsLayout_OnBackPressed,
            .pressedData = data
        });
    }

    return Clay_EndLayout();
}

void CreditsLayout_OnBackPressed(void *pressedData)
{
    LayoutData *data = pressedData;
    *data->layout = LAYOUT_SELECT;
}

Clay_RenderCommandArray CreditsLayout_CreateLayout(LayoutData *data)
{
    ResetArena(data->frameArena);

    Clay_BeginLayout();

    CLAY({
        .backgroundColor = COLOR_BACKGROUND_PRIMARY,
        .layout = {
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .childGap = 32,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    }) {
        CLAY_TEXT(
            CLAY_STRING("Credits"),
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_BOLD_32PT,
                .fontSize = 32,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        CLAY_TEXT(
            CLAY_STRING("\"C8VM\" by Curtis Caulfield.\nhttps://curtiscaulfield.com\nLicensed under the MIT License."),
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_16PT,
                .fontSize = 16,
                .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        CLAY_TEXT(
            CLAY_STRING("\"Pixeloid Sans\" by GGBotNet.\nhttps://ggbot.itch.io/pixeloid-font\nLicensed under the SIL Open Font License."),
            CLAY_TEXT_CONFIG({
                .fontId = FONT_PIXELOID_SANS_16PT,
                .fontSize = 16,
                .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                .textColor = COLOR_FOREGROUND_PRIMARY
            }));

        TextButton((TextButtonData){
            .frameArena = data->frameArena,
            .text = CLAY_STRING("Back"),
            .onPressed = CreditsLayout_OnBackPressed,
            .pressedData = data
        });
    }

    return Clay_EndLayout();
}

static void MainLayout_OnResumePressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    data->virtualMachine->isRunning = true;
}

static void MainLayout_OnRestartPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    C8_Reset(&data->virtualMachine->instance);
    char *error;
    if (!C8_LoadProgram(&data->virtualMachine->instance, data->virtualMachine->programPath, &error))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "C8_LoadProgram failed: %s\n", error);
        return;
    }
    data->virtualMachine->isRunning = true;
}

static void MainLayout_OnExitPressed(void *pressedData)
{
    const LayoutData *data = pressedData;
    data->virtualMachine->isRunning = false;
    C8_Reset(&data->virtualMachine->instance);
    *data->layout = LAYOUT_SELECT;
}

Clay_RenderCommandArray MainLayout_CreateLayout(LayoutData *data)
{
    ResetArena(data->frameArena);

    Clay_BeginLayout();

    CLAY({
        .backgroundColor = COLOR_BACKGROUND_PRIMARY,
        .layout = {
            .sizing = {
                .height = CLAY_SIZING_GROW(),
                .width = CLAY_SIZING_GROW()
            }
        }
    }) {
        if (!data->virtualMachine->isRunning)
        {
            CLAY({
                .backgroundColor = COLOR_BACKGROUND_SEMI_TRANSPARENT,
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
                .layout = {
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER
                    },
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .height = CLAY_SIZING_GROW(),
                        .width = CLAY_SIZING_GROW()
                    }
                }
            }) {
                CLAY_TEXT(
                    CLAY_STRING("Paused"),
                    CLAY_TEXT_CONFIG({
                        .fontId = FONT_PIXELOID_SANS_BOLD_32PT,
                        .fontSize = 32,
                        .textColor = COLOR_FOREGROUND_PRIMARY
                    }));

                CLAY({
                    .layout = {
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_CENTER,
                            .y = CLAY_ALIGN_Y_CENTER
                        },
                        .childGap = 8,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    TextButton((TextButtonData){
                        .frameArena = data->frameArena,
                        .text = CLAY_STRING("Resume"),
                        .sizing = {
                            .width = CLAY_SIZING_GROW()
                        },
                        .onPressed = MainLayout_OnResumePressed,
                        .pressedData = data
                    });

                    TextButton((TextButtonData){
                        .frameArena = data->frameArena,
                        .text = CLAY_STRING("Restart"),
                        .sizing = {
                            .width = CLAY_SIZING_GROW()
                        },
                        .onPressed = MainLayout_OnRestartPressed,
                        .pressedData = data
                    });

                    TextButton((TextButtonData){
                        .frameArena = data->frameArena,
                        .text = CLAY_STRING("Exit to Menu"),
                        .sizing = {
                            .width = CLAY_SIZING_GROW()
                        },
                        .onPressed = MainLayout_OnExitPressed,
                        .pressedData = data
                    });
                }
            }
        }

        C8Display((C8DisplayData){
            .frameArena = data->frameArena,
            .virtualMachine = data->virtualMachine
        });
    }

    return Clay_EndLayout();
}