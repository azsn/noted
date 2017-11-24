/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * nc-private.h: NotedCanvas private methods and types.
 */

#ifndef nc_private_h
#define nc_private_h

#include <stdio.h>
#include "notedcanvas.h"

typedef struct Page_ Page;

typedef struct
{
    Page *page; // Owner page
    float *x; // Array of xs
    float *y; // Array of ys
    NCRect bounds;
    NCStrokeStyle style;
    float maxDistSq; // Longest distance (squared) between two consecutive points
} Stroke;

struct Page_
{
    Stroke *strokes;
    NCRect bounds;
    NCPagePattern pattern;
    unsigned int density;
};

struct NotedCanvas_
{
    NCInvalidateCallback invalidateCallback;
    void *callbackData;
    unsigned long lastStroke; // Used for undo
    Page *pages;
    Stroke *currentStroke;
    float eraserPrevX, eraserPrevY;
    NCStrokeStyle currentStyle;
    char *path;
};

void free_stroke(Stroke *s);
void free_page(Page *p);

inline void rect_expand_by_point(NCRect *a, float x, float y)
{
    if(x > a->x2)
        a->x2 = x;
    if(x < a->x1)
        a->x1 = x;
    if(y > a->y2)
        a->y2 = y;
    if(y < a->y1)
        a->y1 = y;
}

inline float sq_dist(float x1, float y1, float x2, float y2)
{
    return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
}

bool noted_canvas_save(NotedCanvas *canvas, const char *path);

#endif /* nc_private_h */
