#include "notedcanvas.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void update_size_request(NotedCanvas *self);
static void * array_append(void **arr, void *element, unsigned long elementSize, unsigned long *size, unsigned long *capacity);
static inline int rects_intersect(NotedRect *a, NotedRect *b);
static inline void rect_expand_by_point(NotedRect *a, float x, float y);

typedef struct
{
    float x, y;
    float pressure;
} Point;

typedef struct
{
    float size;
    NotedRect bounds;
    unsigned long numPoints, maxPoints;
    Point *points;
} Stroke;

struct NotedCanvas_
{
    NotedCanvasSizeCallback sizeCallback;
    NotedCanvasInvalidateCallback invalidateCallback;
    void *callbackData;
    
    unsigned long x, y; // Offset from actual surface origin to apparent origin
    unsigned long width, height; // Size request
    
    unsigned long numStrokes, maxStrokes;
    Stroke *strokes;
    Stroke *currentStroke;
};



NotedCanvas * noted_canvas_new(NotedCanvasSizeCallback sizeCallback, NotedCanvasInvalidateCallback invalidateCallback, void *data)
{
    if(!sizeCallback || !invalidateCallback)
        return NULL;
    
    NotedCanvas *self = malloc(sizeof(NotedCanvas));
    memset(self, 0, sizeof(NotedCanvas));
    self->sizeCallback = sizeCallback;
    self->invalidateCallback = invalidateCallback;
    self->callbackData = data;
    
    return self;
}

void noted_canvas_destroy(NotedCanvas *self)
{
    for(unsigned long i=0;i<self->numStrokes;++i)
        free(self->strokes[i].points);
    free(self->strokes);
    free(self);
}

void noted_canvas_get_size_request(NotedCanvas *self, unsigned long *width, unsigned long *height)
{
    *width = self->width;
    *height = self->height;
}

void noted_canvas_draw(NotedCanvas *self, cairo_t *cr, NotedRect *rect)
{
    cairo_new_path(cr);
    for(unsigned long i=0;i<self->numStrokes;++i)
    {
        Stroke *s = &self->strokes[i];
        if(!rects_intersect(rect, &s->bounds))
            continue;
        for(unsigned long j=1;j<s->numPoints;++j)
        {
            Point *o = &self->strokes[i].points[j-1];
            Point *p = &self->strokes[i].points[j];

            cairo_move_to(cr, o->x, o->y);
            cairo_line_to(cr, p->x, p->y);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_width(cr, p->pressure*3);
            cairo_stroke(cr);
        }
    }
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
    }
    
    Point *p = array_append((void **)&s->points, NULL, sizeof(Point), &s->numPoints, &s->maxPoints);
    p->x = x;
    p->y = y;
    p->pressure = pressure;
    
    rect_expand_by_point(&s->bounds, p->x, p->y);
    
    if(state == 2)
        self->currentStroke = NULL;
    
    update_size_request(self);
    
    if(s->numPoints > 1)
    {
        float ox = s->points[s->numPoints-2].x;
        float oy = s->points[s->numPoints-2].y;
        float px = s->points[s->numPoints-1].x;
        float py = s->points[s->numPoints-1].y;
        float x = ((ox < px) ? ox : px) - 20;
        if(x < 0) x = 0;
        float y = ((oy < py) ? oy : py) - 20;
        if(y < 0) y = 0;
        float width = fabs(px-ox)+40;
        float height = fabs(py-oy)+40;
        NotedRect r = {x, y, x+width, y+height};
        self->invalidateCallback(self, &r, self->callbackData);
    }
}



static void update_size_request(NotedCanvas *self)
{
    unsigned long prevW = self->width;
    unsigned long prevH = self->height;
    self->width = 150;
    self->height = 150;
    if(self->numStrokes == 0)
    {
        self->sizeCallback(self, self->width, self->height, self->callbackData);
        return;
    }
    
    Stroke *s = &self->strokes[0];
    NotedRect rect = s->bounds;
    
    for(unsigned long i=1;i<self->numStrokes;++i)
    {
        Stroke *s = &self->strokes[i];
        rect_expand_by_point(&rect, s->bounds.x1, s->bounds.y1);
        rect_expand_by_point(&rect, s->bounds.x2, s->bounds.y2);
    }
    self->width += rect.x2;
    self->height += rect.y2;
    if(prevW != self->width || prevH != self->height)
        self->sizeCallback(self, self->width, self->height, self->callbackData);
}

static void * array_append(void **arr, void *element, unsigned long elementSize, unsigned long *size, unsigned long *capacity)
{
    if((*size + 1) > *capacity)
    {
        if(*capacity == 0)
            (*capacity) = 1;
        else
            (*capacity) *= 2;
        void *new = malloc(elementSize * (*capacity));
        if(*arr)
        {
            memcpy(new, *arr, elementSize * (*size));
            free(*arr);
        }
        (*arr) = new;
    }
    
    void *pos = *arr + ((*size) * elementSize);
    if(element)
        memcpy(pos, element, elementSize);
    else
        memset(pos, 0, elementSize);
    (*size) ++;
    return pos;
}

static inline int rects_intersect(NotedRect *a, NotedRect *b)
{
    return a->x1 < b->x2 && a->x2 > b->x1 && a->y1 < b->y2 && a->y2 > b->y1;
}

static inline void rect_expand_by_point(NotedRect *a, float x, float y)
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
