#ifndef notedcanvas_h
#define notedcanvas_h

#include <stdio.h>
#include <cairo/cairo.h>
#include <stdbool.h>

typedef struct NotedCanvas_ NotedCanvas;

typedef struct
{
    float x1, y1, x2, y2;
} NCRect;

typedef enum
{
    kNCToolDown,
    kNCToolUp,
    kNCToolDrag,
} NCInputState;

typedef enum
{
    kNCPenTool,
    kNCEraserTool,
    kNCSelectTool,
} NCInputTool;

/*
 * Called when a region of the canvas has been invalidated (should
 * be redrawn). rect may be NULL if all that changed is the number
 * of pages.
 */
typedef void (*NCInvalidateCallback)(NotedCanvas *canvas, NCRect *rect, unsigned long npages, void *data);


/*
 * Create canvas.
 * pageHeight is in the same units used for mouse x,y and
 * draw coordinates. These are not necessarily screen units.
 */
NotedCanvas * noted_canvas_new(NCInvalidateCallback invalidateCallback, float pageHeight, void *data);

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
 * Call on a mouse/pen/eraser event.
 */
void noted_canvas_input(NotedCanvas *canvas, NCInputState state, NCInputTool tool, float x, float y, float pressure);

/*
 * Sets the color for future strokes or the current selection.
 */
//void noted_canvas_set_color(NotedCanvas *canvas, float r, float g, float b, float a);

/*
 * Sets the thickness for future strokes or the current selection.
 */
//void noted_canvas_set_thickness(NotedCanvas *canvas, float thickness);

/*
 * Clears selection
 */
//void noted_canvas_clear_selection(NotedCanvas *canvas);

/*
 * Deletes content within current selection.
 */
//void noted_canvas_delete_selection(NotedCanvas *canvas);

/*
 * Copies content in current selection.
 * Cut by doing noted_canvas_copy followed
 * by noted_canvas_delete_selection.
 */
//void * noted_canvas_copy(NotedCanvas *canvas);

/*
 * Pastes a content cut/copy (possibly from another canvas).
 */
//void noted_canvas_paste(NotedCanvas *canvas, void *content);

/*
 * Undo. Returns true on success.
 */
bool noted_canvas_undo(NotedCanvas *canvas);

/*
 * Redo. Returns true on success.
 */
bool noted_canvas_redo(NotedCanvas *canvas);

#endif /* notedcanvas_h */
