#import "CairoDraw.h"

#include <cairo/cairo-quartz.h>
#include "notedcanvas.h"

@implementation CairoDraw
{
    NSSize surfaceSize;
    cairo_surface_t *surface;
    cairo_t *cr;
    NotedCanvas *canvas;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if(self)
    {
        self->canvas = noted_canvas_new(size_callback, invalidate_callback, (__bridge void *)(self));
    }
    return self;
}

-(void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    if(newWindow == nil)
    {
        if(self->cr)
            cairo_destroy(self->cr);
        self->cr = NULL;
        
        // Destroying the surface requires the context still be
        // around, which it apparently never is by this point.
        // TODO: Find a destructor-like function which is called
        // before the CGContext is gone.
        struct CGContext *context = [[NSGraphicsContext currentContext] graphicsPort];
        if(self->surface && context)
            cairo_surface_destroy(self->surface);
        self->surface = NULL;
    }
}

- (BOOL)isFlipped
{
    return YES;
}

//- (BOOL)isOpaque
//{
//    return YES;
//}

- (void)setFrameSize:(NSSize)rect
{
    unsigned long w, h;
    noted_canvas_get_size_request(self->canvas, &w, &h);
    if(rect.width < w)
        rect.width = w;
    if(rect.height < h)
        rect.height = h;
    
    [super setFrameSize:rect];
}

- (bool)ensureSurface
{
    NSSize size = [self frame].size;
    size.height = ceil(size.height);
    size.width = ceil(size.width);
    
    if(self->surface && self->cr && CGSizeEqualToSize(self->surfaceSize, size))
        return true;
    self->surfaceSize = size;
    
    if(self->cr)
        cairo_destroy(self->cr);
    self->cr = NULL;
    
    struct CGContext *context = [[NSGraphicsContext currentContext] graphicsPort];
    if(!context)
        return false;
    CGContextTranslateCTM (context, 0.0, size.height);
    CGContextScaleCTM (context, 1.0, -1.0);
    
    if(self->surface)
        cairo_surface_destroy(self->surface);
    self->surface = NULL;
    
    self->surface = cairo_quartz_surface_create_for_cg_context(context, size.width, size.height);
    self->cr = cairo_create(surface);
    return true;
}

- (void)drawRect:(NSRect)rect
{
    if(![self ensureSurface])
        return;
    NotedRect r = {rect.origin.x, rect.origin.y, rect.origin.x+rect.size.width, rect.origin.y+rect.size.height};
    noted_canvas_draw(self->canvas, self->cr, &r);
}

void size_callback(NotedCanvas *canvas, unsigned long width, unsigned long height, void *data)
{
    CairoDraw *self = (__bridge CairoDraw *)(data);
    [self setFrameSize:[self frame].size];
}

void invalidate_callback(NotedCanvas *canvas, NotedRect *r, void *data)
{
    CairoDraw *self = (__bridge CairoDraw *)(data);
    //[self setNeedsDisplay:true];
    [self setNeedsDisplayInRect:NSMakeRect(r->x1, r->y1, r->x2-r->x1, r->y2-r->y1)];
}



- (void)mouseDown:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_mouse(self->canvas, 0, p.x, p.y, [event pressure]);
}

-(void)mouseDragged:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_mouse(self->canvas, 1, p.x, p.y, [event pressure]);
}

- (void)mouseUp:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_mouse(self->canvas, 2, p.x, p.y, [event pressure]);
}

@end
