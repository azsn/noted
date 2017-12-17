//
//  NCColorSelectView.swift
//  noted
//
//  Created by Aidan Shafran on 12/16/17.
//  Copyright © 2017 zelbrium. All rights reserved.
//

import Cocoa

class NCColorSelectView: NSView
{
    var colorSelect : OpaquePointer?
    
    required init?(coder: NSCoder)
    {
        super.init(coder: coder)
        self.canDrawConcurrently = true

        self.colorSelect = nc_color_select_new()
        nc_color_select_set_width(self.colorSelect, 190, 30, 4)
    }
    
    override var isFlipped: Bool
    {
        return true
    }
    
    override func draw(_ rect: NSRect)
    {
        let context = NSGraphicsContext.current()?.cgContext
        
        if(self.colorSelect == nil || context == nil)
        {
            return
        }
        
        // Create suface
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
        
        // Draw
        nc_color_select_draw(self.colorSelect, cr)
        
        // Done
        cairo_destroy(cr)
        cairo_surface_destroy(surface)
    }
    
    func set_canvas(_ canvas: OpaquePointer)
    {
        nc_color_select_set_canvas(self.colorSelect, canvas)
    }
    
    override func mouseDown(with event: NSEvent)
    {
        if(self.colorSelect != nil)
        {
            var p = event.locationInWindow
            p = self.convert(p, from: nil)
            nc_color_select_input(self.colorSelect, kNCToolDown, Float(p.x), Float(p.y))
            self.display()
        }
    }
    
//    override func setFrameSize(_ newSize: NSSize)
//    {
//        let height = nc_color_select_set_width(self.colorSelect, Float(newSize.width), 30, 4)
//        var size = newSize
//        size.height = CGFloat(height)
//        super.setFrameSize(size)
//        self.setFrameOrigin(NSMakePoint(self.frame.origin.x, self.frame.origin.y + newSize.height - size.height))
//    }
}
