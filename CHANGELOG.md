## 0.5.2

* Editing:
    * Added support for animation following a path
* Bug fixes:
    * Fixes loading lottie with hidden stylers
* Other:
    * Added Flatpack
    * Fixed Freedesktop file naming
    * Added Freedesktop metainfo

## 0.5.1

* Editing:
    * New quantization algorithm available (Edge Exclusion Modes)
    * Space to toggle playback
    * Improved Trim Path logic
    * Added Offset Path modifier
    * Improved rendering performance
    * Holding Ctrl snaps the draw tool to 45 degree lines
    * Added Zig Zag modifier
    * Dialog to search and import animations from LottieFiles
* I/O:
    * Fixed lottie export with using global swatch colors
    * Fixed loading external assets by relative path
    * Added option to automatically embed external images on lottie export
    * Fixed parsing of SVG path data with multiple closed paths
    * Added option to toggle old-style lottie keyframe
    * Added support for SVG with animated `display` attribute
* UI:
    * Simplified version of the trace dialog
    * Added Spanish translation
* Bug Fixes:
    * Fixed crash on adding gradient stops
    * Fixed Trim Path handling of multiple shapes
    * Fixed rendering of text objects with large font size
    * Removed spurious warnings when loading a lottie with gradients
    * Fixed paths loaded from SVG having an inconsistent closed state
    * Fixed import of transparent gradients from lottie
* Other:
    * Ported to Qt6

## 0.5.0

* Editing:
    * Action to make animations loop in the property context menu
    * Added Inflate/Deflate modifier (aka Pucker/Bloat)
    * Added support for rounded polygons / stars
    * Added rounded corners modifier
    * Added support for inverted masks
* I/O:
    * Loading Lottie with `meta` no longer show a warning
    * Fixed UUIDs when loading lottie animations
    * Fixed loading layers with a masked parent from lottie
    * Fixed exporting polygons to lottie
    * Added support for loading (old) lottie v 4.0.0
    * Added support for common metadata (author/description) across formats
    * Fixed loading easing from lottie
    * Added support for reversed lottie shapes
    * Fixed parsing SVG paths
    * Added support for loading basic animated SVG paths
    * Fixed importing solid layers from lottie
    * Fixed importing mattes affecting precomp
    * Added transparency support when exporting to WebM
* UI:
    * Added simplified Chinese translation
    * Added British English translation
    * Available languages are shown in their own language in the settings dialog
    * You can drag on the timeline property tree to change a layer's parent link
    * New icon / logo
* Bug Fixes:
    * Fixed rows breaking up in the timeline
    * Fixed various issues with trim path
    * Fixed importing lottie files

## 0.4.6

* Editing
    * Added support for stretching time on the timing dialog
    * The object context menu now can show a dilaog to change animation timing on the object
    * Ability to add fonts from Google Fonts
    * The edit tool now has its own undo history
    * Changing color/style while creating a shape now properly shows the changes
    * You can now convert/render files with glaxnimate from the command line
* I/O:
    * Fixed position of tspan on SVG output
    * Fixed shapes not showing on SVG output
    * Fonts defined in CSS @font-face are now loaded from SVG
    * External fonts are now loaded from lottie
* Scripting:
    * Ability to stretch time for documents and individual objects
* Bug Fixes:
    * The timing dialog honours "Keep initial timing"
    * Fixed crash when switching between compositions
    * Fixed some CSS parsing issues
    * Fixed memory leak on file load
    * The text tool options shows the right font family when selecting an object
    * The list of available languages shows correctly again in the settings dialog

## 0.4.5

* Editing:
    * Action to remove all animations from a property
    * When deleting the currently active object, its parent will be selected instead
* UI:
    * New handle for translating layers, groups etc without having to drag on the object
    * Menu items for images in the asset view
    * Saving the document swatch will ask for a name
    * Transform handles for rotation/translation now keep a constant distance, regardless of zoom
    * Added French translation
    * Improved Italian translation
* I/O:
    * Made error messages from the lottie importer easier to understand
    * SVG export now supports hold frames
    * SVG import now assumes back for unspecified fill
    * Ability to export as a spritesheet
    * Fixed bug then loading CSS from SVG
* Scripting:
    * The Headless environment context manager is no longer required when using glaxnimate as a module
* Bug Fixes:
    * Fixed loading lottie animations with missing "a" attributes
    * The rotation handle no longer changes position based on scale
    * Adding new layers inserts them on top of the selection
    * Fixed the canvas not rendering anything when the document area isn't in view
    * Dragging multiple objects in the layer list properly moves all of them
    * Fixed current selection not being displayed correctly in the layer view
    * You can no longer delete an object if its parent is locked

## 0.4.4

* Editing:
    * Repeater modifier
    * Trim path modifier
    * Improved shape to path conversion
    * Can convert shapes to path from the context menu
    * You can now drag multiple keyframes on the timeline
    * Text can now follow a path
    * Touch gestures for the canvas (pinch and pan)
    * Dropping a file onto the main window now gives options to import or open the file
    * Preset and validation of Discord stickers
    * Ability to import SVG from popular emoji sets
* UI:
    * Made the "Tool Options" view more compact
    * Template system for new files
    * Startup dialog
    * The timeline now allows selecting multiple rows at once
    * In recording mode, a keyframe at time 0 is added automatically for non animated properties
    * Button to clear fill/stroke colors
    * The context menu you get on the composition tabs now allows you to rename precompositions
* I/O:
    * More color formats are supported for the SVG importe
    * Automatically removes embedded images when exporting to TGS
    * Preview exports now show the warnings you'd get when saving to that format
* Scripting:
    * Python can now convert objects to path
    * More intuitive construction of C++ classes from python
* Mobile:
    * Experimental Android port
* Bug Fixes:
    * Dragging keyframes properly preserves keyframe transitions
    * Fixed mask export for lottie
    * Fixed crash on with color quantization on Mac ARM
    * Fixed some canvas selection inconsistencies
    * When creating text shapes, it honours fill/stroke checkboxes
    * Fixed initial font style when creating new text shapes
    * Fixed clicking on text shapes
    * Switching away from the draw tool no longer removes the path
    * Fixed Editing colors not merging in a single undo command
    * Fixed precomposing layer already in precomps
    * Fixed switching to the newly created comp when precomposing
    * Better naming for compositions created from a single object
    * Dragging no longer moves objects that don't need to be moved
    * Fixed tab bars not showing when loading documents containing precomps
    * Fixed crash when opening the most recent file
    * Imported files show their name in the created layer
    * Fixed Lottie export of transparent color
    * Fixed most recent file being forgotten sometimes
    * Fixed anchor points not being loaded properly from SVG
    * SVG import now sets the correct size from the viewBox if width/height aren't specified
    * Fixed plugin-based importers

## 0.4.3

* Editing:
    * The shortcut for "Undo" now works also to remove points when drawing paths
    * The trace dialog is now better at autodetecting colors
* UI:
    * Preferences dialog no longer closes on Enter
    * Some layer properties are displayed inline in the timeline tree
    * Bezier tangents are shown a bit thicker
    * The timeline shows precomposition tabs
    * The timeline now shows the whole composition and more properties are available
    * The timeline now support drag/drop operations
    * Status bar now shows the mouse position
    * When in keyframe recorning mode, the canvas outline turns red and a message appears in the status bar
    * Status bar now shows fill/stroke colors
    * New toolbar for drawing tools
    * New options to configure toolbars
    * Improved scrolling on the timeline
    * Made alignment buttons a bit smaller
* I/O:
    * Lottie output is slightly more compact
    * Added minimal support for CSS on SVG import
    * Improved error reporting when loading lotttie files
    * Improved loading / saving hidden objects for lottie
    * Improved performance when loading large SVG files
* Scripting:
    * New plugin to help with frame-by-frame animation
    * Scripts have access to the current composition
    * Improved support for python standard io streams
    * Exposed bitmap tracing and color quantization utilities
    * Exposed single frame rendering capabilities (raster and svg)
    * New snippet system to run custom scripts without the need of a plugin
* Bug Fixes:
    * Custom keyboard shortcuts are saved properly
    * Fixed values blowing up when changed from the property tree
    * Fixed crash caused by stale model indices
    * Precompositions correctly update whhen the current frame is changed
    * Clicking on the checkboxes in the plugin settings properly toggles the plugins
    * Lottie precomposition dependencies are correctly resolved on import
    * Fixed parenting transforms for imported lottie precomp layers
    * Fixed importing SVG path data
    * Fixed the HueShift plugin
    * Fixed loading transparent gradients from lottie
    * Gradients can be renamed
    * Fixed undoing precomp deletion
    * Fixed visual glitches when switching compositions
    * Fixed row stripe misalignment on the timeline
    * Fixed text to shapes
    * Fixed loading SVG opacity specified as a percentage
    * Fixed wrong selection when undoing precompose
    * Fixed converting shapes to path not closing paths properly
    * Fixed initial zoom for the timeline

## 0.4.2

* Editing:
    * Text shapes
* UI:
    * The path for import images or render frames dialogs is preserved
    * View to manage assets
    * Pressing enter no longer closes the trace bitmap dialog
    * The trace dialog now remembers its settings (and there's a button to reset them to default)
    * Updated dark theme colors
    * Property lists show opacity, scale and similar as percentages
    * Default layer color changed to transparent
    * New "hidden" debug menu
* I/O:
    * Exporting text to SVG
    * Exporting text to Lottie/TGS as shapes
    * Importing text from SVG
    * Importing text from Lottie
* Bug Fixes:
    * Fixed dragging layers to the top
    * Fixed converting ellipses to paths
    * Newly drawn shapes are added on top of existing ones
    * Fixed extending paths with the draw tool
    * Trying to change a built in widget theme color will create a new palette
    * Fixed pasting gradients

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
    * `choices` options are correctly loaded for plugins
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
    * Star shapes properly imported from lottie files
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
    * SVG output now preserve shapes instead of converting everything to path
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
