#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif
#include "nuklear/spritesheet_ui.h"

extern struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init();
void nk_ext_sprites_destroy();

#ifdef NK_IMPLEMENTATION

static struct nk_image ui_spritesheet;
struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init()
{
    if (NK_UI_SCALE > 2)
    {
        nk_imageloadm(res_spritesheet_ui_3x_data, res_spritesheet_ui_3x_size, &ui_spritesheet);
        nk_image2texture(&ui_spritesheet, 0);
        nk_spritesheet_init_ui_3x(ui_spritesheet, &sprites_ui);
    }
    else if (NK_UI_SCALE > 1)
    {
        nk_imageloadm(res_spritesheet_ui_2x_data, res_spritesheet_ui_2x_size, &ui_spritesheet);
        nk_image2texture(&ui_spritesheet, 0);
        nk_spritesheet_init_ui_2x(ui_spritesheet, &sprites_ui);
    }
    else
    {
        nk_imageloadm(res_spritesheet_ui_1x_data, res_spritesheet_ui_1x_size, &ui_spritesheet);
        nk_image2texture(&ui_spritesheet, 0);
        nk_spritesheet_init_ui_1x(ui_spritesheet, &sprites_ui);
    }
}

void nk_ext_sprites_destroy()
{
    nk_imagetexturefree(&ui_spritesheet);
}
#endif