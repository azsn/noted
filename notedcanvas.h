#ifndef notedcanvas_h
#define notedcanvas_h

#include <stdio.h>
#include <cairo/cairo.h>

typedef struct NotedCanvas_ NotedCanvas;

typedef struct
{
    float x1, y1, x2, y2;
} NCRect;

/*
 * Called when a region of the canvas has been invalidated (should
 * be redrawn). rect may be NULL if all that changed is the number
 * of pages.
 */
typedef void (*NCInvalidateCallback)(NotedCanvas *canvas, NCRect *rect, unsigned long npages, void *data);


/*
 * Create canvas.
 */
NotedCanvas * noted_canvas_new(NCInvalidateCallback invalidateCallback, void *data);

/*
 * Destroy canvas.
 */
void noted_canvas_destroy(NotedCanvas *canvas);

/*
 * Redraw the canvas. This should be called as a response to
 * NotedCanvasInvalidateCallback once the backend has initiated
 * a redraw. Use cairo_clip() to specify the region to redraw.
 */
void noted_canvas_draw(NotedCanvas *canvas, cairo_t *cr);

/*
 * Call on a mouse event. State: 0 (mouse down), 1 (mouse drag), 2 (mouse up)
 */
void noted_canvas_mouse(NotedCanvas *canvas, int state, float x, float y, float pressure);

#endif /* notedcanvas_h */
