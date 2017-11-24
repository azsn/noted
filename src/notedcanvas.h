/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * notedcanvas.c: Main canvas object, where the drawing happens.
 *   OS-independent, wrapped by cocoa/nc-view.m and soon others.
 */

#ifndef notedcanvas_h
#define notedcanvas_h

#include <stdlib.h>
#include <stdbool.h>
#include <cairo/cairo.h>


typedef struct NotedCanvas_ NotedCanvas;

typedef struct
{
    float x1, y1, x2, y2;
} NCRect;

typedef struct
{
    uint8_t r, g, b, a;
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

typedef enum
{
    kNCPageBlank,
    kNCPageRuled,
    kNCPageGrided,
} NCPagePattern;

/*
 * Called when a region of the canvas has been invalidated
 * or other properties changed. If rect is non-null, the
 * rect region of the canvas should be redrawn. If rect is
 * null, other canvas properties may have updated and should
 * be checked.
 */
typedef void (*NCInvalidateCallback)(NotedCanvas *canvas, NCRect *rect, void *data);


/*
 * Create a new blank canvas at the given path.
 */
NotedCanvas * noted_canvas_new(const char *path);

/*
 * Open a canvas from a file. Returns NULL on failure.
 */
NotedCanvas * noted_canvas_open(const char *path);

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
 *
 * If the controller applies some external magnification to the
 * rendering that is not accounted for by Cairo's transformation
 * matrix, use 'magnification' to account for that. It is optional,
 * and helps with optimizing rendering strokes. 1 is no magnification.
 */
void noted_canvas_draw(NotedCanvas *canvas, cairo_t *cr, float magnification);

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
 * Returns the number of pages on the canvas.
 */
size_t noted_canvas_get_n_pages(NotedCanvas *canvas);

/*
 * Gets the rect of the page at index. This rect can be passed
 * to noted_canvas_draw to get a full page draw.
 */
void noted_canvas_get_page_rect(NotedCanvas *canvas, size_t index, NCRect *rect);

/*
 * Sets the background pattern of a page. Density is how many
 * lines / grid cells per page.
 * TODO: Will probably be replaced by a more generic method
 * when PDF background support comes.
 */
void noted_canvas_set_page_pattern(NotedCanvas *canvas, size_t index, NCPagePattern pattern, unsigned int density);

/*
 * Moves page from index to targetIndex. targetIndex is specified
 * before the removal at index, so if there are pages A, B, C, D
 * at indices 0, 1, 2, 3, respectively, move_page(c, 0, 3) results
 * in the order B, C, D, A.
 */
void noted_canvas_move_page(NotedCanvas *canvas, size_t index, size_t targetIndex);

//void * noted_canvas_page_copy(NotedCanvas *canvas, size_t index);
//
//void * noted_canvas_page_cut(NotedCanvas *canvas, size_t index);
//
//void noted_canvas_page_paste(NotedCanvas *canvas, size_t index);

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
