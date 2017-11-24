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
    var surface: OpaquePointer?
    var cr: OpaquePointer?
    var surfaceSize: NSSize = NSSize()
    var currentTool: NCInputTool = kNCPenTool
    
    required init?(coder: NSCoder)
    {
        super.init(coder: coder)
        
//        let c = noted_canvas_new()
//        
//        setCanvas(canvas: c!)
    }
    
    override var acceptsFirstResponder: Bool
    {
        return true
    }
    
    override var isFlipped: Bool
    {
        return true
    }

    func setCanvas(canvas: OpaquePointer)
    {
        self.canvas = canvas
        noted_canvas_set_invalidate_callback(self.canvas, invalidate_callback, UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque()))
        
        var style = NCStrokeStyle()
        style.a = 1
        style.r = 0
        style.g = 0
        style.b = 0
        style.thickness = Float(0.8 / NCView.kPageWidth)
        noted_canvas_set_stroke_style(canvas, style)
    }
    
    func ensureSurface() -> Bool
    {
        if(self.canvas == nil)
        {
            return false
        }
        
        var size = self.frame.size
        size.width = ceil(size.width)
        size.height = ceil(size.height)
        
        if(self.surface != nil && self.cr != nil && __CGSizeEqualToSize(self.surfaceSize, size))
        {
            return true
        }
        
        self.surfaceSize = size
        
        if(self.cr != nil)
        {
            cairo_destroy(self.cr)
            self.cr = nil
        }
        
        if(self.surface != nil)
        {
            cairo_surface_destroy(self.surface)
            self.surface = nil
        }
        
        let context = NSGraphicsContext.current()?.cgContext
        if(context == nil)
        {
            return false
        }
        
        self.surface = cairo_quartz_surface_create_for_cg_context(context, UInt32(size.width), UInt32(size.height))
        if(self.surface == nil || cairo_surface_status(self.surface) != CAIRO_STATUS_SUCCESS)
        {
            self.surface = nil
            return false
        }
        
        self.cr = cairo_create(self.surface)
        if(self.cr == nil || cairo_status(self.cr) != CAIRO_STATUS_SUCCESS)
        {
            self.cr = nil
            return false
        }
        
        Swift.print("recreated surface")
        return true;
    }
    
    override func draw(_ rect: NSRect)
    {
        if(!ensureSurface())
        {
            return
        }
        
        // Magnification controlled by the NSScrollView ancestor
        let magnification = (self.superview?.superview as? NSScrollView)?.magnification ?? 1
        
        cairo_save(self.cr)
        cairo_rectangle(self.cr, Double(rect.origin.x), Double(rect.origin.y), Double(rect.size.width), Double(rect.size.height))
        cairo_clip(self.cr)
        cairo_scale(self.cr, Double(self.frame.size.width), Double(self.frame.size.width))
        noted_canvas_draw(self.canvas, self.cr, Float(magnification))
        cairo_restore(self.cr)
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
