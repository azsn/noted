/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * notedcanvas.c: Main canvas object, where the drawing happens.
 *   OS-independent, wrapped by cocoa/nc-view.m and soon others.
 */

#ifndef notedcanvas_h
#define notedcanvas_h

#include <cairo/cairo.h>
#include <stdbool.h>

typedef struct NotedCanvas_ NotedCanvas;

typedef struct
{
    float x1, y1, x2, y2;
} NCRect;

typedef struct
{
    float r, g, b, a;
    float thickness;
} NCStrokeStyle;

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
 * Called when a region of the canvas has been invalidated
 * or other properties changed. If rect is non-null, the
 * rect region of the canvas should be redrawn. If rect is
 * null, other canvas properties may have updated and should
 * be checked.
 */
typedef void (*NCInvalidateCallback)(NotedCanvas *canvas, NCRect *rect, void *data);


/*
 * Create canvas.
 * See: noted_canvas_set_invalidate_callback
 */
NotedCanvas * noted_canvas_new();

/*
 * Destroy canvas.
 */
void noted_canvas_destroy(NotedCanvas *canvas);

/*
 * Change invalidate callback. Called when the canvas
 * should be redrawn or other properties have changed.
 */
void noted_canvas_set_invalidate_callback(NotedCanvas *canvas, NCInvalidateCallback invalidateCallback, void *data);

/*
 * Redraw the canvas. This should be called as a response to
 * NotedCanvasInvalidateCallback once the backend has initiated
 * a redraw. Use cairo_clip() to specify the region to redraw.
 */
void noted_canvas_draw(NotedCanvas *canvas, cairo_t *cr);

/*
 * Call on a mouse/pen/eraser event.
 * x should be in the [0, 1] range,
 * and y in the [0, height] range.
 */
void noted_canvas_input(NotedCanvas *canvas, NCInputState state, NCInputTool tool, float x, float y, float pressure);

/*
 * Gets the height of the canvas in units relative
 * to the width (which is always 1).
 */
float noted_canvas_get_height(NotedCanvas *canvas);

/*
 * Sets the style for future strokes or the current selection.
 */
void noted_canvas_set_stroke_style(NotedCanvas *canvas, NCStrokeStyle style);

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
 * 'where' specifies the vertical center of the location of pasting.
 * This should be the center of the current scroll location.
 */
//void noted_canvas_paste(NotedCanvas *canvas, void *content, float where);

/*
 * Undo. Returns true on success.
 */
bool noted_canvas_undo(NotedCanvas *canvas);

/*
 * Redo. Returns true on success.
 */
bool noted_canvas_redo(NotedCanvas *canvas);

#endif /* notedcanvas_h */
