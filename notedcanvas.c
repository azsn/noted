#include "notedcanvas.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

//static void append_stroke_point(NotedCanvas *self, float x, float y, float pressure);
static void update_size_request(NotedCanvas *self);
static void * array_append(void **arr, void *element, unsigned long elementSize, unsigned long *size, unsigned long *capacity);
static inline NotedRect * expand_rect(NotedRect *a, float amount);
static inline int rects_intersect(NotedRect *a, NotedRect *b);
static inline void rect_expand_by_point(NotedRect *a, float x, float y);
static inline float sq_dist(float x1, float y1, float x2, float y2);

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
    int smoothed;
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
    Stroke *currentSmooth;
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

// Matches bezier curves to the given points. This function takes
// only one dimension of the points at a time, so this function
// should be called twice: once to calculate x control points,
// again to calculate y.
// p1 and p2 are outputs, and should each be allocated with at
// least Klen-1 elements.
// Code converted from the SVG + JavaScript demo found here:
// https://www.particleincell.com/2012/bezier-splines/
void calculate_control_points(float *K, int Klen, float *p1, float *p2)
{
    int n = Klen-1;
    
    /*rhs vector*/
    float a[n], b[n], c[n], r[n];
    
    /*left most segment*/
    a[0]=0;
    b[0]=2;
    c[0]=1;
    r[0] = K[0]+2*K[1];
    
    /*internal segments*/
    for (int i = 1; i < n - 1; i++)
    {
        a[i]=1;
        b[i]=4;
        c[i]=1;
        r[i] = 4 * K[i] + 2 * K[i+1];
    }
    
    /*right segment*/
    a[n-1]=2;
    b[n-1]=7;
    c[n-1]=0;
    r[n-1] = 8*K[n-1]+K[n];
    
    /*solves Ax=b with the Thomas algorithm (from Wikipedia)*/
    for (int i = 1; i < n; i++)
    {
        float m = a[i]/b[i-1];
        b[i] = b[i] - m * c[i - 1];
        r[i] = r[i] - m*r[i-1];
    }
    
    p1[n-1] = r[n-1]/b[n-1];
    for (int i = n - 2; i >= 0; --i)
        p1[i] = (r[i] - c[i] * p1[i+1]) / b[i];
    
    /*we have p1, now compute p2*/
    for (int i=0;i<n-1;i++)
        p2[i]=2*K[i+1]-p1[i+1];
    
    p2[n-1]=0.5*(K[n]+p1[n-1]);
}

float perpDistance(float x0, float y0, float x1, float y1, float x2, float y2)
{
    float a = fabsf(((y2 - y1) * x0) - ((x2 - x1) * y0) + (x2 * y1) - (y2 * x1));
    return a / sqrt(sq_dist(x1, y1, x2, y2));
}

//// Ramer-Douglas-Peucker algorithm
//void remove_detail(float *x, float *y, int n, float *ox, float *oy, float epsilon)
//{
//    if(n <= 0)
//        return;
//    float dmax = 0;
//    int index = 0;
//    
//    for(int i = 1; i < n - 1; ++i)
//    {
//        float d = perpDistance(x[i], y[i], x[0], y[0], x[n - 1], y[n - 1]);
//        if(d > dmax)
//        {
//            index = i;
//            dmax = d;
//        }
//    }
//    
//    if(dmax > epsilon)
//    {
//        remove_detail(x, y, index + 1, ox, oy, epsilon);
//        remove_detail(x + index + 1, y + index + 1, n - index, ox + index + 1, oy + index  +1, epsilon);
//    }
//    else
//    {
//        ox[0] = x[0];
//        oy[0] = y[0];
//        ox[n - 1] = x[n - 1];
//        oy[n - 1] = y[n - 1];
//    }
//}

void noted_canvas_draw(NotedCanvas *self, cairo_t *cr, NotedRect *rect)
{
//    float inftest = NAN;
//    char *aa = &inftest;
////    memset(&inftest, 255, sizeof(float));
//    printf("%hhx.%hhx.%hhx.%hhx\n", aa[0], aa[1], aa[2], aa[3]);
    
    clock_t begin = clock();
    cairo_new_path(cr);
    for(unsigned long i=0;i<self->numStrokes;++i)
    {
        Stroke *s = &self->strokes[i];
        
        // TODO: Expand the rect by width of stroke, so that the intersection
        // calculation includes the outside edge of the stroke.
        // TODO: This still doesn't work quite right, especially when there
        // are a lot of strokes all in one area and drawing fast.
        NotedRect r = s->bounds;
        if(!rects_intersect(rect, expand_rect(&r, 5)))
            continue;
        
        if(s->numPoints <= 2)
        {
            for(unsigned long j=1;j<s->numPoints;++j)
            {
                Point *o = &self->strokes[i].points[j-1];
                Point *p = &self->strokes[i].points[j];
                
                cairo_move_to(cr, o->x, o->y);
                cairo_line_to(cr, p->x, p->y);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_set_line_width(cr, p->pressure*2);
                cairo_stroke(cr);
            }
            continue;
        }
        
        float x[s->numPoints], y[s->numPoints];
        float xp1[s->numPoints], yp1[s->numPoints], xp2[s->numPoints], yp2[s->numPoints];
        for(unsigned long i=0;i<s->numPoints;i++)
        {
            x[i] = s->points[i].x;
            y[i] = s->points[i].y;
        }
        
        int nan = 0;
        memset(&nan, 255, sizeof(int));
        
//        float ox[s->numPoints], oy[s->numPoints];
//        memset(ox, 255, s->numPoints*sizeof(float));
//        memset(oy, 255, s->numPoints*sizeof(float));
//        remove_detail(x, y, (int)s->numPoints, ox, oy, 1);
//        
//        float zx[s->numPoints], zy[s->numPoints];
//        int newnp = 0;
        
//        for(int i = 0; i < s->numPoints; ++i)
//        {
//            
//            if(d == nan)
//            {
//                printf("%f, %f\n", ox[i], oy[i]);
//            }
//        }
        
        calculate_control_points(x, (int)s->numPoints, xp1, xp2);
        calculate_control_points(y, (int)s->numPoints, yp1, yp2);
        
//        cairo_new_path(cr);
//        cairo_set_source_rgb(cr, 1, 0, 0);
//        cairo_move_to(cr, 0, 0);
//        for(int j=0;j<s->numPoints - 1;++j)
//        {
////            while(ox[j+1])
////            while(1)
////            {
////                float a = ox[j];
////                float *b = &a;
////                int *c = (int *)b;
////                int d = *c;
////                if(d == nan)
////                    j++;
////                else
////                    break;
////            }
//            
//            cairo_move_to(cr, x[j], y[j]);
//            
//            
//            
////            while(1)
////            {
////                float a = ox[j+1];
////                float *b = &a;
////                int *c = (int *)b;
////                int d = *c;
////                if(d == nan)
////                    j++;
////                else
////                    break;
////            }
////            printf("hi %f, %f\n", ox[j], oy[j]);
//            //            if(dsq < 40)
//            cairo_line_to(cr, x[j+1], y[j+1]);
//            //            else
//            //            cairo_curve_to(cr, xp1[j], yp1[j], xp2[j], yp2[j], x[j+1], y[j+1]);
//            
//            //            float speed = sqrt(dsq)/10.0;
//            //            if(speed < 1)
//            //                speed = 1;
//            //            printf("%f\n", speed);
//            
//            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
//            cairo_set_line_width(cr, 1);//p->pressure * 2);
////            cairo_stroke(cr);
//        }
        
        cairo_new_path(cr);
        
        if(s->smoothed)
            cairo_set_source_rgba(cr, 1, 0, 0, 1);
        else
            cairo_set_source_rgba(cr, 0,0,0,0.1);
        for(int j=0;j<s->numPoints-1;++j)
        {
            
            cairo_move_to(cr, x[j], y[j]);
            
//            if(dsq < 40)
//                cairo_line_to(cr, q->x, q->y);
//            else
                cairo_curve_to(cr, xp1[j], yp1[j], xp2[j], yp2[j], x[j+1], y[j+1]);
            
//            float speed = sqrt(dsq)/10.0;
//            if(speed < 1)
//                speed = 1;
//            printf("%f\n", speed);
            
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_width(cr, 1.3);//p->pressure * 2);
            cairo_stroke(cr);
        }
    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("time: %li, %fs\n", end-begin, time_spent);
}

void noted_canvas_mouse(NotedCanvas *self, int state, float x, float y, float pressure)
{
    Stroke *s = self->currentStroke;
    Stroke *ss = self->currentSmooth;
    
    float sx = x, sy = y;
    
    if(state == 0 || !s)
    {
        s = array_append((void **)&self->strokes, NULL, sizeof(Stroke), &self->numStrokes, &self->maxStrokes);
        self->currentStroke = s;
        s->bounds.x1 = s->bounds.x2 = x;
        s->bounds.y1 = s->bounds.y2 = y;
        s->smoothed = 0;
        
        ss = array_append((void **)&self->strokes, NULL, sizeof(Stroke), &self->numStrokes, &self->maxStrokes);
        self->currentSmooth = ss;
        ss->bounds.x1 = ss->bounds.x2 = x;
        ss->bounds.y1 = ss->bounds.y2 = y;
        ss->smoothed = 1;
    }
    else
    {
//        Point *p = &s->points[j];
//        Point *q = &s->points[j + 1];
//        float dsq = (q->y - p->y) * (q->y - p->y) + (q->x - p->x) * (q->x - p->x);
        
        Point *sp = &ss->points[ss->numPoints-1];
//        float dsq = sq_dist(x, y, p->x, p->y);
//        if(dsq < 5)
//        {
////            printf("remove\n");
//            return;
//        }
        
        sx = (sp->x + sx) / 2;
        sy = (sp->y + sy) / 2;
    }
    
    Point *p = array_append((void **)&s->points, NULL, sizeof(Point), &s->numPoints, &s->maxPoints);
    p->x = x;
    p->y = y;
    p->pressure = pressure;
    
    Point *ps = array_append((void **)&ss->points, NULL, sizeof(Point), &ss->numPoints, &ss->maxPoints);
    ps->x = sx;
    ps->y = sy;
    ps->pressure = pressure;
    
    rect_expand_by_point(&s->bounds, p->x, p->y);
    rect_expand_by_point(&ss->bounds, ps->x, ps->y);

    if(state == 2)
    {
        self->currentStroke = NULL;
        self->currentSmooth = NULL;
    }
    
    update_size_request(self);
    
    if(s->numPoints > 1)
    {
//        float ox = s->points[s->numPoints-2].x;
//        float oy = s->points[s->numPoints-2].y;
//        float px = s->points[s->numPoints-1].x;
//        float py = s->points[s->numPoints-1].y;
//        float x = ((ox < px) ? ox : px) - 20;
//        if(x < 0) x = 0;
//        float y = ((oy < py) ? oy : py) - 20;
//        if(y < 0) y = 0;
//        float width = fabs(px-ox)+40;
//        float height = fabs(py-oy)+40;
//        NotedRect r = {x, y, x+width, y+height};
        NotedRect r = s->bounds;
        expand_rect(&r, 5);
        self->invalidateCallback(self, &r, self->callbackData);
    }
}


//static void append_stroke_point(NotedCanvas *self, float x, float y, float pressure)
//{
//    Stroke *s = self->currentStroke;
//    
//    // Smooth fast jumps in mouse movement (usually do to lag or just
//    // the device not sending enough data) by adding lots of small steps
//    // and gradually fading the pressure.
//    // TODO: This helps a little bit, especially with the ends of strokes,
//    // but also causes more lag.
//    // TODO: This could be improved to take into account the current arc
//    // and interpolate where the mouse probably was, which would make
//    // really fast moving strokes look nicer.
//    if(0 && s->numPoints > 0)
//    {
//        Point *o = &s->points[s->numPoints-1];
//        float vpressure = (pressure - o->pressure);
//        float dist = sqrt(sq_dist(o->x, o->y, x, y));
//        int steps = dist;
//        
//        if(vpressure != 0 && steps > 1)
//        {
//            float vx = (x - o->x);
//            float vy = (y - o->y);
//            vx /= steps;
//            vy /= steps;
//            vpressure /= steps;
//            for(int i=0;i<steps;++i)
//            {
//                Point *p = array_append((void **)&s->points, NULL, sizeof(Point), &s->numPoints, &s->maxPoints);
//                Point *o = &s->points[s->numPoints-2];
//                p->x = o->x + vx;
//                p->y = o->y + vy;
//                rect_expand_by_point(&s->bounds, p->x, p->y);
//                p->pressure = o->pressure + vpressure;
//            }
//        }
//    }
//    
//    
//}

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

static inline NotedRect * expand_rect(NotedRect *a, float amount)
{
    a->x1 -= amount;
    a->x2 += amount;
    a->y1 -= amount;
    a->y2 += amount;
    return a;
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

static inline float sq_dist(float x1, float y1, float x2, float y2)
{
    return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
}
