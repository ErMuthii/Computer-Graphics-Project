import "./style.css";
import {
  defaults,
  motif,
  palettes,
  paintMotif,
  renderPattern,
  renderRepeat,
  hitShape,
  paletteScore,
  validateProject,
} from "./core.js";
import { createModel } from "./model.js";
import {
  hitResizeHandle,
  resizeFromHandle,
  resizeCursor,
} from "./selection.js";
import {
  detectPose,
  drawPhoto,
  manualAnchors,
  paintMaskStroke,
  rebuildMask,
} from "./photo.js";
import { motifCatalog, paletteCatalog, swatches } from "./catalog.js";

let project = defaults(),
  selected = null,
  tool = "select",
  role = 1,
  past = [],
  future = [],
  drag = null,
  playing = false,
  time = 0,
  last = performance.now(),
  dirty = true,
  photo = null,
  anchors = manualAnchors(),
  landmarks = null,
  photoToken = 0,
  mode = "model",
  model;
let photoTool = "fit",
  maskStrokes = [],
  maskDrag = null;
const photoMask = document.createElement("canvas"),
  repeatTile = document.createElement("canvas");
const $ = (s) => document.querySelector(s),
  snapshot = () => JSON.stringify(project);
const app = $("#app");
app.innerHTML = `
<header><a class="wordmark" href="./">Kitenge<span>Pattern Studio</span></a><p>One motif. A fabric of possibilities.</p><div class="file-actions"><button id="save">Save project</button><label class="button">Open project<input id="open" type="file" accept=".json,application/json" hidden></label><button class="primary" id="export">Export PNG</button></div></header>
<main><aside class="inspector"><h1>Your design</h1><p class="subtle">Create a motif, then see it become cloth.</p><section><h2>Starting motif</h2><p class="fine">18 editable designs, including the six native motifs.</p><div class="motif-grid" id="presets">${motifCatalog.map(([id, label]) => `<button data-preset="${id}" title="Start with ${label}"><canvas width="72" height="72" aria-hidden="true"></canvas><span>${label}</span></button>`).join("")}</div></section>
<section><h2>Drawing tools</h2><div class="tool-grid" role="group" aria-label="Drawing tools">${[
  ["select", "Select / move"],
  ["brush", "Freehand"],
  ["rectangle", "Rectangle"],
  ["ellipse", "Ellipse"],
  ["triangle", "Triangle"],
  ["diamond", "Diamond"],
]
  .map(
    ([id, label]) =>
      `<button data-tool="${id}" aria-pressed="${id === "select"}">${label}</button>`,
  )
  .join(
    "",
  )}</div><div class="button-row"><button id="undo" disabled>Undo</button><button id="redo" disabled>Redo</button><button id="delete" disabled>Delete</button></div></section>
<section><h2>Selected shape</h2><p id="selection" class="subtle">Select a shape to edit its geometry.</p><fieldset id="shape-fields" disabled><div class="pair"><label>Width<input id="shape-w" type="number" min="8" max="380" value="100"></label><label>Height<input id="shape-h" type="number" min="8" max="380" value="100"></label></div><label>Rotation <output id="shape-angle-out">0°</output><input id="shape-angle" type="range" min="-180" max="180" value="0"></label><label>Stroke width<input id="shape-stroke" type="range" min="1" max="24" value="4"></label><label class="check"><input type="checkbox" id="shape-fill" checked> Filled shape</label></fieldset></section>
<section><h2>Palette & drawing colours</h2><label>Colour theme<select id="palette-preset"><option value="">Choose a theme…</option>${paletteCatalog.map((p, i) => `<option value="${i}">${p.name}</option>`).join("")}</select></label><p class="fine">Custom colours are supported in every slot. Add accents to use several colours in one design.</p><div id="palette" class="palette"></div><label>Drawing colour / swatch target<select id="drawing-role"></select></label><div class="button-row"><button id="add-accent">Add accent</button><button id="remove-accent" disabled>Remove accent</button></div><p class="fine">Removing an accent recolours its shapes with Primary. Up to 32 active colours.</p><details class="swatch-library" open><summary>Colour library · 84 swatches</summary><label>Colour family<select id="swatch-family"><option value="all">All colours</option>${[...new Set(swatches.map((s) => s.family))].map((n) => `<option>${n}</option>`).join("")}</select></label><p id="swatch-target" class="fine"></p><div id="swatches" class="swatches"></div></details><button id="suggest" class="wide">Suggest a harmonious palette</button><div class="score"><strong id="score">—</strong><div><b>Palette balance</b><span id="scheme"></span></div></div><dl id="score-detail"></dl><p id="pair-count" class="fine"></p><p class="fine">Harmony uses hue spacing; readability uses luminance contrast. These are design heuristics, not an objective beauty or accessibility certification.</p></section>
<section><h2>Layers</h2><div id="layers"></div><button id="clear" class="wide">Clear motif</button></section></aside>
<div class="workbench"><div class="workspace-heading"><div><h2>Motif workspace</h2><p id="tool-hint">Select a shape and drag to move it.</p></div><span class="dimensions">400 × 400</span></div><div class="editor-wrap"><canvas id="editor" width="400" height="400" aria-label="Motif drawing canvas" tabindex="0"></canvas></div>
<section class="pattern-section"><div class="workspace-heading"><h2>Live fabric</h2><div class="button-row"><select id="animation" aria-label="Animation mode"><option value="off">Still pattern</option><option value="rotate">Rotate motifs</option><option value="pulse">Pulse motifs</option></select><button id="play">Play</button></div></div><canvas id="pattern" width="720" height="480" aria-label="Generated repeating fabric"></canvas><div class="pattern-controls"><label>Rotation <output id="angle-out">0°</output><input id="angle" type="range" min="-180" max="180" value="0"></label><label>Scale <output id="scale-out">1×</output><input id="scale" type="range" min=".3" max="2" step=".05" value="1"></label><label>Tile spacing<input id="pitch" type="range" min="55" max="180" value="110"></label><label>Animation speed<input id="speed" type="range" min=".2" max="3" step=".1" value="1"></label><label class="check"><input id="reflect-x" type="checkbox"> Mirror X</label><label class="check"><input id="reflect-y" type="checkbox"> Mirror Y</label></div></section></div>
<aside class="preview"><div class="preview-tabs" role="group" aria-label="Preview mode"><button id="model-tab" aria-pressed="true">3D mannequin</button><button id="photo-tab" aria-pressed="false">Photo fitting</button></div><div id="model-panel"><h2>Your pattern, in three dimensions</h2><p class="subtle">A generated mannequin wearing a textured dress. Drag to orbit, scroll to zoom.</p><div id="model" class="model"></div><div class="button-row"><button id="reset-view">Reset view</button><button id="export-model">Export 3D view</button></div><p class="fine">Procedurally generated geometry. A visual preview, not a garment measurement or cloth-physics simulation.</p></div>
<div id="photo-panel" hidden><h2>Fit a dress to your photograph</h2><button id="enlarge-photo" class="wide">Enlarge fitting view</button><p class="subtle">Choose a front-facing, full-body photo. We detect shoulders and hips, then you refine the fit.</p><label class="upload">Choose a photograph<input id="photo-file" type="file" accept="image/jpeg,image/png,image/webp" hidden><span>JPG, PNG or WebP · up to 20 MB</span></label><div class="photo-stage"><div id="photo-empty">Your photograph stays on this device. The dress overlay appears here after you select an image.</div><canvas id="photo" width="640" height="800" aria-label="Photograph with dress alignment handles" tabindex="0" hidden></canvas></div><div class="button-row"><button id="detect" disabled>Detect person again</button><button id="manual" disabled>Reset manual fit</button></div><p class="fine">Drag the six blue handles to fit the shoulders, natural waist and hem. This adds a new dress; it does not erase existing clothing outside its outline.</p><fieldset id="alignment" disabled><label>Alignment handle<select id="anchor-select">${["Left shoulder", "Right shoulder", "Right waist", "Right hem", "Left hem", "Left waist"].map((n, i) => `<option value="${i}">${i + 1}. ${n}</option>`).join("")}</select></label><div class="pair"><label>X position (%)<input id="anchor-x" type="number" min="0" max="100" step=".5"></label><label>Y position (%)<input id="anchor-y" type="number" min="0" max="100" step=".5"></label></div><div class="pair"><label>Neckline<select id="neckline"><option value="round">Round</option><option value="v">V-neck</option><option value="boat">Boat</option></select></label><label>Neck depth<input id="neck-depth" type="range" min="8" max="40" value="20"></label></div><label>Print size<input id="print-size" type="range" min="35" max="200" value="100"></label><label class="check"><input id="cloth-shading" type="checkbox" checked> Soft garment shading</label><label>Overlay opacity<input id="opacity" type="range" min=".3" max="1" step=".05" value="1"></label><label class="check"><input id="arm-mask" type="checkbox"> Keep original forearms (includes sleeves)</label><h3>Correct hands & hair</h3><p class="fine">Protect photo reveals original pixels in front of the dress. Restore dress removes those brush marks. Use a small brush around hands, hair and accessories.</p><div class="button-row" role="group" aria-label="Photo correction tools"><button data-photo-tool="fit" aria-pressed="true">Fit handles</button><button data-photo-tool="protect" aria-pressed="false">Protect photo</button><button data-photo-tool="restore" aria-pressed="false">Restore dress</button></div><label>Correction brush size<input id="mask-size" type="range" min="5" max="70" value="20"></label><div class="button-row"><button id="mask-undo" disabled>Undo brush</button><button id="mask-clear" disabled>Clear corrections</button></div></fieldset><button id="export-photo" class="primary wide" disabled>Export fitted photograph</button></div></aside></main>
<footer><span>Made with geometry. Inspired by kitenge.</span><span>Local editing · C++ companion preserved</span></footer><div id="status" role="status" aria-live="polite">Ready. Draw a motif or choose a starting design.</div>`;
const editor = $("#editor"),
  pattern = $("#pattern"),
  photoCanvas = $("#photo");
function status(t, error = false) {
  $("#status").textContent = t;
  $("#status").classList.toggle("error", error);
}
// File inputs stay private; make their visual labels usable from the keyboard.
document.querySelectorAll('label:has(input[type="file"])').forEach((label) => {
  label.tabIndex = 0;
  label.setAttribute("role", "button");
  label.onkeydown = (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      label.querySelector("input").click();
    }
  };
});
function remember(before = snapshot()) {
  past.push(before);
  if (past.length > 40) past.shift();
  future = [];
}
function change(fn) {
  remember();
  fn();
  dirty = true;
  sync();
}
const colourName = (i) =>
  i === 0 ? "Background" : i === 1 ? "Primary" : `Accent ${i - 1}`;
function paletteUI() {
  $("#palette").replaceChildren(
    ...project.palette.map((hex, i) => {
      const box = document.createElement("div");
      box.className = "colour-slot";
      box.innerHTML = `<label>${colourName(i)}<input aria-label="${colourName(i)} colour" type="color" value="${hex}" data-color="${i}"></label><input aria-label="${colourName(i)} hex" type="text" value="${hex}" maxlength="7" pattern="#[a-fA-F0-9]{6}" spellcheck="false">`;
      const [picker, text] = box.querySelectorAll("input");
      picker.onchange = () => change(() => (project.palette[i] = picker.value));
      text.onchange = () => {
        if (!/^#[a-f\d]{6}$/i.test(text.value)) {
          text.value = project.palette[i];
          return status("Enter a six-digit hex colour, such as #31553c.", true);
        }
        change(() => (project.palette[i] = text.value.toLowerCase()));
      };
      return box;
    }),
  );
  $("#drawing-role").replaceChildren(
    ...project.palette.map((hex, i) => {
      const o = document.createElement("option");
      o.value = i;
      o.textContent = `${colourName(i)} · ${hex}`;
      return o;
    }),
  );
  $("#drawing-role").value = role;
  $("#add-accent").disabled = project.palette.length >= 32;
  $("#remove-accent").disabled = role < 2 || project.palette.length <= 3;
  swatchUI();
}
function swatchUI() {
  $("#swatch-target").textContent =
    `Apply a swatch to ${colourName(role)}. Or enter any hex colour above.`;
  $("#swatches").replaceChildren(
    ...swatches
      .filter(
        (s) =>
          $("#swatch-family").value === "all" ||
          s.family === $("#swatch-family").value,
      )
      .map((s) => {
        const b = document.createElement("button");
        b.style.backgroundColor = s.hex;
        b.title = `${s.name} · ${s.hex}`;
        b.setAttribute("aria-label", b.title);
        b.setAttribute(
          "aria-pressed",
          project.palette[role].toLowerCase() === s.hex.toLowerCase(),
        );
        b.onclick = () => change(() => (project.palette[role] = s.hex));
        return b;
      }),
  );
}
function themedColours(colors) {
  return [...colors, ...project.palette.slice(colors.length)];
}
function paintThumbnails() {
  const source = document.createElement("canvas");
  source.width = source.height = 400;
  document.querySelectorAll("[data-preset]").forEach((b) => {
    paintMotif(source, { ...project, shapes: motif(b.dataset.preset) });
    const c = b.querySelector("canvas").getContext("2d");
    c.clearRect(0, 0, 72, 72);
    c.fillStyle = project.palette[0];
    c.fillRect(0, 0, 72, 72);
    c.drawImage(source, 75, 75, 250, 250, 0, 0, 72, 72);
  });
}
function sync() {
  const s = project.shapes.find((o) => o.id === selected);
  $("#undo").disabled = !past.length;
  $("#redo").disabled = !future.length;
  $("#delete").disabled = !s;
  $("#shape-fields").disabled = !s;
  $("#selection").textContent = s
    ? `Editing ${s.type}. Drag handles to resize, or drag inside to move. Shift keeps corner proportions.`
    : "Select a shape to edit its geometry.";
  if (s) {
    $("#shape-w").value = Math.round(s.w);
    $("#shape-h").value = Math.round(s.h);
    $("#shape-angle").value = s.rotation;
    $("#shape-angle-out").value = `${s.rotation}°`;
    $("#shape-stroke").value = s.stroke;
    $("#shape-fill").checked = s.filled;
    $("#shape-fill").disabled = s.type === "path";
  }
  if (role >= project.palette.length) role = 1;
  paletteUI();
  paintThumbnails();
  $("#angle").value = project.settings.angle;
  $("#angle-out").value = project.settings.angle + "°";
  $("#scale").value = project.settings.scale;
  $("#scale-out").value = project.settings.scale + "×";
  $("#pitch").value = project.settings.pitch;
  $("#speed").value = project.settings.speed;
  $("#animation").value = project.settings.mode;
  $("#reflect-x").checked = project.settings.reflectX;
  $("#reflect-y").checked = project.settings.reflectY;
  const score = paletteScore(project.palette);
  $("#score").textContent = score.total;
  $("#scheme").textContent = `${score.scheme} hue relationship`;
  $("#score-detail").innerHTML =
    `<dt>Hue harmony</dt><dd>${score.harmony}/100</dd><dt>Pattern readability</dt><dd>${score.readability}/100</dd>` +
    [...score.pairs]
      .sort((a, b) => a.ratio / a.target - b.ratio / b.target)
      .slice(0, 5)
      .map((p) => `<dt>${p.label}</dt><dd>${p.ratio.toFixed(2)}:1</dd>`)
      .join("");
  $("#pair-count").textContent =
    `All ${score.pairs.length} colour pairs scored; the lowest contrast pairs are shown.`;
  $("#layers").replaceChildren(
    ...[...project.shapes].reverse().map((o, i) => {
      const b = document.createElement("button");
      b.className = "layer";
      b.textContent = `${project.shapes.length - i}. ${o.type}`;
      b.setAttribute("aria-pressed", o.id === selected);
      b.onclick = () => {
        selected = o.id;
        role = o.role;
        tool = "select";
        toolUI();
        sync();
        dirty = true;
      };
      return b;
    }),
  );
}
function toolUI() {
  document
    .querySelectorAll("[data-tool]")
    .forEach((b) => b.setAttribute("aria-pressed", b.dataset.tool === tool));
  $("#tool-hint").textContent =
    tool === "select"
      ? "Drag a shape to move it. Drag its handles to resize; Shift keeps proportions."
      : tool === "brush"
        ? "Drag to draw a freehand stroke."
        : "Drag to size a new shape.";
  editor.style.cursor = tool === "select" ? "move" : "crosshair";
}
function point(e, canvas) {
  const r = canvas.getBoundingClientRect();
  return [
    ((e.clientX - r.left) * canvas.width) / r.width,
    ((e.clientY - r.top) * canvas.height) / r.height,
  ];
}
editor.onpointerdown = (e) => {
  if (e.button !== 0) return;
  e.preventDefault();
  editor.setPointerCapture(e.pointerId);
  const [x, y] = point(e, editor),
    before = snapshot();
  const current = project.shapes.find((s) => s.id === selected);
  const handle =
    tool === "select" &&
    current &&
    hitResizeHandle(
      current,
      x,
      y,
      Math.max(10, (10 * 400) / editor.getBoundingClientRect().width),
    );
  if (handle) {
    tool = "select";
    toolUI();
    drag = {
      kind: "resize",
      handle,
      original: structuredClone(current),
      before,
    };
    editor.style.cursor = resizeCursor(handle, current.rotation);
  } else if (tool === "select") {
    const s = [...project.shapes].reverse().find((s) => hitShape(s, x, y));
    selected = s?.id ?? null;
    if (s) role = s.role;
    drag = s ? { kind: "move", x, y, sx: s.x, sy: s.y, before } : null;
  } else {
    if (project.shapes.length >= 300)
      return status("This motif has reached the 300-layer limit.", true);
    const s = {
      id: crypto.randomUUID(),
      type:
        tool === "brush"
          ? "path"
          : ["triangle", "diamond"].includes(tool)
            ? "polygon"
            : tool,
      x,
      y,
      w: 8,
      h: 8,
      rotation: 0,
      role,
      stroke: +$("#shape-stroke").value,
      filled: tool !== "brush",
      points:
        tool === "brush"
          ? [[0, 0]]
          : tool === "triangle"
            ? [
                [0, -1],
                [1, 1],
                [-1, 1],
              ]
            : [
                [0, -1],
                [1, 0],
                [0, 1],
                [-1, 0],
              ],
    };
    project.shapes.push(s);
    selected = s.id;
    drag = { kind: tool === "brush" ? "brush" : "shape", x, y, before };
  }
  // Adding a layer changes the mobile page height. Defer the inspector rebuild
  // until pointer-up so the canvas cannot move underneath an active gesture.
  if (!drag) sync();
  dirty = true;
};
editor.onpointermove = (e) => {
  if (!drag) {
    const [x, y] = point(e, editor),
      s = project.shapes.find((s) => s.id === selected);
    const handle =
      tool === "select" &&
      s &&
      hitResizeHandle(
        s,
        x,
        y,
        Math.max(10, (10 * 400) / editor.getBoundingClientRect().width),
      );
    editor.style.cursor = handle
      ? resizeCursor(handle, s.rotation)
      : tool === "select"
        ? "move"
        : "crosshair";
    return;
  }
  const [px, py] = point(e, editor),
    x = Math.max(0, Math.min(400, px)),
    y = Math.max(0, Math.min(400, py)),
    s = project.shapes.find((s) => s.id === selected);
  if (!s) return;
  if (drag.kind === "resize") {
    Object.assign(
      s,
      resizeFromHandle(drag.original, drag.handle, x, y, e.shiftKey),
    );
    $("#shape-w").value = Math.round(s.w);
    $("#shape-h").value = Math.round(s.h);
  } else if (drag.kind === "move") {
    s.x = Math.max(0, Math.min(400, drag.sx + x - drag.x));
    s.y = Math.max(0, Math.min(400, drag.sy + y - drag.y));
  } else if (drag.kind === "brush") {
    if (s.points.length < 5000) s.points.push([x - s.x, y - s.y]);
    const xs = s.points.map((p) => p[0]),
      ys = s.points.map((p) => p[1]);
    s.w = Math.max(8, Math.max(...xs) - Math.min(...xs));
    s.h = Math.max(8, Math.max(...ys) - Math.min(...ys));
  } else {
    s.x = (x + drag.x) / 2;
    s.y = (y + drag.y) / 2;
    s.w = Math.max(8, Math.abs(x - drag.x));
    s.h = Math.max(8, Math.abs(y - drag.y));
  }
  dirty = true;
};
function endDraw() {
  if (!drag) return;
  const s = project.shapes.find((s) => s.id === selected);
  if (s?.type === "path" && drag.kind === "brush") {
    if (s.points.length === 1) s.points.push([0.1, 0.1]);
    const minX = Math.min(...s.points.map((p) => p[0])),
      maxX = Math.max(...s.points.map((p) => p[0])),
      minY = Math.min(...s.points.map((p) => p[1])),
      maxY = Math.max(...s.points.map((p) => p[1]));
    const cx = (minX + maxX) / 2,
      cy = (minY + maxY) / 2;
    s.points = s.points.map(([x, y]) => [x - cx, y - cy]);
    s.x += cx;
    s.y += cy;
  }
  if (snapshot() !== drag.before) remember(drag.before);
  if (drag.kind === "shape") {
    tool = "select";
    toolUI();
  }
  drag = null;
  sync();
  dirty = true;
}
editor.onpointerup = endDraw;
editor.onpointercancel = endDraw;
document.querySelectorAll("[data-tool]").forEach(
  (b) =>
    (b.onclick = () => {
      tool = b.dataset.tool;
      toolUI();
    }),
);
document.querySelectorAll("[data-preset]").forEach(
  (b) =>
    (b.onclick = () =>
      change(() => {
        project.shapes = motif(b.dataset.preset);
        selected = null;
      })),
);
$("#undo").onclick = () => {
  if (!past.length) return;
  future.push(snapshot());
  project = JSON.parse(past.pop());
  selected = null;
  dirty = true;
  sync();
};
$("#redo").onclick = () => {
  if (!future.length) return;
  past.push(snapshot());
  project = JSON.parse(future.pop());
  selected = null;
  dirty = true;
  sync();
};
$("#delete").onclick = () =>
  change(() => {
    project.shapes = project.shapes.filter((s) => s.id !== selected);
    selected = null;
  });
$("#clear").onclick = () =>
  change(() => {
    project.shapes = [];
    selected = null;
  });
for (const [id, key] of [
  ["shape-w", "w"],
  ["shape-h", "h"],
  ["shape-angle", "rotation"],
  ["shape-stroke", "stroke"],
  ["shape-fill", "filled"],
])
  $("#" + id).onchange = () => {
    const s = project.shapes.find((o) => o.id === selected);
    if (!s) return;
    const v = key === "filled" ? $("#" + id).checked : +$("#" + id).value;
    if (
      typeof v === "number" &&
      (!Number.isFinite(v) ||
        Math.abs(v) > 380 ||
        (["w", "h", "stroke"].includes(key) && v < 1))
    )
      return sync();
    change(() => {
      if (s.type === "path" && ["w", "h"].includes(key)) {
        const ratio = v / s[key],
          index = key === "w" ? 0 : 1;
        s.points = s.points.map((p) =>
          p.map((n, i) => (i === index ? n * ratio : n)),
        );
      }
      s[key] = v;
    });
  };
$("#drawing-role").onchange = () => {
  role = +$("#drawing-role").value;
  if (selected)
    change(() => (project.shapes.find((s) => s.id === selected).role = role));
  else paletteUI();
};
$("#swatch-family").onchange = swatchUI;
$("#add-accent").onclick = () =>
  change(() => {
    if (project.palette.length >= 32) return;
    project.palette.push(
      swatches[(project.palette.length * 7) % swatches.length].hex,
    );
    role = project.palette.length - 1;
    status(
      `Added ${colourName(role)}. Select it as the drawing colour or recolour a selected shape.`,
    );
  });
$("#remove-accent").onclick = () =>
  change(() => {
    if (role < 2 || project.palette.length <= 3) return;
    project.shapes.forEach((s) => {
      if (s.role === role) s.role = 1;
      else if (s.role > role) s.role--;
    });
    project.palette.splice(role, 1);
    role = 1;
    status(
      "Accent removed. Shapes that used it now use Primary. Undo restores the accent.",
    );
  });
$("#palette-preset").onchange = () => {
  const index = $("#palette-preset").value;
  if (index === "") return;
  change(() => {
    project.palette = themedColours(paletteCatalog[+index].colors);
    status(
      `${paletteCatalog[+index].name} theme applied. Extra accents are retained.`,
    );
  });
  $("#palette-preset").value = "";
};
$("#suggest").onclick = () =>
  change(() => {
    const choices = palettes
      .map((p) => themedColours(p))
      .filter((p) => p.join() !== project.palette.join())
      .map((p) => ({ p, score: paletteScore(p).total }))
      .sort((a, b) => b.score - a.score);
    project.palette = choices[0].p;
    status(
      "Suggested theme selected. Compare the harmony and contrast breakdown. Extra accents are retained.",
    );
  });
for (const [id, key] of [
  ["angle", "angle"],
  ["scale", "scale"],
  ["pitch", "pitch"],
  ["speed", "speed"],
  ["animation", "mode"],
  ["reflect-x", "reflectX"],
  ["reflect-y", "reflectY"],
])
  $("#" + id).onchange = () =>
    change(() => {
      project.settings[key] =
        id === "animation"
          ? $("#" + id).value
          : id.startsWith("reflect")
            ? $("#" + id).checked
            : +$("#" + id).value;
      time = 0;
    });
$("#play").onclick = () => {
  if (project.settings.mode === "off") {
    project.settings.mode = "rotate";
    $("#animation").value = "rotate";
  }
  playing = !playing;
  $("#play").textContent = playing ? "Pause" : "Play";
  dirty = true;
};
function download(data, name) {
  const a = document.createElement("a");
  a.href = data;
  a.download = name;
  a.click();
}
$("#save").onclick = () => {
  const blob = new Blob([JSON.stringify(project, null, 2)], {
      type: "application/json",
    }),
    url = URL.createObjectURL(blob);
  download(url, "kitenge-project.json");
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  status("Project saved. Photos are not included in the project file.");
};
$("#open").onchange = async (e) => {
  try {
    const f = e.target.files[0];
    if (!f) return;
    if (f.size > 4e6) throw Error("Project file must be smaller than 4 MB.");
    const p = validateProject(JSON.parse(await f.text()));
    remember();
    project = p;
    selected = null;
    time = 0;
    playing = false;
    $("#play").textContent = "Play";
    dirty = true;
    sync();
    status("Project opened.");
  } catch (err) {
    status(err.message, true);
  } finally {
    e.target.value = "";
  }
};
$("#export").onclick = () => {
  renderPattern(pattern, project, time);
  download(pattern.toDataURL("image/png"), "kitenge-fabric.png");
  status("Fabric PNG exported.");
};
function switchMode(next) {
  mode = next;
  $("#model-panel").hidden = next !== "model";
  $("#photo-panel").hidden = next !== "photo";
  $("#model-tab").setAttribute("aria-pressed", next === "model");
  $("#photo-tab").setAttribute("aria-pressed", next === "photo");
  if (next === "model") {
    $("main").classList.remove("fitting-focus");
    $("#enlarge-photo").textContent = "Enlarge fitting view";
  }
  dirty = true;
}
$("#model-tab").onclick = () => switchMode("model");
$("#photo-tab").onclick = () => switchMode("photo");
$("#enlarge-photo").onclick = () => {
  const expanded = $("main").classList.toggle("fitting-focus");
  $("#enlarge-photo").textContent = expanded
    ? "Return to design workspace"
    : "Enlarge fitting view";
  $("#photo-panel").scrollIntoView({ block: "start" });
};
$("#reset-view").onclick = () => model?.reset();
$("#export-model").onclick = () => {
  if (model) download(model.export(), "kitenge-3d-preview.png");
};
function anchorFields() {
  const [x, y] = anchors[+$("#anchor-select").value];
  $("#anchor-x").value = (x * 100).toFixed(1);
  $("#anchor-y").value = (y * 100).toFixed(1);
}
function paintPhoto(handles = true) {
  renderRepeat(repeatTile, project, time);
  drawPhoto(photoCanvas, photo, repeatTile, anchors, {
    handles,
    opacity: +$("#opacity").value,
    landmarks,
    arms: $("#arm-mask").checked,
    mask: photoMask,
    printScale: +$("#print-size").value / 100,
    neckline: $("#neckline").value,
    neckDepth: +$("#neck-depth").value / 100,
    shading: $("#cloth-shading").checked,
  });
}
async function autodetect(token = photoToken) {
  if (!photo) return;
  const img = photo;
  $("#detect").disabled = true;
  status("Detecting shoulders and hips locally. Please wait…");
  try {
    await new Promise((r) => setTimeout(r, 50));
    const result = await detectPose(img);
    if (token !== photoToken) return;
    anchors = result.anchors;
    landmarks = result.landmarks;
    status("Person detected. Drag the six handles to refine the fit.");
  } catch (e) {
    if (token === photoToken)
      status(
        e.message.includes("Shoulders") || e.message.includes("person")
          ? e.message
          : "Automatic alignment is unavailable. Adjust the six handles manually or try detection again.",
        true,
      );
  } finally {
    if (token === photoToken) {
      $("#detect").disabled = false;
      anchorFields();
      dirty = true;
    }
  }
}
$("#photo-file").onchange = async (e) => {
  const f = e.target.files[0];
  if (!f) return;
  const token = ++photoToken;
  try {
    if (
      !["image/jpeg", "image/png", "image/webp"].includes(f.type) ||
      f.size > 20 * 1024 * 1024
    )
      throw Error("Choose a JPG, PNG or WebP smaller than 20 MB.");
    const bitmap = await createImageBitmap(f, {
      imageOrientation: "from-image",
    });
    if (token !== photoToken) {
      bitmap.close();
      return;
    }
    const factor = Math.min(1, 1600 / Math.max(bitmap.width, bitmap.height));
    const img = document.createElement("canvas");
    img.width = Math.round(bitmap.width * factor);
    img.height = Math.round(bitmap.height * factor);
    img.getContext("2d").drawImage(bitmap, 0, 0, img.width, img.height);
    bitmap.close();
    photo = img;
    photoCanvas.width = img.width;
    photoCanvas.height = img.height;
    photoMask.width = img.width;
    photoMask.height = img.height;
    maskStrokes = [];
    maskDrag = null;
    maskButtons();
    anchors = manualAnchors();
    landmarks = null;
    $("#photo-empty").hidden = true;
    photoCanvas.hidden = false;
    $("#alignment").disabled = false;
    $("#manual").disabled = false;
    $("#export-photo").disabled = false;
    anchorFields();
    paintPhoto();
    await autodetect(token);
  } catch (err) {
    if (token === photoToken) {
      $("#detect").disabled = !photo;
      status(err.message, true);
    }
  } finally {
    e.target.value = "";
  }
};
$("#detect").onclick = () => autodetect();
$("#manual").onclick = () => {
  cancelDetection();
  anchors = manualAnchors();
  landmarks = null;
  anchorFields();
  dirty = true;
  status("Manual fit restored. Drag handles or enter their positions.");
};
$("#anchor-select").onchange = anchorFields;
for (const [id, index] of [
  ["anchor-x", 0],
  ["anchor-y", 1],
])
  $("#" + id).onchange = () => {
    const v = +$("#" + id).value;
    if (!Number.isFinite(v) || v < 0 || v > 100) return anchorFields();
    cancelDetection();
    anchors[+$("#anchor-select").value][index] = v / 100;
    dirty = true;
  };
$("#opacity").oninput = () => (dirty = true);
$("#arm-mask").onchange = () => (dirty = true);
for (const id of ["neckline", "neck-depth", "print-size", "cloth-shading"])
  $("#" + id).oninput = () => (dirty = true);
function cancelDetection() {
  if ($("#detect").disabled && photo) {
    photoToken++;
    $("#detect").disabled = false;
    status(
      "Manual correction is active. Use Detect person again to replace the fit.",
    );
  }
}
function maskButtons() {
  $("#mask-undo").disabled = $("#mask-clear").disabled = !maskStrokes.length;
}
document.querySelectorAll("[data-photo-tool]").forEach(
  (b) =>
    (b.onclick = () => {
      photoTool = b.dataset.photoTool;
      document
        .querySelectorAll("[data-photo-tool]")
        .forEach((e) => e.setAttribute("aria-pressed", e === b));
      photoCanvas.style.cursor = photoTool === "fit" ? "default" : "crosshair";
      status(
        photoTool === "fit"
          ? "Drag a blue alignment handle."
          : photoTool === "protect"
            ? "Brush over the original hands or hair to bring those pixels in front of the dress."
            : "Brush to remove protected photo pixels and restore the dress.",
      );
    }),
);
$("#mask-undo").onclick = () => {
  maskStrokes.pop();
  rebuildMask(photoMask, maskStrokes);
  maskButtons();
  dirty = true;
};
$("#mask-clear").onclick = () => {
  maskStrokes = [];
  rebuildMask(photoMask, maskStrokes);
  maskButtons();
  dirty = true;
};
let photoDrag = null;
photoCanvas.onpointerdown = (e) => {
  if (e.button !== 0 || !photo) return;
  e.preventDefault();
  photoCanvas.focus();
  const [x, y] = point(e, photoCanvas);
  if (photoTool !== "fit") {
    if (maskStrokes.length >= 200)
      return status(
        "Undo or clear correction strokes before adding more.",
        true,
      );
    cancelDetection();
    maskDrag = {
      mode: photoTool,
      size: +$("#mask-size").value / 800,
      points: [[x / photoCanvas.width, y / photoCanvas.height]],
    };
    maskStrokes.push(maskDrag);
    paintMaskStroke(photoMask, maskDrag);
    maskButtons();
    photoCanvas.setPointerCapture(e.pointerId);
    dirty = true;
    return;
  }
  let best = Infinity;
  anchors.forEach(([ax, ay], i) => {
    const dist = Math.hypot(
      x - ax * photoCanvas.width,
      y - ay * photoCanvas.height,
    );
    if (dist < best && dist < photoCanvas.width * 0.06) {
      best = dist;
      photoDrag = i;
    }
  });
  if (photoDrag !== null) {
    cancelDetection();
    photoCanvas.setPointerCapture(e.pointerId);
    $("#anchor-select").value = photoDrag;
    anchorFields();
  }
};
photoCanvas.onpointermove = (e) => {
  if (maskDrag) {
    if (maskDrag.points.length >= 2000) return;
    const [x, y] = point(e, photoCanvas),
      last = maskDrag.points.at(-1),
      next = [
        Math.max(0, Math.min(1, x / photoCanvas.width)),
        Math.max(0, Math.min(1, y / photoCanvas.height)),
      ];
    maskDrag.points.push(next);
    paintMaskStroke(photoMask, { ...maskDrag, points: [last, next] });
    dirty = true;
    return;
  }
  if (photoDrag === null) return;
  const [x, y] = point(e, photoCanvas);
  anchors[photoDrag] = [
    Math.max(0, Math.min(1, x / photoCanvas.width)),
    Math.max(0, Math.min(1, y / photoCanvas.height)),
  ];
  anchorFields();
  dirty = true;
};
photoCanvas.onpointerup = photoCanvas.onpointercancel = () => {
  photoDrag = null;
  maskDrag = null;
};
$("#export-photo").onclick = () => {
  paintPhoto(false);
  download(photoCanvas.toDataURL("image/png"), "kitenge-photo-fitting.png");
  paintPhoto();
  status("Photo exported without alignment handles.");
};
document.addEventListener("keydown", (e) => {
  if (["INPUT", "SELECT", "TEXTAREA"].includes(document.activeElement.tagName))
    return;
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
    e.preventDefault();
    if (
      mode === "photo" &&
      document.activeElement === photoCanvas &&
      !e.shiftKey
    )
      $("#mask-undo").click();
    else $(e.shiftKey ? "#redo" : "#undo").click();
  }
  if (e.key === "Delete" || e.key === "Backspace") {
    e.preventDefault();
    if (selected) $("#delete").click();
  }
});
try {
  model = createModel($("#model"), pattern, (t) => status(t, true));
  if (!model) {
    $("#export-model").disabled = true;
    $("#reset-view").disabled = true;
  }
} catch (e) {
  status(
    "3D preview could not initialize. Fabric and photo editing remain available.",
    true,
  );
}
function tick(now) {
  const dt = Math.min((now - last) / 1000, 0.1);
  last = now;
  if (playing) {
    time += dt;
    dirty = true;
  }
  if (dirty) {
    paintMotif(editor, project, true, tool === "select" ? selected : null);
    renderPattern(pattern, project, time);
    model?.update();
    if (photo) paintPhoto();
    dirty = false;
  }
  requestAnimationFrame(tick);
}
sync();
requestAnimationFrame(tick);
window.addEventListener("beforeunload", () => model?.dispose());
