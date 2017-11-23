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
    @IBOutlet weak var outline: NSOutlineView!

    // Root NoteFile; directory where notes are stored
    static var noteDirectory: NoteFile! = {
        var dir : URL
        do {
            dir = try FileManager.default.url(for: FileManager.SearchPathDirectory.applicationSupportDirectory, in: FileManager.SearchPathDomainMask.userDomainMask, appropriateFor: nil, create: true)
        } catch {
            return nil
        }
        
        let name : String? = Bundle.main.infoDictionary?["CFBundleExecutable"] as! String?
        if(name == nil)
        {
            return nil
        }
        
        dir = dir.appendingPathComponent(name!, isDirectory: true)
        dir = dir.appendingPathComponent("Notes", isDirectory: true)

        do {
            try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        } catch {
            return nil
        }
        
        return NoteFile(dir)
    }()
    
    func applicationDidFinishLaunching(_ aNotification: Notification)
    {
//        let x = outline.item(atRow: outline.selectedRow)
//        print(x)
    }
    
    func applicationWillTerminate(_ aNotification: Notification)
    {
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool
    {
        return true
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
            return it.child(atIndex: i)
        }
        
        return AppDelegate.noteDirectory.child(atIndex: i)
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
        if let it = item as? NoteFile
        {
            return !it.isDirectory
        }
        return false
    }
    
    func outlineViewSelectionDidChange(_ notification: Notification)
    {
//        let x = outline.item(atRow: outline.selectedRow)
//        print(x)
    }
}

// Represents a file or directory in the Application Support tree
class NoteFile
{
    var url: URL!
    var dirctory: Bool = false
    
    static let options: FileManager.DirectoryEnumerationOptions = [.skipsHiddenFiles, .skipsPackageDescendants, .skipsSubdirectoryDescendants]
    
    lazy var children: [NoteFile] = {
        guard let enumerator = FileManager.default.enumerator(at: self.url, includingPropertiesForKeys:[URLResourceKey](), options: NoteFile.options, errorHandler: nil) else
        {
            return []
        }

        var files = [NoteFile]()
        while let url = enumerator.nextObject() as? URL
        {
            files.append(NoteFile(url))
        }
        return files
//        do {
//            return try FileManager.default.contentsOfDirectory(at: self.url, includingPropertiesForKeys: [], options: FileManager.DirectoryEnumerationOptions.skipsHiddenFiles)
//        } catch {
//            return []
//        }
    }()
    
    var name: String {
        get {
            return self.url.lastPathComponent
        }
    }
    
    var isDirectory: Bool {
        get {
            return self.dirctory
        }
    }
    
    var count: Int {
        get {
            return self.children.count
        }
    }
    
    init(_ url: URL)
    {
        self.url = url
        
        var isdir : ObjCBool = false
        FileManager.default.fileExists(atPath: url.path, isDirectory: &isdir)
        self.dirctory = isdir.boolValue
    }
    
    func child(atIndex i: Int) -> NoteFile
    {
        return self.children[i]
    }
}
