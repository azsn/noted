#import "CairoDraw.h"
#include <cairo/cairo.h>
#include <cairo/cairo-quartz.h>

@implementation CairoDraw
{
    NSRect cairoRect;
    cairo_surface_t *surface;
    cairo_t *cr;
    bool drawing;
    NSPoint previousLocation;
    NSMutableArray *paths;
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if(self)
    {
        self->paths = [[NSMutableArray alloc] init];
        NSLog(@"init paths %@", self->paths);
//        [self becomeFirstResponder];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if(self)
    {
        self->paths = [[NSMutableArray alloc] init];
        NSLog(@"init paths %@", self->paths);
        //        [self becomeFirstResponder];
    }
    return self;
}
//
//- (BOOL)acceptsFirstResponder
//{
//    return YES;
//}
//
//- (BOOL)acceptsFirstMouse:(NSEvent *)event
//{
//    return YES;
//}

- (void)dealloc
{
    if(self->surface)
        cairo_destroy(self->cr);
    if(self->surface)
        cairo_surface_destroy(self->surface);
}

- (void)setFrameSize:(NSSize)rect
{
    [super setFrameSize:rect];
    NSLog(@"set frame size");
    
}

- (void)ensureCairo:(NSRect)rect
{
    
    if(self->surface && self->cr && CGSizeEqualToSize(self->cairoRect.size, rect.size))
        return;
//    self->currentPath = [[NSMutableArray alloc] init];
    NSLog(@"recreate");
    self->cairoRect = rect;
    
    struct CGContext *context = [[NSGraphicsContext currentContext] graphicsPort];
    CGContextTranslateCTM (context, 0.0, rect.size.height);
    CGContextScaleCTM (context, 1.0, -1.0);
    
    if(self->cr)
        cairo_destroy(self->cr);
    if(self->surface)
        cairo_surface_destroy(self->surface);
    
    self->surface = cairo_quartz_surface_create_for_cg_context(context, ceil(rect.size.width), ceil(rect.size.height));
    self->cr = cairo_create(surface);
}

- (void)drawRect:(NSRect)rect
{
    [self ensureCairo:rect];
    
//    cairo_new_path(self->cr);
    cairo_stroke_preserve(cr);
    cairo_path_t *p = cairo_copy_path(cr);
//    cairo_save(cr);
    cairo_identity_matrix(cr);
    for (NSValue *item in self->paths)
    {
//        NSLog(@"relaying path");
        cairo_path_t *path = [item pointerValue];
        cairo_new_path(self->cr);
        cairo_append_path(self->cr, path);
        cairo_stroke(self->cr);
    }
    
    cairo_new_path(self->cr);
    cairo_append_path(self->cr, p);
    cairo_stroke_preserve(self->cr);
//    cairo_restore(cr);
    //[self->currentPath removeObjectsInRange:NSMakeRange(0, [self->currentPath count]-1)];
    
//    cairo_set_source_rgb(cr, 1, 1, 1);
//    cairo_paint(cr);
    
    // do your drawings with cairo here
//    cairo_set_source_rgb(cr, 1, 0, 0);
////    cairo_new_path(cr);
//    cairo_rectangle(cr, 0, 0, 200, 200);
//    cairo_fill(cr);
//    cairo_paint(cr);
    
//    cairo_destroy(cr);
//    cairo_surface_destroy(surface);
}

- (void)mouseDown:(NSEvent *)event
{
//    [[self window] setAcceptsMouseMovedEvents:YES];
//    [[self window] makeFirstResponder:self];
//    self->drawing = TRUE;
//    //    [self->currentPath addObject:[NSValue valueWithPoint:[event locationInWindow]]];
    self->previousLocation = [event locationInWindow];
    cairo_new_path(cr);
    cairo_identity_matrix(cr);
    cairo_move_to(cr, self->previousLocation.x, self->previousLocation.y);
    NSLog(@"down");
}

-(void)mouseDragged:(NSEvent *)event
{
//    [self->currentPath addObject:[NSValue valueWithPoint:[event locationInWindow]]];
//    cairo_new_path(cr);
//    cairo_set_source_rgb(self->cr, 1, 0, 0);
//    cairo_move_to(cr, self->previousLocation.x, self->previousLocation.y);
    self->previousLocation = [event locationInWindow];
    cairo_line_to(cr, self->previousLocation.x, self->previousLocation.y);
    [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent *)event
{
    NSLog(@"up");
    cairo_path_t *path = cairo_copy_path(cr);
    [self->paths addObject:[NSValue valueWithPointer:path]];
//    NSLog(@"paths %@, %lui", self->paths,(unsigned long) [self->paths count]);

}

@end
