# Established design direction

Continue the desktop project's warm fabric palette: cream surfaces, cocoa
navigation, rust actions and olive active controls. Use a small serif wordmark
and legible system text for controls. The artwork is the visual focus.

Arrange the desktop as tools, motif/fabric workspace, and garment preview.
Use flat sections with clear labels rather than decorative cards. On narrow
screens stack the workspace and preserve all controls without horizontal
scrolling. Drawing and alignment use pointer events; numerical shape and
alignment fields provide precise correction. File controls, layers and preview
tabs are keyboard accessible.

Show selected, disabled, loading, empty, success and error states. Automatic
photo detection must visibly fall back to manual fitting. Display the palette
score with its component scores and contrast ratios so it can be explained.
Animation occurs in the fabric only when the user presses Play.

Keep the expanded motif gallery and swatches scrollable. The desktop tool
column scrolls independently so the artwork remains visible while editing
colours. On mobile, colour slots use two columns to keep labels readable.
The enlarged fitting view gives the photograph room beside its controls.
Protect/restore brushes name exactly which original pixels they reveal.

Selected motif shapes show eight square resize handles. Corners resize both
axes, edges resize one, and Shift locks corner proportions. Cursor directions
follow rotation. During drawing, defer inspector layout changes until pointer-up
to keep the canvas steady on narrow screens.
