#ifndef notedcanvas_h
#define notedcanvas_h

#include <stdio.h>
#include <cairo/cairo.h>

typedef struct NotedCanvas_ NotedCanvas;

typedef struct
{
    float x1, y1, x2, y2;
} NotedRect;


/*
 * Informs of the size request of the canvas. The canvas should be at least as big as width/height.
 */
typedef void (*NotedCanvasSizeCallback)(NotedCanvas *canvas, unsigned long width, unsigned long height, void *data);

/*
 * Called when a region of the canvas has been invalidated (should be redrawn).
 */
typedef void (*NotedCanvasInvalidateCallback)(NotedCanvas *canvas, NotedRect *rect, void *data);


/*
 * Create canvas.
 */
NotedCanvas * noted_canvas_new(NotedCanvasSizeCallback sizeCallback, NotedCanvasInvalidateCallback invalidateCallback, void *data);

/*
 * Destroy canvas.
 */
void noted_canvas_destroy(NotedCanvas *canvas);

/*
 * The size request of the canvas. Equivelent to the last NotedCanvasSizeCallback event.
 */
void noted_canvas_get_size_request(NotedCanvas *canvas, unsigned long *width, unsigned long *height);

/*
 * Redraw this region of the canvas. This should be called as a response to
 * NotedCanvasInvalidateCallback once the backend has initiated a redraw.
 */
void noted_canvas_draw(NotedCanvas *canvas, cairo_t *cr, NotedRect *rect);

/*
 * Call on a mouse event. State: 0 (mouse down), 1 (mouse drag), 2 (mouse up)
 */
void noted_canvas_mouse(NotedCanvas *canvas, int state, float x, float y, float pressure);

#endif /* notedcanvas_h */
