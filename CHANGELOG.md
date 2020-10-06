## 0.2.0

* Editing:
    * Ability to embed external images
    * Quick action to reopen the last opened/saved document
    * Right clicking with the select tool shows actions for all the objects under the mouse
    * The shape editing tool now shows a context menu when right-clicking on nodes
    * The select tool now can only modify transforms, all other editing operations are done by the edit tool
    * The select tool will show an outline for selected shapes (that are not groups)
    * Implemented node type menu actions
    * Added node/layer delete menu action
    * Added segment curve/straighten menu action
* I/O:
    * Paste images
* Bug Fixes:
    * Star shapes propely imported from lottie files
    * Layers are properly hidden when outside their frame range
    * Editing Path objects registers undo commands and keyframes
    * Changing Path node types now affects the shape correctly
    * Cut/Delete properly remove groups/layers

## 0.1.1

* Editing:
    * Star/Polygon Shapes
    * Document Swatch
    * Ability to lock objects to avoid editing them
    * More precise selection when clicking on the canvas
* Tools:
    * Star/Polygon Draw Tool
    * Clicking with the simple shapes drawing tools switches to the select tool
    * Color picking tool
* Scripting:
    * Added logging support integration with Python
* I/O:
    * SVG ouput now preserve shapes instead of converting everything to path
* Bug Fixes:
    * Fixed palettes not being saved correctly
    * Undoing commands no longer adds spurious keyframes
    * Dock widgets have their icon displayed correctly on all supported systems
    * Opacity is now being displayed correctly

## 0.1.0

* Editing:
    * Layer Management
    * Basic Animations
    * Basic Shapes
* Tools:
    * Select Tool
    * Bezier Edit Tool
    * Bezier Draw Tool
    * Rectangle Draw Tool
    * Ellipse Draw Tool
* I/O:
    * Open/Save Rawr files
    * Open/Save Lottie files
    * Open/Save Telegram animated stickers
    * Open SVG (without animations)
    * Render still frames as images
    * Render still frames as SVG
    * Browser Lottie preview
    * Copy/Paste within Glaxnimate
    * Copy/Paste SVG
    * Copy selection as images
    * Telegram sticker validation
    * Automatic backups
    * Drop files to open them
* Scripting:
    * Python Scripting capabilities
    * Menu action plugins
    * Python console
* UI:
    * Color Selection View
    * Stroke Style View
    * Scripting Console
    * Log View
    * Tool View
    * Shape creation tool options
    * Timeline View
    * Properties View
    * Document Node View
    * Undo History View
