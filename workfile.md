# Checkpoint 1 collaboration workfile

Project: **Kitenge Pattern Designer and Virtual Dress Preview**

This temporary working plan covers CP1. Keep it in the repository while the group
uses it, update progress through issues and PRs, and consolidate lasting decisions
into the README and algorithm documentation before retiring it after CP1.

## What we will submit

Create a static geometric fabric swatch and a simplified patterned dress using
graphics algorithms implemented in C++17. The group should explain how lines,
polygons, clipping, filling, circles, ellipses and Bézier curves create those
components. Use the specific algorithms taught in class if their names differ
from the proposed choices below.

The shared raster canvas saves a PPM image using only the C++ standard library.
Opening that image in a viewer is how we inspect and screenshot our output.
CP1 does not require a camera, OpenCV, OpenGL, automatic body detection, a live
preview or a color compatibility score. Those remain later project features.

Use a bodice and skirt built from separate convex polygons. This keeps polygon
clipping understandable; a single concave dress outline is not an appropriate
clipping boundary for the proposed Sutherland-Hodgman implementation. Curves can
add the neckline or decoration; the first clipped fabric panels remain polygons.

## First task: Muthii Eric Macharia, 166390

Branch: `feat/cp1-foundation`

Deliver the shared foundation first so everybody develops against the same
interfaces. This is a technical contribution, followed by responsibility for
integrating the completed modules into the final CP1 scene.

The starter implements:

- Shared `Color`, integer `Point`, floating `Point2D`, `Polygon` and `Rect` types.
- A `Canvas` with RGB storage, clear, pixel writes and checked pixel reads.
- A binary PPM writer and a pixel diagnostic executable.
- C++17 build configuration and checks of canvas boundaries and image encoding.
- Repository README, ignored build/generated files, and issue and PR templates.

Acceptance: compile the starter, run the foundation checks, generate the diagnostic
image and inspect it. Eric has authorized direct publication of this verified
foundation to `main`, waiving Jeremy's review for this initial task. Classmates
can start their branches as soon as the foundation is on remote `main`. Review
requirements for their algorithm PRs remain in place.

Each teammate should confirm that the foundation builds on their own machine;
the group has not yet specified its operating systems. The current local
verification uses macOS and Apple Clang with the explicit SDK selection
documented in README; the default compiler SDK is incompatible with the installed
linker. CMake 4.4.4 successfully configured and built a clean copy of the committed
files; CTest passed and the resulting executable generated its diagnostic image.
The direct compiler route was also verified. Other-platform builds still need
confirmation on the group's own machines.

Later integration: add the merged algorithms to the build, compose the fabric
swatch and two-panel dress, collect evidence, and make the README accurate for
the checkpoint submission. Integrating these components does not transfer their
authorship; keep each contributor's algorithm commits in the history.

## Six individual assignments

Each owner implements, explains and verifies their own module. Everyone uses
their own GitHub-linked Git identity. The reviewer is a teammate who checks the
math, code and rendered output before the PR is merged.

| Member and number | Branch | Owned implementation | Project application | Reviewer |
| --- | --- | --- | --- | --- |
| Muthii Eric Macharia — 166390 | `feat/cp1-foundation` | Shared foundation, build setup; later integration on `feat/cp1-scene` | Export images; combine the fabric swatch and dress | Initial foundation review waived by Eric |
| Waruhiu Jeremy Kang'ethe — 166263 | `feat/cp1-lines` | Bresenham lines and Cohen-Sutherland line clipping | Motif edges, seam lines and line clipping in a rectangular fabric window | Murega Kelvin Mutwiri |
| Deborah Rehana — 168656 | `feat/cp1-polygon-fill` | Polygon validation and scanline filling | Fill the bodice, skirt and geometric motifs | Sayialel Eric Lenina |
| Murega Kelvin Mutwiri — 166914 | `feat/cp1-polygon-clipping` | Sutherland-Hodgman clipping against convex boundaries | Crop polygon motifs at the edges of fabric panels | Andrew Karanja Gathirwa |
| Sayialel Eric Lenina — 161492 | `feat/cp1-circles-ellipses` | Midpoint circle and ellipse rasterization | Round and oval decorative motifs and buttons | Deborah Rehana |
| Andrew Karanja Gathirwa — 167144 | `feat/cp1-bezier` | Cubic Bézier evaluation and rendering | Curved neckline guides and curved fabric decoration | Muthii Eric Macharia |

### Jeremy: lines and line operations

Own `include/kitenge/lines.hpp`, `src/lines.cpp`, `tests/line_tests.cpp` and
`docs/algorithms/lines.md`. Implement Bresenham for all octants and both endpoint
orders, plus Cohen-Sutherland for a rectangular clipping window. Draw a simple
line-based motif and show lines before and after clipping.

Check horizontal, vertical, steep, negative-slope and single-point lines; reversed
endpoints; boundary-crossing, fully visible and fully rejected clipped lines.
Document what happens at the clipping boundary.

### Deborah: polygon representation and filling

Own `include/kitenge/polygon_fill.hpp`, `src/polygon_fill.cpp`,
`tests/polygon_fill_tests.cpp` and `docs/algorithms/polygon-fill.md`.
Use the shared ordered vertex list. Implement an even-odd scanline fill; explain
how horizontal edges and shared vertices are handled to avoid double-counting.
Validate polygons appropriately for the supported simple inputs and document
the assumptions. Fill a diamond and the simplified dress panels.

Check a rectangle, triangle, concave example, horizontal edges and off-canvas
coordinates. The first integrated scene uses simple non-self-intersecting panels.

### Kelvin: polygon clipping

Own `include/kitenge/polygon_clip.hpp`, `src/polygon_clip.cpp`,
`tests/polygon_clip_tests.cpp` and `docs/algorithms/polygon-clipping.md`.
Implement Sutherland-Hodgman against a convex polygon. Start with a rectangle,
then use convex trapezoid dress panels. Preserve floating-point intersections
until rasterization. Define how clip-boundary winding and boundary points work.

Check fully inside, fully outside, edge-crossing, boundary-touching and empty
inputs. Return an empty vertex list for no visible polygon. Demonstrate the
clipped geometric motif separately from its original shape. Polygon clipping
does not automatically clip circle pixels; the first demonstration uses polygon
motifs, with circles and curves shown inside safe panel interiors.

### Lenina: circles and ellipses

Own `include/kitenge/circles.hpp`, `src/circles.cpp`, `tests/circle_tests.cpp`
and `docs/algorithms/circles-ellipses.md`. Implement midpoint circle and ellipse
drawing and explain the symmetry used to place the pixels. Render decorative
circular and oval motifs. Circle outlines are sufficient; filling them is an
extension after the assigned algorithms work.

Check different radii, symmetry, edges of the canvas and zero-size cases. Reject
negative radii consistently. Define zero-axis ellipse behavior in the interface.

### Andrew: Bézier curves

Own `include/kitenge/curves.hpp`, `src/curves.cpp`, `tests/curve_tests.cpp`
and `docs/algorithms/bezier.md`. Evaluate a cubic Bézier curve using de Casteljau
or the class-taught equation. Render sampled segments using Jeremy's line
module once it is merged. Show the four control points and curve in a separate
demonstration, then add curved decoration or a neckline guide to the dress.

Check endpoints at t=0 and t=1, an intermediate point, repeated control points
and a straight-line configuration. Define input handling for t outside [0,1]
and for invalid segment counts. Splines are optional if time remains.

## Shared interface agreement

The foundation types are implemented now. The algorithm signatures below are
the proposed contract for the owners to implement; they are not completed code
or a claim that the modules already exist. Agree on changes in an issue before
changing another member's interface.

```cpp
// lines.hpp
void drawLine(Canvas&, Point start, Point end, Color);
std::optional<std::pair<Point2D, Point2D>>
clipLineToRect(Point2D start, Point2D end, Rect window);

// polygon_fill.hpp
void fillPolygon(Canvas&, const Polygon&, Color);

// polygon_clip.hpp: clipping boundary must be convex
Polygon clipPolygonToConvex(const Polygon& subject, const Polygon& boundary);

// circles.hpp
void drawCircleMidpoint(Canvas&, Point center, int radius, Color);
void drawEllipseMidpoint(Canvas&, Point center, int radiusX, int radiusY, Color);

// curves.hpp
Point2D evaluateCubicBezier(const std::array<Point2D, 4>& control, double t);
void drawCubicBezier(Canvas&, const std::array<Point2D, 4>& control,
                    Color, int segments = 100);
```

All declarations belong in `namespace kitenge` and include their required standard
headers. Coordinate origin is top left; x increases right and y increases down.
Pixels use integer coordinates and RGB values from 0 to 255. Geometric clipping
and curve evaluation use doubles; round with `std::lround` at the raster boundary.
A polygon stores vertices in order without repeating the first vertex at the end.
Image rows are stored and exported top to bottom.

`Canvas::setPixel` ignores positions outside the canvas. This is pixel safety,
not the project's line or polygon clipping algorithm; owners must implement
the actual geometric clipping calculations separately.

Build algorithms against `Canvas` rather than file output or camera input. Keep
the geometry and math independent so later transformations and OpenGL can reuse
the shapes and calculations. Do not add library calls that replace your assigned
rasterization, filling, clipping or curve implementation.

## Work order and integration

1. Eric publishes the verified foundation to `main` under the authorized initial
   review exception.
2. All members pull the merged `main` and create their own branches.
3. Lines, circles/ellipses, polygon fill and polygon clipping can begin independently
   using the shared types. Andrew can implement curve evaluation immediately and
   add curve rendering after line drawing is merged.
4. Each owner opens a PR for their working module, explanation and checks. Coordinate
   changes to shared `CMakeLists.txt` with Eric; integrate one such change at a time.
   Owners add their own source and checks to the build, and adjust README status
   only for the component actually completed.
5. Eric starts `feat/cp1-scene` from the updated `main` and composes the swatch,
   clipping comparison and simplified dress from the merged modules.
6. Each member helps review the output and documents their own contribution.
   Collect screenshots, finalize the README and check clean-clone build instructions.

No calendar deadline is invented here; add the lecturer's submission date to the
group issues when it is known. Keep the first complete scene small before adding
extra motifs or decorations.

## Git identity and branches

Each person first checks their repository-local Git identity:

```sh
git config user.name
git config user.email
```

If needed, configure their own name and an email verified on their GitHub account
or their GitHub-provided private commit email. Never copy another person's email.

```sh
git config user.name "Your full name"
git config user.email "Your own GitHub-linked commit email"
```

Once the foundation is published to `main`, run these commands in your local repository.
The branch below is Jeremy's example; substitute your assignment's branch name.

```sh
git switch main
git pull --ff-only origin main
git switch -c feat/cp1-lines
```

Code on the feature branch. Do not begin separate tasks on `main` or on a teammate's
branch. One branch covers one task. Use a follow-up fix branch for corrections
after a feature has already merged.

## Commit structure

Commit real, complete logical steps as the work progresses. Use this format:

```text
type(scope): describe the change
```

Types: `feat` for capability, `fix` for a correction, `test` for checks, `docs` for
explanations and `build` for build configuration. Scopes identify the feature:
`foundation`, `lines`, `polygon-fill`, `polygon-clip`, `circles`, `curves`, `scene`.

Good examples for actual implementation milestones:

```text
feat(lines): implement Bresenham drawing for all octants
test(lines): cover steep slopes and reversed endpoints
feat(polygon-clip): clip motifs against convex dress panels
fix(polygon-fill): avoid counting shared vertices twice
docs(curves): explain control points and curve sampling
```

For Eric's existing starter, one coherent commit can contain the shared foundation,
build setup, checks and collaboration documents:

```text
feat(foundation): add pure C++ canvas and checkpoint setup
```

Do not manufacture extra commits, submit all work in one end-of-checkpoint dump,
or label an unfinished algorithm as implemented. Never commit build outputs,
generated working images, credentials or a teammate's authorship identity.

Before every commit, inspect and stage only the intended paths. This example uses
files Jeremy will create; adapt it to the files in your actual task:

```sh
git status
git diff
git add include/kitenge/lines.hpp src/lines.cpp tests/line_tests.cpp
git diff --cached
git commit -m "feat(lines): implement Bresenham drawing for all octants"
git push -u origin feat/cp1-lines
```

After the first push, subsequent commits on the same branch use `git push`.
Include the necessary build edits in the implementation commit so the branch is
buildable; the checks and documentation may be part of that commit or separate
meaningful commits. A commit should have a clear purpose, not an arbitrary size.

## Pull requests and merging into main

1. Open a GitHub pull request with base `main` and your feature branch as compare.
2. Link its task issue. Explain the algorithm, project application, compile and
   check commands, visible result and limitations using the supplied PR template.
3. Request the assigned reviewer. They should run the code, inspect the image and
   ask the author to explain the algorithm. Eric coordinates integration changes.
4. Address review feedback on the same feature branch and push additional commits.
5. Once reviewed and buildable, merge using **Create a merge commit** so individual
   development commits and their original authors remain visible. The PR author
   or designated maintainer may merge if their GitHub permissions allow it.
6. After merging, everyone who needs the new module updates their own clone:

```sh
git switch main
git pull --ff-only origin main
```

Do not force-push `main`. If GitHub branch protection is available, the repository
owner can require a PR and at least one approval; that setting has not been changed
by this starter. The starter has no automatic GitHub CI workflow, so reviewers
must run the documented checks. Eric's initial foundation publication is the
explicitly authorized exception to the review workflow; it does not require
Jeremy's approval.

To update an ongoing feature branch when `main` changes, first commit or otherwise
safely preserve your local work, then use:

```sh
git fetch origin
git switch feat/cp1-lines
git merge origin/main
```

Resolve conflicts with the affected file's owner, stage the resolved files,
finish the merge commit, rebuild, run checks and push. If you cannot resolve the
merge safely, `git merge --abort` returns to the pre-merge branch state. Never
discard a teammate's changes simply to make a conflict disappear.

## Evidence and completion checklist

Each algorithm explanation should cover inputs, mathematical steps, important
edge cases, limitations and how it supports the pattern or dress. The owner
should be able to explain their implementation and the overall project.

Final CP1 evidence should contain a fabric swatch, a clipping comparison and an
integrated dress scene. Add selected screenshots in `docs/evidence/` with the
source commit, generation command, author and algorithms shown. A PDF may combine
them when required. The foundation color grid is diagnostic output, not enough
evidence for the checkpoint on its own.

The README must state the project title and description, members and responsibilities,
tools, algorithms actually implemented, compile and run instructions, progress,
limitations and issues. Check these instructions from the repository root on
the group members' actual platforms. Keep issues and PR links as the development
record; the source files and meaningful author commits provide contribution evidence.

## Current state

- Foundation implementation branch: `feat/cp1-foundation`.
- Shared foundation and documentation are present; the class algorithm modules
  remain assigned to their respective owners.
- The diagnostic executable and foundation checks build without warnings using
  the documented macOS SDK flag; the checks pass and the PPM image is generated.
- The foundation is published to GitHub `main` and its feature branch. Source
  commit: `723a41e`; initial merge into `main`: `92fc80e`. It uses Eric's configured
  Git identity, and the original feature commit is retained in history.
- Foundation implementation and the current build setup are complete. The CMake
  clean-checkout build and CTest pass; other-platform confirmation remains a
  teammate onboarding check, not a blocker on starting the assignments.
- Classmates should pull remote `main`, create their assigned branches and begin
  their modules. Future scene integration depends on those modules being merged.
