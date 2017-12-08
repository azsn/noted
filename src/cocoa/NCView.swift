/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * NCView.swift: macOS NSView wrapper for NotedCanvas.
 *   Used by Main.xib.
 */

import Cocoa

class NCView: NSView
{
    static var kPageWidth: CGFloat = 500
    var canvas: OpaquePointer?
    var currentTool: NCInputTool = kNCPenTool
    
    required init?(coder: NSCoder)
    {
        super.init(coder: coder)
    }
    
    deinit
    {
        if(self.canvas != nil)
        {
            noted_canvas_destroy(self.canvas)
        }
    }
    
    override var acceptsFirstResponder: Bool
    {
        return true
    }
    
    override var isFlipped: Bool
    {
        return true
    }

    // Takes ownership of canvas
    func setCanvas(canvas: OpaquePointer!)
    {
        if(self.canvas != nil)
        {
            noted_canvas_destroy(self.canvas)
        }
        
        self.canvas = canvas
        if(self.canvas == nil)
        {
            return
        }
        
        noted_canvas_set_invalidate_callback(self.canvas, invalidate_callback, UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque()))
        
        var style = NCStrokeStyle()
        style.a = 1
        style.r = 0
        style.g = 0
        style.b = 0
        style.thickness = Float(0.8 / NCView.kPageWidth)
        noted_canvas_set_stroke_style(canvas, style)
    }

    override func draw(_ rect: NSRect)
    {
        let context = NSGraphicsContext.current()?.cgContext
        
        if(self.canvas == nil || context == nil)
        {
            return
        }
        
        // Create suface
        // TODO: See if there's a way to not have to create a new
        // surface/context each draw. Doing so works, until the current
        // CGContext changes, which happens unpredictably, at which
        // point the surface breaks.
        let surface = cairo_quartz_surface_create_for_cg_context(context,
                                                                 UInt32(self.frame.size.width),
                                                                 UInt32(self.frame.size.height))
        if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        {
            return
        }
        
        // Create cairo context
        let cr = cairo_create(surface)
        if(cairo_status(cr) != CAIRO_STATUS_SUCCESS)
        {
            return
        }
        
        // Clip
        cairo_rectangle(cr, Double(rect.origin.x), Double(rect.origin.y), Double(rect.size.width), Double(rect.size.height))
        cairo_clip(cr)
        
        // Scale from canvas coordinates (width 0..1) to screen coordinates
        cairo_scale(cr, Double(self.frame.size.width), Double(self.frame.size.width))

        // Magnification controlled by the NSScrollView ancestor
        // Used by the canvas to know if it shouldn't bother drawing
        // beziers because they'd be too small to see on screen.
        let magnification = (self.superview?.superview as? NSScrollView)?.magnification ?? 1
        
        // Draw
        noted_canvas_draw(canvas, cr, Float(magnification))
        
        // Done
        cairo_destroy(cr)
        cairo_surface_destroy(surface)
    }
    
    let invalidate_callback : @convention(c) (OpaquePointer?, UnsafeMutablePointer<NCRect>?, UnsafeMutableRawPointer?) -> Void =
    {
        (c, rect, s) -> Void in

        let `self`: NCView = Unmanaged<NCView>.fromOpaque(s!).takeUnretainedValue()
        
        if(rect?.pointee == nil)
        {
            // Update properties
            let height = noted_canvas_get_height(self.canvas)
            self.setFrameSize(NSMakeSize(NCView.kPageWidth, CGFloat(height) * NCView.kPageWidth))
            self.setNeedsDisplay(self.frame)
        }
        else
        {
            // Redraw
            var nsr = NSMakeRect(CGFloat((rect?.pointee.x1)!),
                                 CGFloat((rect?.pointee.y1)!),
                                 CGFloat((rect?.pointee.x2)! - (rect?.pointee.x1)!),
                                 CGFloat((rect?.pointee.y2)! - (rect?.pointee.y1)!))
            
            nsr.origin.x *= self.frame.size.width
            nsr.origin.y *= self.frame.size.width
            nsr.size.width *= self.frame.size.width
            nsr.size.height *= self.frame.size.width
            
            self.setNeedsDisplay(nsr)
        }
    }
    
    override func mouseDown(with event: NSEvent)
    {
        if(self.canvas != nil)
        {
            var p = event.locationInWindow
            p = self.convert(p, from: nil)
            p.x /= self.frame.size.width
            p.y /= self.frame.size.width
            noted_canvas_input(self.canvas, kNCToolDown, self.currentTool, Float(p.x), Float(p.y), event.pressure)
        }
    }
    
    override func mouseDragged(with event: NSEvent)
    {
        if(self.canvas != nil)
        {
            var p = event.locationInWindow
            p = self.convert(p, from: nil)
            p.x /= self.frame.size.width
            p.y /= self.frame.size.width
            noted_canvas_input(self.canvas, kNCToolDrag, self.currentTool, Float(p.x), Float(p.y), event.pressure)
        }
    }
    
    override func mouseUp(with event: NSEvent)
    {
        if(self.canvas != nil)
        {
            var p = event.locationInWindow
            p = self.convert(p, from: nil)
            p.x /= self.frame.size.width
            p.y /= self.frame.size.width
            noted_canvas_input(self.canvas, kNCToolUp, self.currentTool, Float(p.x), Float(p.y), event.pressure)
//            [[self undoManager] registerUndoWithTarget:self selector:@selector(onUndo:) object:nil];
        }
    }
    
    override func tabletProximity(with event: NSEvent)
    {
        self.currentTool = (event.pointingDeviceType == NSPointingDeviceType.eraser) ? kNCEraserTool : kNCPenTool
    }
    
    func delete(_ sender: Any?)
    {
        Swift.print("delete")
    }
    
//    - (void)onRedo:(void *)v
//    {
//    if(self->canvas)
//    {
//    noted_canvas_redo(self->canvas);
//    [[self undoManager] registerUndoWithTarget:self selector:@selector(onUndo:) object:nil];
//    }
//    }
//    
//    - (void)onUndo:(void *)v
//    {
//    if(self->canvas)
//    {
//    noted_canvas_undo(self->canvas);
//    [[self undoManager] registerUndoWithTarget:self selector:@selector(onRedo:) object:nil];
//    }
//    }
}

// https://stackoverflow.com/a/28154936/161429
class CenteredClipView: NSClipView
{
    override func constrainBoundsRect(_ proposedBounds: NSRect) -> NSRect
    {
        var rect = super.constrainBoundsRect(proposedBounds)
        if let containerView = self.documentView
        {
            if (rect.size.width > containerView.frame.size.width)
            {
                rect.origin.x = (containerView.frame.width - rect.width) / 2
            }
            
            if(rect.size.height > containerView.frame.size.height)
            {
                rect.origin.y = (containerView.frame.height - rect.height) / 2
            }
        }
        
        return rect
    }
}

//    var surface: OpaquePointer?
//    var cr: OpaquePointer?
//    var surfaceSize: NSSize = NSSize()

//    func ensureSurface() -> Bool
//    {
//        // Don't bother if there isn't a canvas
//        if(self.canvas == nil)
//        {
//            return false
//        }
//
//        // Can't create a surface without a context
//        let context = NSGraphicsContext.current()?.cgContext
//        if(context == nil)
//        {
//            return false
//        }
//
//        var contextChanged = false
//
//        // If we already have a good surface,
//        // don't recreate it
//        if(self.surface != nil
//            && self.cr != nil
//            && __CGSizeEqualToSize(self.surfaceSize, self.frame.size))
//        {
//            // Check to see if the context that cairo has
//            // is the same as the current context. If not,
//            // the surface must be recreated.
//            if let cctx = cairo_quartz_surface_get_cg_context(self.surface)
//            {
//                if(cctx.toOpaque() == Unmanaged<CGContext>.passUnretained(context!).toOpaque())
//                {
//                    return true
//                }
//            }
//            contextChanged = true
//        }
//
//        // (Re)create surface
//
//        self.surfaceSize = self.frame.size
//
//        // Destroy any existing cairo things
//        if(self.cr != nil)
//        {
//            cairo_destroy(self.cr)
//            self.cr = nil
//        }
//        if(self.surface != nil)
//        {
////            if(contextChanged)
////            {
//                // When the context changes, it seems the old context's stack is
//                // emptied. This causes cairo to segfault when destroying the
//                // surface, since it tries to do a stack restore before releasing.
//                // So push a fake save to the stack.
//                if let cctx = cairo_quartz_surface_get_cg_context(self.surface)
//                {
//                    cctx.takeRetainedValue().saveGState()
//                }
////            }
//            cairo_surface_destroy(self.surface)
//            self.surface = nil
//        }
//
//        // Create
//
//        self.surface = cairo_quartz_surface_create_for_cg_context(context,
//                                                                  UInt32(self.frame.size.width),
//                                                                  UInt32(self.frame.size.height))
//        if(self.surface == nil)
//        {
//            self.surface = nil
//            return false
//        }
//        if(cairo_surface_status(self.surface) != CAIRO_STATUS_SUCCESS)
//        {
//            cairo_surface_destroy(self.surface)
//            self.surface = nil
//            return false
//        }
//
//        self.cr = cairo_create(self.surface)
//        if(self.cr == nil)
//        {
//            self.cr = nil
//            return false
//        }
//        if(cairo_status(self.cr) != CAIRO_STATUS_SUCCESS)
//        {
//            cairo_destroy(self.cr)
//            self.cr = nil
//            return false
//        }
//        return true;
//    }

