## 0.4.1

* Editing:
    * Added menu action and button to flip the view
    * The draw tool can now extend existing paths
* UI:
    * Improved and expanded the alignment dock view
    * There is now an outline around the canvas draw area
    * Canvas playback now might skip frames to match real time
* Bug Fixes:
    * Fix pasting SVG data
    * Fix window size for smaller screen resolutions
    * Fixed loading SVG animated colors
    * Fixed draw tools marker scaling
    * Fixed parenting when grouping objects
    * Selection area for an object is now more accurate
    * Improved performance for canvas navigation and editing
    * Fixed easing import/export for lottie
    * Fixed copy/pasting shape keyframes
    * Long file export operations properly show the progress dialog
	* Scripts should now work on windows without Python installed

## 0.4.0

* Editing:
    * Action to paste as a composition
    * Action to import animations
    * Added checks to avoid cyclical dependencies between compositions
    * Context menu action to "Decompose" PreComp layers
    * Support for clipping masks
* UI:
    * Right clicking on the layer view to get a context menu no longer changes the selection
    * Object context menus now have actions to toggle visibility and lock
    * Resizing the document duration in the timeline doesn't move the timeline chart
    * Layer context menu action to update its first/last frame based on the main comp
    * Improved canvas rendering performance
    * "Reset Transform" action for images in the context menu
* I/O:
    * Animated SVG now honours layer first/last frames
    * SVG keeps track of parent layer transforms
    * Animated SVG now trims excess frames from animated properties
    * Animated SVG output now properly supports animated opacity for layers and groups
    * The SVG parsers now parses correctly "d" attributes without spaces
    * Fixed lottie export of layers containing shape groups and other layers
    * Hidden object are hidden on lottie export
    * All exported lottie layers now have a layer index
    * Added support for lottie mattes and masks
    * Added support for SVG masks and clipping paths
* Scripting:
    * Python has now access to the users of an asset
    * Python can update the current selection
* Bug Fixes:
    * Duplicating shapes updates uuids
    * Importing images sets their name
    * Fixed hidden layers

## 0.3.2

* Editing:
    * Added support for precompositions
    * Ability to copy/paste keyframe values (through menus)
    * Menu action (and keyboard shortcut) to duplicate the selected shapes
    * Holding Ctrl while drawing stars/polygons snaps their angle to 15 degrees increments
    * Rubberband selections only selects objects fully within the rubber band
    * Menu and view to align selected objects
    * Hold Ctrl while dragging an objects with the select tool to snap movement to the axes
* UI:
    * The timeline now allows editing layer frame ranges
    * When selecting groups, its inner objects will show in the timeline
    * The timeline now shows some non-animated properties for convenience
    * The trace dialog has a slider to compare the trace preview to the original image
    * The timeline has a larger handle between properties and keyframes
    * Objects in the timeline can be expanded and collapsed
    * The timeline and properties view will show gradient properties when a fill or stroke object is selected
* Scripting:
    * Color values can be manipulated using the HSV color space
    * Scripts can update gradient colors
    * Scripts can read the list of selected shapes
    * New plugin to shift hues in the selected shapes
* I/O:
    * Added support for gradientTransform when parsing SVG
    * Added more warnings for features not supported by Telegram
* Bug Fixes:
    * Fixed crash caused by switching between certain themes
    * Fixed missing canvas handles when selecting the main composition
    * Opacity properly displayed when loading objects
    * Fixed some rounding errors when exporting to tgs
    * Fixed copy/pasting gradients
    * Fixed layer bounding boxes not being updated in some cases
    * Rendering to raster correctly takes into account layer parenting
    * Loading Lottie layers with non-integer in/out points works correctly
    * Pixed python libraries on windows
    * Fixed crash when deleting a gradient that is being edited
    * Fixed loading transparent gradients
    * The "record" button is unchecked when loading a new animation
    * Transparent gradients are correctly exported to lottie
    * `choices` options are correctly loaded fro plugins
    * Items in the Plugins menu are now kept always in a sensible order
    * Dragging keframes keeps the correct selection
    * Image shapes that are children of layers are correctly exported to Lottie

## 0.3.1

* Editing:
    * Menu action to convert any shape into a Path
    * Removing keyframes will preserve easing across the affected keyframes
    * Fill tool
    * The context menu for bezier handles now shows property actions
    * Bezier tangent handles are only shown for the selected nodes
* I/O:
    * Support for SVG SMIL animations
    * Browser SVG preview
    * Browser Lottie canvas preview
    * Video export, supporting most video formats that ffmpeg supports
* UI:
    * Added a theme style selector
* Bug Fixes:
    * Removed spurious warnings when loading lottie
    * Bezier data is loaded correctly from lottie
    * Fixed layer parenting when loading lottie
    * Rounded rectangles are correctly exported to SVG
    * Duplicating shapes selects the new shape and places it on top of the original
    * Keyframes added on newly created objects are on the right frame
    * OK/Cancel buttons in the Trace Bitmap dialog now react to user input
    * When the selected object is deleted, the timeline and property views are cleared
    * Adding keyframes preserves linear/hold easing
    * SVG scale() is parsed correctly
    * Resize dialog spin boxes update correctly when ratio is locked

## 0.3.0

* Editing:
    * Adding or removing bezier nodes will affect all keyframes
    * Adding or removing gradient stops will affect all keyframes
    * Options on what to do with layers when re-timing a document
    * Adding new keyframes will preserve easing across the affected keyframes
    * Menu action to convert between Layers and Groups
    * Ability to trace bitmaps
* I/O:
    * Support for rendering and loading animated GIF and WebP
    * Open/Save dotLottie animations
    * Open/Save Synfig files
* UI:
    * Dialog to edit document metadata
* Scripting:
    * Ability to load Qt designer UI files
    * Ability to create any document object
    * Rendering to PIL images
    * Import/Export plugin types
    * Button to clear the script console
    * Button to reload plugin modules
* Bug Fixes:
    * Animated Path shapes keeps the correct tangents
    * Fixed inconsistencies with closed/open bezier paths
    * Removed spurious warning when loading animated gradients
    * Duplicating shapes correctly sets up the duplicate's animations
    * File-based plugin icons are correctly loaded
    * Images are properly loaded from lottie files
    * Fixed crash when opening a document
    * Opening a raster image correctly sets up the canvas size
    * Groups and Layers get their bounding boxes properly updated when changing frames

## 0.2.0

* Editing:
    * Last used Named Color will be used for new shapes
    * Dragging bezier very close to nodes no longer initiates molding
* UI:
    * Improved support for changing the icon theme
    * Drag/Drop in the layer view
    * Menu action to remove unused assets
    * Dialog to change animation timing
    * Added Playback menu
    * Left/Right arrow keys change frame even if the timeline isn't focused
    * Settings page to change keyboard shortcuts
* Scripting:
    * Python module
* Bug Fixes:
    * Fixed misleading mouse cursor when hovering over bezier node handles
    * Releasing shift/ctrl on the draw tool correctly updates the path
    * Canceling a node in the draw tool properly resets tangents
    * Unused gradient settings are removed when no longer needed
    * Fixed title of the "Move To" dialog
    * Fixed lottie export of properties with exactly 1 keyframe
    * Fixed layer order on lottie export
    * Theme editor properly supports transparency

## 0.1.5

* Editing:
    * Added gradient support
    * Option to exclude some layers from being rendered / exported
    * Freehand drawing tool
    * Interactive "add node" with the edit tool
    * Bezier molding, click and drag on a curve to edit it, without using handles
    * "Dissolve nodes", remove bezier nodes while trying to keep the old shape
    * Right-clicking on a handle with the select or edit tools will display a context menu for it
* I/O:
    * Floats get truncated to 3 digits on compact lottie / tgs output
* UI:
    * Configurable widget themes
    * The property area of the timeline view can now be dragged by the user to expand it
* Bug Fixes:
    * Exporting the document no longer alters the document save state
    * Fixed crash on Lottie export
    * Fixed keyframe Lottie export
    * Timeline view always shows the last frame
    * Improved performance when dragging multiple objects

## 0.1.4

* Editing:
    * Added support for copy/pasting document defs
    * Dragging bezier handles now replaces the current selection when needed
* Housekeeping:
    * Moved the project to GitLab group to have access to more CI minutes
* Bug Fixes:
    * Fixed rlottie layer transforms for exported lottie
    * Importing lottie keyframe checks the type before unwrapping arrays
    * Fixed crash when selecting handles
    * Export menu action shows the right file name after exporting

## 0.1.3

* Editing:
    * Export action
* Bug Fixes:
    * Fixed crash when exporting lottie

## 0.1.2

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
    * Imported/pasted images have the transform anchor point set to their center
* I/O:
    * Paste images
    * Open images
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
