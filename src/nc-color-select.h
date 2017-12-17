//
//  nc-color-select.h
//  noted
//
//  Created by Aidan Shafran on 12/16/17.
//  Copyright © 2017 zelbrium. All rights reserved.
//

#ifndef nc_color_select_h
#define nc_color_select_h

#include "notedcanvas.h"

typedef struct NCColorSelect_ NCColorSelect;

/*
 * Creates a color selection widget, for use with the canvas.
 */
NCColorSelect * nc_color_select_new();

/*
 * Attaches the color select widget to the canvas.
 */
void nc_color_select_set_canvas(NCColorSelect *colorselect, NotedCanvas *canvas);

/*
 * Sets the width of the widget (and swatch size).
 * Returns the height request for that width.
 */
float nc_color_select_set_width(NCColorSelect *self, float width, float swatchSize, float swatchSpacing);

/*
 * Draw the widget into cr.
 */
void nc_color_select_draw(NCColorSelect *colorselect, cairo_t *cr);

/*
 * Send mouse input to the widget.
 * Input in the range of the width and height of the widget.
 */
void nc_color_select_input(NCColorSelect *colorselect, NCInputState state, float x, float y);

#endif /* nc_color_select_h */
