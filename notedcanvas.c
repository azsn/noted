#include "notedcanvas.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

static float kStrokeWidth = 1.3;
static float kMinBezierDistanceSq = 20;
static float kMinBezierAngleRad = 0.7;

static void calculate_control_points(float *p, int n, float *cp1, float *cp2);
static float angle_from_points(float ax, float ay, float bx, float by, float cx, float cy);
static void * array_append(void **arr, void *element, unsigned long elementSize, unsigned long *size, unsigned long *capacity);
static inline NCRect * expand_rect(NCRect *a, float amount);
static inline bool rects_intersect(NCRect *a, NCRect *b);
static inline void rect_expand_by_point(NCRect *a, float x, float y);
static inline float sq_dist(float x1, float y1, float x2, float y2);

typedef struct
{
    NCRect bounds;
    float *x; // Array of xs
    float *y; // Array of ys
    unsigned long numPoints, maxPoints;
    bool useBezier; // Set true if there are any points that are far away from their previous point
} Stroke;

struct NotedCanvas_
{
    NCInvalidateCallback invalidateCallback;
    void *callbackData;
    unsigned long npages;
    unsigned long numStrokes, maxStrokes;
    Stroke *strokes;
    Stroke *currentStroke;
};



NotedCanvas * noted_canvas_new(NCInvalidateCallback invalidateCallback, void *data)
{
    if(!invalidateCallback)
        return NULL;
    
    NotedCanvas *self = calloc(1, sizeof(NotedCanvas));
    self->invalidateCallback = invalidateCallback;
    self->callbackData = data;
    self->npages = 1;
    return self;
}

void noted_canvas_destroy(NotedCanvas *self)
{
    for(unsigned long i=0;i<self->numStrokes;++i)
    {
        free(self->strokes[i].x);
        free(self->strokes[i].y);
    }
    free(self->strokes);
    free(self);
}

void noted_canvas_draw(NotedCanvas *self, cairo_t *cr)
{
    NCRect clipRect;
    
    {
        double x1, y1, x2, y2;
        cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
        clipRect.x1 = x1;
        clipRect.y1 = y1;
        clipRect.x2 = x2;
        clipRect.y2 = y2;
    }
    
//    clock_t begin = clock();
    cairo_new_path(cr);
    for(unsigned long i = 0; i < self->numStrokes; ++i)
    {
        Stroke *s = &self->strokes[i];
        
        // Expand the rect by width of stroke, so that the intersection
        // calculation includes the outside edge of the stroke.
        NCRect r = s->bounds;
        if(!rects_intersect(&clipRect, expand_rect(&r, kStrokeWidth)))
            continue;
        
        if(s->useBezier && s->numPoints > 2) // Bezier algorithm needs at least 3 points
        {
            float xc1[s->numPoints], yc1[s->numPoints], xc2[s->numPoints], yc2[s->numPoints];
            calculate_control_points(s->x, (int)s->numPoints, xc1, xc2);
            calculate_control_points(s->y, (int)s->numPoints, yc1, yc2);
            
            cairo_new_path(cr);
            for(int j=0;j<s->numPoints-1;++j)
            {
                cairo_move_to(cr, s->x[j], s->y[j]);
                cairo_curve_to(cr, xc1[j], yc1[j], xc2[j], yc2[j], s->x[j+1], s->y[j+1]);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_set_line_width(cr, kStrokeWidth);
                cairo_stroke(cr);
            }
        }
        else
        {
            cairo_new_path(cr);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_width(cr, kStrokeWidth);
            cairo_move_to(cr, s->x[0], s->y[0]);
            for(unsigned long j = 1; j < s->numPoints; ++j)
                cairo_line_to(cr, s->x[j], s->y[j]);
            cairo_stroke(cr);
        }
    }
//    clock_t end = clock();
//    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("time: %li, %fs\n", end-begin, time_spent);
}

void noted_canvas_mouse(NotedCanvas *self, int state, float x, float y, float pressure)
{
    Stroke *s = self->currentStroke;
    
    if(state == 0 || !s)
    {
        s = array_append((void **)&self->strokes, NULL, sizeof(Stroke), &self->numStrokes, &self->maxStrokes);
        self->currentStroke = s;
        s->bounds.x1 = s->bounds.x2 = x;
        s->bounds.y1 = s->bounds.y2 = y;
        s->useBezier = false;
    }
    else
    {
        unsigned long i = s->numPoints - 1; // Previous point index

        // Stabilization
        // Probably could use a fancy algorithm for this,
        // but simply averaging the previous point with
        // this point actually helps a lot.
        x = (s->x[i] + x) / 2;
        y = (s->y[i] + y) / 2;
        
        // Use beziers when there is a long distance between points.
        float dsq = sq_dist(s->x[i], s->y[i], x, y);
        if(dsq > kMinBezierDistanceSq)
            s->useBezier = true;
        
        // Use beziers when there is a sharp turn in the line,
        // since those don't usually look good with regular lines.
        if(s->numPoints > 1 && !s->useBezier)
        {
            float a = angle_from_points(s->x[i - 1], s->y[i - 1],
                                      s->x[i], s->y[i],
                                      x, y);
            if(fabs(a) < kMinBezierAngleRad)
                s->useBezier = true;
        }
    }
    
    unsigned long pmax = s->maxPoints, psize = s->numPoints;
    array_append((void **)&s->x, &x, sizeof(float), &s->numPoints, &s->maxPoints);
    array_append((void **)&s->y, &y, sizeof(float), &psize, &pmax);
    
    rect_expand_by_point(&s->bounds, x, y);

    if(state == 2)
        self->currentStroke = NULL;
    
    if(s->numPoints > 1)
    {
        // Invalidate the rect containing the past few points
        // Past few points are needed, since beziers shift
        // slightly as they are fitted to the points. Also
        // helps regular lines, but I'm not totally sure why.
        NCRect r = {x, y, x, y}; // x == s->x[s->numPoints - 1]
        for(int i = 2; i <= 4 && s->numPoints >= i; ++i)
            rect_expand_by_point(&r, s->x[s->numPoints - i], s->y[s->numPoints - i]);
        // Plus a little extra for stroke width
        expand_rect(&r, kStrokeWidth);
        self->invalidateCallback(self, &r, self->npages, self->callbackData);
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

// https://stackoverflow.com/a/3487062/161429
static float angle_from_points(float ax, float ay, float bx, float by, float cx, float cy)
{
    float abx = bx - ax;
    float aby = by - ay;
    float cbx = bx - cx;
    float cby = by - cy;
    float dot = (abx * cbx + aby * cby); // dot product
    float cross = (abx * cby - aby * cbx); // cross product
    return atan2(cross, dot);
}

static void * array_append(void **arr, void *element, unsigned long elementSize, unsigned long *size, unsigned long *capacity)
{
    if((*size + 1) > *capacity)
    {
        if(*capacity == 0)
            (*capacity) = 16;
        else
            (*capacity) *= 2;
        *arr = realloc(*arr, elementSize * (*capacity));
    }
    
    void *pos = *arr + ((*size) * elementSize);
    if(element)
        memcpy(pos, element, elementSize);
    else
        memset(pos, 0, elementSize);
    (*size) ++;
    return pos;
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

static inline void rect_expand_by_point(NCRect *a, float x, float y)
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

static inline float sq_dist(float x1, float y1, float x2, float y2)
{
    return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
}
