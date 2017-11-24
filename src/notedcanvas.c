/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * notedcanvas.c: Main canvas object, where the drawing happens.
 *   OS-independent, wrapped by cocoa/NCView.swift and soon others.
 */

#include "nc-private.h"
#include "array.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

static void pen_input(NotedCanvas *self, NCInputState state, float x, float y, float pressure);
static void eraser_input(NotedCanvas *self, NCInputState state, float x, float y, float pressure);
static void draw_page(cairo_t *cr, Page *p);
static void draw_stroke(cairo_t *cr, Stroke *s, float magnification);
static void append_page(NotedCanvas *self);
static void calculate_control_points(float *p, int n, float *cp1, float *cp2);
static void clear_redos(NotedCanvas *self);

static inline NCRect * expand_rect(NCRect *a, float amount);
static inline bool rects_intersect(NCRect *a, NCRect *b);
static inline bool point_in_rect(NCRect *r, float x, float y);
extern inline void rect_expand_by_point(NCRect *a, float x, float y);
extern inline float sq_dist(float x1, float y1, float x2, float y2);


NotedCanvas * noted_canvas_new(const char *path)
{
    NotedCanvas *self = calloc(1, sizeof(NotedCanvas));
    self->path = strdup(path);
    self->pages = array_new(sizeof(Page), (FreeNotify)free_page);
    append_page(self);
    return self;
}

void noted_canvas_destroy(NotedCanvas *self)
{
    if(self->path)
        free(self->path);
    array_free(self->pages);
    free(self);
}

void noted_canvas_set_invalidate_callback(NotedCanvas *self, NCInvalidateCallback invalidateCallback, void *data)
{
    self->invalidateCallback = invalidateCallback;
    self->callbackData = data;
    
    if(self->invalidateCallback)
    {
        self->invalidateCallback(self, NULL, self->callbackData);
        NCRect x = {0, 0, 1, noted_canvas_get_height(self)};
        self->invalidateCallback(self, &x, self->callbackData);
    }
}

void noted_canvas_draw(NotedCanvas *self, cairo_t *cr, float magnification)
{
    NCRect clipRect, relClipRect;
    
    {
        double x1, y1, x2, y2;
        cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
        clipRect.x1 = x1;
        clipRect.y1 = y1;
        clipRect.x2 = x2;
        clipRect.y2 = y2;
    }
    
    size_t npages = array_size(self->pages);
    for(size_t i = 0; i < npages; ++i)
    {
        Page *p = &self->pages[i];
        
        if(!rects_intersect(&clipRect, &p->bounds))
            continue;
        
        draw_page(cr, p);
    }
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for(size_t i = 0; i < npages; ++i)
    {
        Page *p = &self->pages[i];
        
        relClipRect.x1 = clipRect.x1 - p->bounds.x1;
        relClipRect.y1 = clipRect.y1 - p->bounds.y1;
        relClipRect.x2 = clipRect.x2 - p->bounds.x1;
        relClipRect.y2 = clipRect.y2 - p->bounds.y1;
        
        cairo_save(cr);
        cairo_translate(cr, p->bounds.x1, p->bounds.y1);
        
        size_t nstrokes = array_size(p->strokes);
        for(unsigned long j = 0; j < nstrokes; ++j)
        {
            Stroke *s = &p->strokes[j];
            
            // Expand the rect by width of stroke, so that the intersection
            // calculation includes the outside edge of the stroke.
            NCRect r = s->bounds;
            if(!rects_intersect(&relClipRect, expand_rect(&r, s->style.thickness)))
                continue;
            
            cairo_set_line_width(cr, s->style.thickness);
            cairo_set_source_rgba(cr, s->style.r, s->style.g, s->style.b, s->style.a);
            draw_stroke(cr, s, magnification);
        }
        
        cairo_restore(cr);
    }
}

void noted_canvas_input(NotedCanvas *self, NCInputState state, NCInputTool tool, float x, float y, float pressure)
{
    switch(tool)
    {
        case kNCPenTool:
            pen_input(self, state, x, y, pressure);
            break;
        case kNCEraserTool:
            eraser_input(self, state, x, y, pressure);
            break;
        case kNCSelectTool:
            // TODO
            printf("select tool not implemented\n");
            break;
    }
    
    if(state == kNCToolUp && self->path)
    {
        printf("save to %s\n", self->path);
        if(!noted_canvas_save(self, self->path))
        {
            printf("error saving to %s\n", self->path);
        }
    }
}

float noted_canvas_get_height(NotedCanvas *self)
{
    if(array_size(self->pages) == 0)
        return 0;
    return self->pages[array_size(self->pages) - 1].bounds.y2;
}

void noted_canvas_set_stroke_style(NotedCanvas *self, NCStrokeStyle style)
{
    self->currentStyle = style;
}

size_t noted_canvas_get_n_pages(NotedCanvas *self)
{
    return array_size(self->pages);
}

void noted_canvas_get_page_rect(NotedCanvas *self, size_t index, NCRect *rect)
{
    *rect = self->pages[index].bounds;
}

void noted_canvas_set_page_pattern(NotedCanvas *self, size_t index, NCPagePattern pattern, unsigned int density)
{
    self->pages[index].pattern = pattern;
    self->pages[index].density = density;
}

void noted_canvas_move_page(NotedCanvas *self, size_t index, size_t targetIndex)
{
//    Page p = self->pages[index];
}

// TODO: Undo doesn't work with erasing.
bool noted_canvas_undo(NotedCanvas *self)
{
//    if(self->lastStroke <= 0 || self->currentStroke != NULL)
//        return false;
//
//    NCRect r = self->strokes[self->lastStroke - 1].bounds;
//    expand_rect(&r, kStrokeWidth);
//    --self->lastStroke;
//    if(self->invalidateCallback)
//        self->invalidateCallback(self, &r, self->callbackData);
    return true;
}

bool noted_canvas_redo(NotedCanvas *self)
{
//    if(self->lastStroke >= self->numStrokes || self->currentStroke != NULL)
//        return false;
//    
//    ++self->lastStroke;
//    NCRect r = self->strokes[self->lastStroke - 1].bounds;
//    expand_rect(&r, kStrokeWidth);
//    if(self->invalidateCallback)
//        self->invalidateCallback(self, &r, self->callbackData);
    return true;
}

static void pen_input(NotedCanvas *self, NCInputState state, float x, float y, float pressure)
{
    Stroke *s = self->currentStroke;
        
    if(state == kNCToolDown)
    {
        // Start new stroke
        
        clear_redos(self);
        
        Page *p = NULL;
        size_t i;
        for(i = 0; i < array_size(self->pages); ++i)
        {
            if(point_in_rect(&self->pages[i].bounds, x, y))
            {
                p = &self->pages[i];
                break;
            }
        }
        
        if(!p)
            return;
        
        x -= p->bounds.x1;
        y -= p->bounds.y1;
        
        Stroke new = {
            .bounds = {x, y, x, y},
            .maxDistSq = 0,
            .page = p,
            .style = self->currentStyle,
            .x = array_new(sizeof(float), NULL),
            .y = array_new(sizeof(float), NULL),
        };
        p->strokes = array_append(p->strokes, &new);
        self->currentStroke = s = &p->strokes[array_size(p->strokes) - 1];
        
        // If this is a stroke on the last page, add a new page
        if(i == array_size(self->pages) - 1)
            append_page(self);
    }
    else
    {
        // Append to stroke
        
        if(!s)
            return;
        
        unsigned long i = array_size(s->x) - 1; // Previous point index
        
        x -= s->page->bounds.x1;
        y -= s->page->bounds.y1;
        
        // Stabilization
        // Probably could use a fancy algorithm for this,
        // but simply averaging the previous point with
        // this point actually helps a lot.
        x = (s->x[i] + x) / 2;
        y = (s->y[i] + y) / 2;
        
        // Use beziers when there is a long distance between points.
        float dsq = sq_dist(s->x[i], s->y[i], x, y);
        if(dsq > s->maxDistSq)
            s->maxDistSq = dsq;
    }
    
    s->x = array_append(s->x, &x);
    s->y = array_append(s->y, &y);
    
    rect_expand_by_point(&s->bounds, x, y);
    
    if(state == kNCToolUp)
        self->currentStroke = NULL;
    
    size_t npoints = array_size(s->x);
    if(npoints > 1)
    {
        // Invalidate the rect containing the past few points
        // Past few points are needed, since beziers shift
        // slightly as they are fitted to the points. Also
        // helps regular lines, but I'm not totally sure why.
        NCRect r = {x, y, x, y}; // x == s->x[s->numPoints - 1]
        for(int i = 2; i <= 4 && npoints >= i; ++i)
            rect_expand_by_point(&r, s->x[npoints - i], s->y[npoints - i]);
        
        r.x1 += s->page->bounds.x1;
        r.y1 += s->page->bounds.y1;
        r.x2 += s->page->bounds.x1;
        r.y2 += s->page->bounds.y1;
        
        // Plus a little extra for stroke width
        expand_rect(&r, s->style.thickness);
        if(self->invalidateCallback)
            self->invalidateCallback(self, &r, self->callbackData);
    }
}

// Erasing uses alpha overlap to detect when the user has
// "drawn" an erasing line over a stroke. To prevent skipping
// over strokes when the eraser is moving fast, the coordinate
// coordinate is saved after each input and used in the next
// input to draw an erase line between the two.
static void eraser_input(NotedCanvas *self, NCInputState state, float x, float y, float pressure)
{
    if(state == kNCToolDown)
    {
        self->eraserPrevX = NAN;
        self->eraserPrevY = NAN;
    }

    float eraserThickness = self->currentStyle.thickness;
    
    // Create a rect which bounds the erasing path.
    // This includes the current erasing point, and
    // the line from it to the previous erasing point
    // if one exists.
    NCRect eraserRect = {x, y, x, y};
    float eraserToX = x, eraserToY = y;
    if(!isnan(self->eraserPrevX) && !isnan(self->eraserPrevY))
    {
        rect_expand_by_point(&eraserRect, self->eraserPrevX, self->eraserPrevY);
        eraserToX = self->eraserPrevX;
        eraserToY = self->eraserPrevY;
    }
    expand_rect(&eraserRect, eraserThickness / 2.0);
    
    // Since the stroke coordinates are in [0,1] range, use
    // 2 / kEraserWidth as a scale factor to make them pixelizable
    int srwidth = ceil((eraserRect.x2 - eraserRect.x1) * 2 / eraserThickness);
    int srheight = ceil((eraserRect.y2 - eraserRect.y1) * 2 / eraserThickness);
    
    // Weird issues with 1 width or height
    if(srwidth < 2) srwidth = 2;
    if(srheight < 2) srheight = 2;
    
    // Don't create a surface until we know there is
    // at least one stroke to check for erasing.
    cairo_surface_t *sr = NULL;
    cairo_t *cr = NULL;
    unsigned char *data = NULL;
    unsigned long datalen = 0;
    
    size_t npages = array_size(self->pages);
    for(size_t i = 0; i < npages; ++i)
    {
        Page *p = &self->pages[i];

        size_t nstrokes = array_size(p->strokes);
        for(unsigned long j = 0; j < nstrokes; ++j)
        {
            Stroke *s = &p->strokes[j];
            
            NCRect r = s->bounds;
            r.x1 += p->bounds.x1;
            r.y1 += p->bounds.y1;
            r.x2 += p->bounds.x1;
            r.y2 += p->bounds.y1;
            
            // Ignore stroke if it isn't in the eraser rect
            if(!rects_intersect(&eraserRect, expand_rect(&r, s->style.thickness)))
                continue;
            
            if(!sr)
            {
                // Create a memory surface the size of the eraser line
                sr = cairo_image_surface_create(CAIRO_FORMAT_A8, srwidth, srheight);
                if(!sr || cairo_surface_status(sr) != CAIRO_STATUS_SUCCESS)
                    return;
                cr = cairo_create(sr);
                if(!cr || cairo_status(cr) != CAIRO_STATUS_SUCCESS)
                    return;
                cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE); // No need for quality
                cairo_set_operator(cr, CAIRO_OPERATOR_ADD); // Easier testing of overlap
                
                // Translate painting region to where the surface is
                cairo_scale(cr, 2 / eraserThickness, 2 / eraserThickness);
                cairo_translate(cr, -eraserRect.x1, -eraserRect.y1);

                data = cairo_image_surface_get_data(sr);
                datalen = cairo_image_surface_get_stride(sr) * srheight;
            }
            
            // Clear the surface
            memset(data, 0, datalen);
            cairo_surface_mark_dirty(sr);
            cairo_save(cr);
            
            // Draw eraser, half alpha
            cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_width(cr, eraserThickness);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, eraserToX, eraserToY);
            cairo_stroke(cr);
            
            // Draw the stroke, offset by the position of the eraser
            // Half alpha
            cairo_translate(cr, p->bounds.x1, p->bounds.y1);
            cairo_set_line_width(cr, s->style.thickness);
            draw_stroke(cr, s, 1);
            
            cairo_restore(cr);
            cairo_surface_flush(sr);
            
//            for(unsigned long k = 0; k < srheight; ++k)
//            {
//                for(unsigned long l = 0; l < cairo_image_surface_get_stride(sr); ++l)
//                    printf("%x ", data[k * cairo_image_surface_get_stride(sr) + l]);
//                printf("\n");
//            }
//            printf("\n");
            
            // If there is any overlap in alpha (pixel is 0xFF), the
            // eraser hit the stroke and the stroke should be removed.
            for(unsigned long k = 0; k < datalen; ++k)
            {
                if(data[k] == 0xFF)
                {
                    clear_redos(self);
                    array_remove(p->strokes, j, true);
                    if(self->invalidateCallback)
                        self->invalidateCallback(self, &r, self->callbackData);
                    --j;
                    --nstrokes;
                    break;
                }
            }
        }
    }
    
    if(state == kNCToolUp)
    {
        self->eraserPrevX = NAN;
        self->eraserPrevY = NAN;
    }
    else
    {
        self->eraserPrevX = x;
        self->eraserPrevY = y;
    }
    if(cr)
        cairo_destroy(cr);
    if(sr)
        cairo_surface_destroy(sr);
}

static void draw_page(cairo_t *cr, Page *p)
{
    // Clear background
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_rectangle(cr, p->bounds.x1, p->bounds.y1, p->bounds.x2 - p->bounds.x1, p->bounds.y2 - p->bounds.y1);
    cairo_fill(cr);
    
    // Background pattern
    if(p->pattern == kNCPageBlank)
        return;
    
    static const float kHBorderPad = 1.f/24.f;
    static const float kVBorderPad = 1.f/14.f;
    
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.1);
    cairo_set_line_width(cr, 1.f/600.f);
    
    float w = p->bounds.x2 - p->bounds.x1 - kHBorderPad - kHBorderPad;
    float h = p->bounds.y2 - p->bounds.y1 - kVBorderPad - kVBorderPad;
    
    switch(p->pattern)
    {
        case kNCPageGrided:
        {
            float size = (1.f / p->density) * w;
            unsigned int vdensity = floor((h / w) * p->density);
            h = size * vdensity;
            float vpad = ((p->bounds.y2 - p->bounds.y1) - h) / 2;
            
            // Draw vertical lines at user's density
            for(int i = 0; i <= p->density; ++i)
            {
                float f = size * i;
                cairo_move_to(cr, p->bounds.x1 + kHBorderPad + f, p->bounds.y1 + vpad);
                cairo_line_to(cr, p->bounds.x1 + kHBorderPad + f, p->bounds.y2 - vpad);
            }
            
            // Draw horizontal lines such that they're exactly the same
            // size as the vertical gaps, but more of them to fill h.
            for(int i = 0; i <= vdensity; ++i)
            {
                float f = size * i;
                cairo_move_to(cr, p->bounds.x1 + kHBorderPad, p->bounds.y1 + vpad + f);
                cairo_line_to(cr, p->bounds.x2 - kHBorderPad, p->bounds.y1 + vpad + f);
            }
            
            cairo_stroke(cr);
            break;
        }
        
        case kNCPageRuled:
        {
            float size = (1.f / p->density) * h;
            for(int i = 0; i <= p->density; ++i)
            {
                float f = size * i;
                cairo_move_to(cr, p->bounds.x1 + kHBorderPad, p->bounds.y1 + kVBorderPad + f);
                cairo_line_to(cr, p->bounds.x2 - kHBorderPad, p->bounds.y1 + kVBorderPad + f);
            }
            
            cairo_stroke(cr);
            break;
        }
            
        case kNCPageBlank:
            break;
    }
}

// Draws in page-relative coordinates, so a call to
// cairo_translate before this might be useful.
static void draw_stroke(cairo_t *cr, Stroke *s, float magnification)
{
    static const double kMinBezierDist = 2.0; // "device coordinates" (pixels)

    cairo_new_path(cr);
    cairo_move_to(cr, s->x[0], s->y[0]);
    
    // If the stroke is very compressed on screen,
    // it's not important to render it with actual curves.
    double maxDist = sqrt(s->maxDistSq), _ = 0;
    cairo_user_to_device_distance(cr, &maxDist, &_);
    maxDist *= magnification;
    
    size_t npoints = array_size(s->x);
    if(maxDist > kMinBezierDist && npoints > 2) // Bezier algorithm needs at least 3 points
    {
        float xc1[npoints], yc1[npoints], xc2[npoints], yc2[npoints];
        calculate_control_points(s->x, (int)npoints, xc1, xc2);
        calculate_control_points(s->y, (int)npoints, yc1, yc2);
        
        for(int j = 0; j < npoints - 1; ++j)
            cairo_curve_to(cr, xc1[j], yc1[j], xc2[j], yc2[j], s->x[j+1], s->y[j+1]);
    }
    else
    {
        for(unsigned long j = 1; j < npoints; ++j)
            cairo_line_to(cr, s->x[j], s->y[j]);
    }
    
    cairo_stroke(cr);
}


static void append_page(NotedCanvas *self)
{
    // TODO: Temporary. Should default to blank.
    NCPagePattern pattern = kNCPageGrided;
    unsigned int density = 24;
    
    NCRect b = {0};
    if(array_size(self->pages) > 0)
    {
        Page *prev = &self->pages[array_size(self->pages) - 1];
        b = prev->bounds;
        b.y2 += 0.01;
        
        // Copy pattern from last page
        pattern = prev->pattern;
        density = prev->density;
    }
    
    Page p = {
        .bounds = {0, b.y2, 1, b.y2 + 11/8.5f},
        .density = density,
        .pattern = pattern,
        .strokes = array_new(sizeof(Stroke), (FreeNotify)free_stroke)
    };
    self->pages = array_append(self->pages, &p);
    
    if(self->invalidateCallback)
    {
        self->invalidateCallback(self, NULL, self->callbackData);
        self->invalidateCallback(self, &p.bounds, self->callbackData);
    }
}


// Matches bezier curves to the given points. This function takes
// only one dimension of the points at a time, so this function
// should be called twice: once to calculate x control points,
// again to calculate y. cp1 and cp2 are outputs, and should each
// be allocated with at least n-1 elements.
// Code converted from the SVG + JavaScript demo found here:
// https://www.particleincell.com/2012/bezier-splines/
static void calculate_control_points(float *p, int n, float *cp1, float *cp2)
{
    --n;
    
    // rhs vector
    float a[n], b[n], c[n], r[n];
    
    // left most segment
    a[0] = 0;
    b[0] = 2;
    c[0] = 1;
    r[0] = p[0] + (2 * p[1]);
    
    // internal segments
    for (int i = 1; i < n - 1; ++i)
    {
        a[i] = 1;
        b[i] = 4;
        c[i] = 1;
        r[i] = 4 * p[i] + 2 * p[i+1];
    }
    
    // right segment
    a[n-1] = 2;
    b[n-1] = 7;
    c[n-1] = 0;
    r[n-1] = 8 * p[n-1] + p[n];
    
    // solves Ax=b with the Thomas algorithm (from Wikipedia)
    for (int i = 1; i < n; ++i)
    {
        float m = a[i] / b[i-1];
        b[i] = b[i] - m * c[i - 1];
        r[i] = r[i] - m * r[i-1];
    }
    
    cp1[n-1] = r[n-1] / b[n-1];
    for (int i = n - 2; i >= 0; --i)
        cp1[i] = (r[i] - c[i] * cp1[i+1]) / b[i];
    
    // we have cp1, now compute cp2
    for (int i = 0; i < n-1; i++)
        cp2[i] = (2 * p[i+1]) - cp1[i+1];
    
    cp2[n-1] = 0.5 * (p[n] + cp1[n-1]);
}

static void clear_redos(NotedCanvas *self)
{
//    for(unsigned long i = self->lastStroke; i < self->numStrokes; ++i)
//        free_stroke(&self->strokes[i]);
//    self->numStrokes = self->lastStroke;
}

void free_stroke(Stroke *s)
{
    array_free(s->x);
    array_free(s->y);
}

void free_page(Page *p)
{
    array_free(p->strokes);
}

static inline NCRect * expand_rect(NCRect *a, float amount)
{
    a->x1 -= amount;
    a->x2 += amount;
    a->y1 -= amount;
    a->y2 += amount;
    return a;
}

static inline bool rects_intersect(NCRect *a, NCRect *b)
{
    return a->x1 < b->x2 && a->x2 > b->x1 && a->y1 < b->y2 && a->y2 > b->y1;
}

static inline bool point_in_rect(NCRect *r, float x, float y)
{
    return x > r->x1 && x < r->x2 && y > r->y1 && y < r->y2;
}


//    clock_t begin = clock();
//    clock_t end = clock();
//    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("time: %li, %fs\n", end-begin, time_spent);
