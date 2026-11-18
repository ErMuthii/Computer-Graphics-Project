# Browser studio product contract

Kitenge Pattern Studio helps students and pattern designers draw a motif, create
repeating cloth, evaluate colour relationships, and preview a patterned dress.
The browser extension accompanies the original C++/OpenGL coursework project.

The confirmed workflow is local browser editing, using mouse-drawn strokes and
editable geometric shapes. The same pattern feeds a fabric preview, an uploaded
person photograph with a new dress overlay, and a generated 3D mannequin wearing
the dress. Photo fitting begins with automatic shoulder/hip detection and allows
manual correction. Colour scoring explains hue harmony and luminance contrast.

The expanded workflow supports 18 starting motifs, 24 colour themes and 84
swatches plus custom hex colours. A design has one background, one primary and
up to 30 accents. Photo fitting uses a shaped outline with consistent print
scale and optional shading. Users can enlarge the view, tune the neckline, and
protect/restore original photo pixels for hands, hair and accessories.

The library adds ten editable fabric-and-dress sets with paired previews and
five dress silhouettes. Users can load and edit a set, undo loading, save its
silhouette in project JSON, and export fabric, garment illustration, 3D view or
fitted photograph. PNG exports use Blob downloads with visible retry/preview
links so embedded browsers have a recovery path.

Essential outputs are editable project JSON and PNG images of cloth, photo
fitting and the 3D view. No account, remote photo processing or live camera is
required. The six-member group needs readable implementation and reproducible
tests for its technical demonstration.

The preview demonstrates graphics, not physical garment fitting. The 3D model
is procedural; it does not reconstruct the uploaded person. Photo compositing
does not erase their existing clothes. These boundaries must stay visible in
the application and technical documentation.
