/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * AppDelegate.swift: macOS main file, handles file loading and other UI
 */

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate
{
    static var noteDirectory: NoteFile! = NoteFile()

    @IBOutlet weak var outline: NSOutlineView!
    @IBOutlet weak var canvas: NCView!
    @IBOutlet weak var window: NSWindow!
    var currentFile: NoteFile!
    
    func applicationDidFinishLaunching(_ aNotification: Notification)
    {
        // Load whatever is currently selected
        if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile
        {
            self.canvas.setCanvas(canvas: selected.open())
        }
    }
    
    func applicationWillTerminate(_ aNotification: Notification)
    {
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool
    {
        return true
    }
    
    override func awakeFromNib()
    {
        window.titleVisibility = .hidden
    }
    
    @IBAction func newNote(_ sender: AnyObject)
    {
        if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile ?? AppDelegate.noteDirectory
        {
            if let (new, c) = selected.createNew()
            {
                self.canvas.setCanvas(canvas: c)
                self.outline.reloadData()
                
                let index = self.outline.row(forItem: new)
                self.outline.selectRowIndexes([index], byExtendingSelection: false)
                self.outline.editColumn(0, row: index, with: nil, select: true)
                self.currentFile = new
            }
        }
    }
    
    @IBAction func newDirectory(_ sender: AnyObject)
    {
        if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile ?? AppDelegate.noteDirectory
        {
            do
            {
                let new = try selected.createDirectory()
                self.outline.reloadData()
                
                let index = self.outline.row(forItem: new)
                self.outline.selectRowIndexes([index], byExtendingSelection: false)
                self.outline.editColumn(0, row: index, with: nil, select: true)
                self.currentFile = new
            }
            catch let e as NSError
            {
                let alert = NSAlert()
                alert.messageText = "Error: " + e.localizedDescription
                alert.addButton(withTitle: "OK")
                alert.beginSheetModal(for: self.window)
            }
        }
    }
    
    override func validateMenuItem(_ item: NSMenuItem) -> Bool
    {
        if(item.action == #selector(delete))
        {
            return self.window.firstResponder == self.outline && self.outline.selectedRow != -1
        }
        
        return true;
    }

    // Called by delete menu item, even though it's not an override...
    func delete(_ sender: Any?)
    {
        if(self.window.firstResponder == self.outline)
        {
            if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile
            {
                let alert = NSAlert()
                alert.messageText = "Are you sure you want to delete " + selected.name
                if(selected.isDirectory)
                {
                    alert.messageText += " (a directory)"
                }
                alert.messageText += "?"
                alert.alertStyle = .warning
                alert.addButton(withTitle: "OK")
                alert.addButton(withTitle: "Cancel")
                alert.beginSheetModal(for: self.window, completionHandler: { (modalResponse) -> Void in
                    if(modalResponse == NSAlertFirstButtonReturn)
                    {
                        do
                        {
                            try selected.delete()
                            self.outline.reloadData()
                            
                            if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile
                            {
                                if let c = selected.open()
                                {
                                    self.canvas.setCanvas(canvas: c)
                                    self.currentFile = selected
                                }
                            }
                            else
                            {
                                self.canvas.setCanvas(canvas: nil)
                            }
                        }
                        catch let e as NSError
                        {
                            let alert = NSAlert()
                            alert.messageText = "Error: " + e.localizedDescription
                            alert.addButton(withTitle: "OK")
                            alert.beginSheetModal(for: self.window)
                        }
                    }
                })
            }
        }
    }
}

// Left-side notelist view data source and delegate
extension AppDelegate: NSOutlineViewDelegate, NSOutlineViewDataSource
{
    // Get file object child by index
    func outlineView(_ outlineView: NSOutlineView, child i: Int, ofItem item: Any?) -> Any
    {
        if let it = item as? NoteFile
        {
            return it.children[i]
        }
        
        return AppDelegate.noteDirectory.children[i]
    }
    
    // Get number of children of file object
    func outlineView(_ outlineView: NSOutlineView, numberOfChildrenOfItem item: Any?) -> Int
    {
        if let it = item as? NoteFile
        {
            return it.count
        }
        if let d = AppDelegate.noteDirectory
        {
            return d.count
        }
        return 0
    }
    
    // Get file object name
    func outlineView(_ outlineView: NSOutlineView, objectValueFor tableColumn: NSTableColumn?, byItem item: Any?) -> Any?
    {
        if let it = item as? NoteFile
        {
            return it.name
        }
        return nil
    }
    
    // Get if expandable
    func outlineView(_ outlineView: NSOutlineView, isItemExpandable item: Any) -> Bool
    {
        if let it = item as? NoteFile
        {
            return it.count > 0
        }
        return false
    }
    
    // Get if selectable
    func outlineView(_ outlineView: NSOutlineView, shouldSelectItem item: Any) -> Bool
    {
        return true
    }
    
    // Check if editable
    func outlineView(_ outlineView: NSOutlineView, shouldEdit tableColumn: NSTableColumn?, item: Any) -> Bool
    {
        return true
    }
    
    // On edit
    func outlineView(_ outlineView: NSOutlineView, setObjectValue object: Any?, for tableColumn: NSTableColumn?, byItem item: Any?)
    {
        if let it = item as? NoteFile
        {
            if let name = object as? String
            {
                do
                {
                    try it.tryRename(name)
                    
                    if(self.currentFile == it)
                    {
                        // If the currently open file was just renamed,
                        // reopen it with the new name
                        self.canvas.setCanvas(canvas: it.open())
                    }
                }
                catch let e as NSError
                {
                    let alert = NSAlert()
                    alert.messageText = "Error: " + e.localizedDescription
                    alert.addButton(withTitle: "OK")
                    alert.beginSheetModal(for: self.window)
                }
            }
        }
    }
    
    // Selection changed
    func outlineViewSelectionDidChange(_ notification: Notification)
    {
        if let selected = self.outline.item(atRow: self.outline.selectedRow) as? NoteFile
        {
            if let c = selected.open()
            {
                self.canvas.setCanvas(canvas: c)
                self.currentFile = selected
            }
        }
    }
}

// Represents a file or directory in the Application Support tree
class NoteFile : NSObject
{
    var url: URL!
    var parent: NoteFile!

    var name: String {
        get {
            return self.url.lastPathComponent
        }
    }
    
    private var _children: [NoteFile]!
    var children: [NoteFile] {
        get {
            if(self._children == nil)
            {
                self.updateChildren()
            }
            return self._children
        }
    }
    
    var count: Int {
        get {
            return self.children.count
        }
    }
    
    lazy var isDirectory: Bool = {
        var isdir : ObjCBool = false
        FileManager.default.fileExists(atPath: self.url.path, isDirectory: &isdir)
        return isdir.boolValue
    }()
    
    // If this is a directory, returns itself.
    // Otherwise, return the parent directory.
    var directory: NoteFile {
        get {
            if(self.isDirectory || self.parent == nil)
            {
                return self
            }
            return self.parent
        }
    }

    
    init(_ url: URL, _ parent: NoteFile!)
    {
        super.init()
        
        self.url = url
        self.parent = parent
    }
    
    // Create new NoteFile which is the root note directory
    convenience init?(_: Void)
    {
        do
        {
            var dir : URL
            dir = try FileManager.default.url(for: FileManager.SearchPathDirectory.applicationSupportDirectory, in: FileManager.SearchPathDomainMask.userDomainMask, appropriateFor: nil, create: true)
            
            let name : String? = Bundle.main.infoDictionary?["CFBundleExecutable"] as! String?
            if(name != nil)
            {
                dir = dir.appendingPathComponent(name!, isDirectory: true)
                dir = dir.appendingPathComponent("Notes", isDirectory: true)
                
                try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
                
                self.init(dir, nil)
            }
            else
            {
                return nil
            }
        }
        catch
        {
            return nil
        }
    }
    
    // Find the child that matches the given URL
    func child(matching url: URL) -> NoteFile!
    {
        for child in self.children
        {
            if(child.url.absoluteString == url.absoluteString)
            {
                return child
            }
        }
        
        return nil
    }
    
    // Try to rename this file to the given name.
    func tryRename(_ name: String) throws
    {
        if(name.range(of: "/") != nil)
        {
            throw NSError(domain: NSURLErrorDomain, code: 0, userInfo: [NSLocalizedDescriptionKey: "Name cannot contain a forward slash (/)"])
        }
        
        let newurl = self.url.deletingLastPathComponent().appendingPathComponent(name)
        
        try FileManager.default.moveItem(at: self.url, to: newurl)
        
        self.url = newurl
        self.updateChildrenNames()
    }
    
    // Recursively update the URLs of children.
    // Used after a call to tryRename.
    private func updateChildrenNames()
    {
        for child in self.children
        {
            child.url = self.url.appendingPathComponent(child.url.lastPathComponent)
            child.updateChildrenNames()
        }
    }
    
    // Loads in children
    func updateChildren()
    {
        self._children = [NoteFile]()
        let options: FileManager.DirectoryEnumerationOptions = [.skipsHiddenFiles, .skipsPackageDescendants, .skipsSubdirectoryDescendants]
        if let enumerator = FileManager.default.enumerator(at: self.url, includingPropertiesForKeys:[URLResourceKey](), options: options, errorHandler: nil)
        {
            while let url = enumerator.nextObject() as? URL
            {
                self._children.append(NoteFile(url, self))
            }
        }
    }
    
    // Open this file as a NotedCanvas
    func open() -> OpaquePointer!
    {
        if(self.isDirectory)
        {
            return nil
        }
        
        return noted_canvas_open(self.url.path)
    }
    
    // Try to create a new note in this directory
    func createNew() -> (NoteFile, OpaquePointer)!
    {
        let dir = self.directory
        if(dir.url == nil)
        {
            return nil
        }
        
        // Make sure directory is updated
        dir.updateChildren()
        
        // Create name
        var new = dir.url!.appendingPathComponent("New Note")
        var i = 1
        while(dir.child(matching: new) != nil)
        {
            new = dir.url!.appendingPathComponent(String(format: "New Note (%i)", i))
            i += 1
        }
        
        // See if the note already exists, just in case
        var c = noted_canvas_open(new.path)
        if(c != nil)
        {
            noted_canvas_destroy(c)
            return nil
        }
        
        // Create
        c = noted_canvas_new(new.path)
        if(c == nil)
        {
            return nil
        }
        
        let file = NoteFile(new, dir)
        dir._children.append(file)
        return (file, c!)
    }
    
    func createDirectory() throws -> NoteFile
    {
        let dir = self.directory
        if(dir.url == nil)
        {
            throw NSError(domain: NSURLErrorDomain, code: 0, userInfo: [NSLocalizedDescriptionKey: "Unknown"])
        }
        
        // Make sure directory is updated
        self.directory.updateChildren()
        
        // Create name
        var new = dir.url!.appendingPathComponent("New Directory")
        var i = 1
        while(dir.child(matching: new) != nil)
        {
            new = dir.url!.appendingPathComponent(String(format: "New Directory (%i)", i))
            i += 1
        }
        
        // Create
        try FileManager.default.createDirectory(at: new, withIntermediateDirectories: false, attributes: nil)
        
        let file = NoteFile(new, dir)
        dir._children.append(file)
        return file
    }
    
    func delete() throws
    {
        try FileManager.default.trashItem(at: self.url, resultingItemURL: nil)
        self.url = nil
        if(self.parent != nil)
        {
            self.parent.updateChildren()
        }
    }
    
    // Comparison
    override func isEqual(_ object: Any?) -> Bool
    {
        if let object = object as? NoteFile
        {
            return self.url.absoluteString == object.url.absoluteString
        }
        else
        {
            return false
        }
    }
    
    override var hash: Int
    {
        return self.url.absoluteString.hash
    }
}
