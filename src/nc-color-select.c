//
//  nc-color-select.c
//  noted
//
//  Created by Aidan Shafran on 12/16/17.
//  Copyright © 2017 zelbrium. All rights reserved.
//

#include "nc-color-select.h"
#include <math.h>

typedef struct
{
    float r, g, b;
} Color;

#define C(c) { (((c) >> 16) & 0xFF) / 255.f, (((c) >> 8) & 0xFF) / 255.f, ((c) & 0xFF) / 255.f }
static Color kColors[] = {
    C(0x000000), // Black
    C(0xFFFFFF), // White
    C(0xFFA500), // Orange
    C(0xFF0000), // Red
    C(0x800080), // Purple
    C(0x0000FF), // Blue
    C(0x00FF00), // Green
    C(0xFFFF00), // Yellow
};
#undef C

static unsigned int kNumColors = (unsigned int)(sizeof(kColors) / sizeof(Color));

struct NCColorSelect_
{
    NotedCanvas *canvas;
    int currentColor; // -1 for none selected
    float width, swatchRadius, swatchArea;
};

NCColorSelect * nc_color_select_new()
{
    return calloc(1, sizeof(NCColorSelect));
}

void nc_color_select_set_canvas(NCColorSelect *self, NotedCanvas *canvas)
{
    self->currentColor = -1;
    self->canvas = canvas;
    if(!canvas)
        return;
    
    // Match current color to canvas
    NCStrokeStyle style = noted_canvas_get_stroke_style(canvas);
    for(unsigned int i = 0; i < kNumColors; ++i)
    {
        if(style.r == kColors[i].r && style.g == kColors[i].g && style.b == kColors[i].b)
        {
            self->currentColor = i;
            break;
        }
    }
}

float nc_color_select_set_width(NCColorSelect *self, float width, float swatchSize, float swatchSpacing)
{
    self->width = width;
    self->swatchRadius = swatchSize / 2;
    self->swatchArea = swatchSize + swatchSpacing;
    
    int nColumns = floor(self->width / self->swatchArea);
    float pad = (self->width - (nColumns * self->swatchArea)) / 2.f;
    return pad + ceil(((kNumColors - 1) / nColumns) + 1) * self->swatchArea;
}

void nc_color_select_draw(NCColorSelect *self, cairo_t *cr)
{
    int nColumns = floor(self->width / self->swatchArea);
    float pad = (self->width - (nColumns * self->swatchArea)) / 2.f;
    
    if(nColumns == 0)
        return;
    
    cairo_set_line_width(cr, 1);
    for(int i = 0; i < kNumColors; ++i)
    {
        int row = i / nColumns;
        int col = i % nColumns;
        
        cairo_set_source_rgb(cr, kColors[i].r, kColors[i].g, kColors[i].b);
        cairo_arc(cr,
                  pad + col * self->swatchArea + self->swatchRadius,
                  pad + row * self->swatchArea + self->swatchRadius,
                  self->swatchRadius, 0, 2 * M_PI);
        cairo_fill_preserve(cr);
        if(self->currentColor == i)
            cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
        else
            cairo_set_source_rgba(cr, 0, 0, 0, 0.2);
        cairo_stroke(cr);
    }
}

void nc_color_select_input(NCColorSelect *self, NCInputState state, float x, float y)
{
    if(state != kNCToolDown || !self->canvas)
        return;
    
    int nColumns = floor(self->width / self->swatchArea);
    float pad = (self->width - (nColumns * self->swatchArea)) / 2.f;
    
    for(int i = 0; i < kNumColors; ++i)
    {
        int row = i / nColumns;
        int col = i % nColumns;
        
        float cx = pad + col * self->swatchArea + self->swatchRadius;
        float cy = pad + row * self->swatchArea + self->swatchRadius;
        
        if(sqrt((cx-x) * (cx-x) + (cy-y) * (cy-y)) < self->swatchRadius)
        {
            self->currentColor = i;
            NCStrokeStyle style = noted_canvas_get_stroke_style(self->canvas);
            style.r = kColors[i].r * 255;
            style.g = kColors[i].g * 255;
            style.b = kColors[i].b * 255;
            style.a = 255;
            noted_canvas_set_stroke_style(self->canvas, style);
            break;
        }
    }
}
