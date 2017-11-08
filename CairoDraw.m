#import "CairoDraw.h"

#include <cairo/cairo-quartz.h>
#include "notedcanvas.h"

static float kPageSize = 500.0;

@implementation CairoDraw
{
    NSSize surfaceSize;
    cairo_surface_t *surface;
    cairo_t *cr;
    NotedCanvas *canvas;
    unsigned long npages;
    NCInputTool currentTool;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if(self)
    {
        self->canvas = noted_canvas_new(invalidate_callback, kPageSize, (__bridge void *)(self));
        self->npages = 0;
        self->currentTool = kNCPenTool;
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

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)isFlipped
{
    return YES;
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
    cairo_save(cr);
    cairo_rectangle(self->cr, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    cairo_clip(self->cr);
    noted_canvas_draw(self->canvas, self->cr);
    cairo_restore(cr);
}

void invalidate_callback(NotedCanvas *canvas, NCRect *r, unsigned long npages, void *data)
{
    CairoDraw *self = (__bridge CairoDraw *)(data);
    //[self setNeedsDisplay:true];
    [self setNeedsDisplayInRect:NSMakeRect(r->x1, r->y1, r->x2-r->x1, r->y2-r->y1)];
}

- (void)mouseDown:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_input(self->canvas, kNCToolDown, self->currentTool, p.x, p.y, [event pressure]);
    [super mouseDown: event];
}

-(void)mouseDragged:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_input(self->canvas, kNCToolDrag, self->currentTool, p.x, p.y, [event pressure]);
    [super mouseDragged:event];
}

- (void)mouseUp:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    p = [self convertPoint:p fromView:nil];
    noted_canvas_input(self->canvas, kNCToolUp, self->currentTool, p.x, p.y, [event pressure]);
    [[self undoManager] registerUndoWithTarget:self selector:@selector(onUndo:) object:nil];
    [super mouseUp:event];
}

-(void)tabletProximity:(NSEvent *)event
{
    self->currentTool = ([event pointingDeviceType] == NSPointingDeviceTypeEraser) ? kNCEraserTool : kNCPenTool;
}

- (void)onRedo:(void *)v
{
    noted_canvas_redo(self->canvas);
    [[self undoManager] registerUndoWithTarget:self selector:@selector(onUndo:) object:nil];
}

- (void)onUndo:(void *)v
{
    noted_canvas_undo(self->canvas);
    [[self undoManager] registerUndoWithTarget:self selector:@selector(onRedo:) object:nil];
}


@end
