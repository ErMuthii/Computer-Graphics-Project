import "./style.css";
import {
  defaults,
  motif,
  palettes,
  paintMotif,
  renderPattern,
  hitShape,
  paletteScore,
  validateProject,
} from "./core.js";
import { createModel } from "./model.js";
import { detectPose, drawPhoto, manualAnchors } from "./photo.js";

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
const $ = (s) => document.querySelector(s),
  snapshot = () => JSON.stringify(project);
const app = $("#app");
app.innerHTML = `
<header><a class="wordmark" href="./">Kitenge<span>Pattern Studio</span></a><p>One motif. A fabric of possibilities.</p><div class="file-actions"><button id="save">Save project</button><label class="button">Open project<input id="open" type="file" accept=".json,application/json" hidden></label><button class="primary" id="export">Export PNG</button></div></header>
<main><aside class="inspector"><h1>Your design</h1><p class="subtle">Create a motif, then see it become cloth.</p><section><h2>Starting motif</h2><div class="button-row" id="presets"><button data-preset="diamond">Diamond</button><button data-preset="flower">Flower</button><button data-preset="steps">Steps</button></div></section>
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
<section><h2>Palette & drawing colour</h2><div class="palette">${["Background", "Primary", "Accent"].map((name, i) => `<label>${name}<input aria-label="${name} colour" type="color" data-color="${i}" value="${project.palette[i]}"></label>`).join("")}</div><div class="button-row" id="roles">${["Background", "Primary", "Accent"].map((n, i) => `<button data-role="${i}" aria-pressed="${i === 1}">${n}</button>`).join("")}</div><button id="suggest" class="wide">Suggest a harmonious palette</button><div class="score"><strong id="score">—</strong><div><b>Palette balance</b><span id="scheme"></span></div></div><dl id="score-detail"></dl><p class="fine">An explainable design heuristic, not an objective beauty or accessibility certification. Harmony uses hue spacing; readability uses luminance contrast.</p></section>
<section><h2>Layers</h2><div id="layers"></div><button id="clear" class="wide">Clear motif</button></section></aside>
<div class="workbench"><div class="workspace-heading"><div><h2>Motif workspace</h2><p id="tool-hint">Select a shape and drag to move it.</p></div><span class="dimensions">400 × 400</span></div><div class="editor-wrap"><canvas id="editor" width="400" height="400" aria-label="Motif drawing canvas" tabindex="0"></canvas></div>
<section class="pattern-section"><div class="workspace-heading"><h2>Live fabric</h2><div class="button-row"><select id="animation" aria-label="Animation mode"><option value="off">Still pattern</option><option value="rotate">Rotate motifs</option><option value="pulse">Pulse motifs</option></select><button id="play">Play</button></div></div><canvas id="pattern" width="720" height="480" aria-label="Generated repeating fabric"></canvas><div class="pattern-controls"><label>Rotation <output id="angle-out">0°</output><input id="angle" type="range" min="-180" max="180" value="0"></label><label>Scale <output id="scale-out">1×</output><input id="scale" type="range" min=".3" max="2" step=".05" value="1"></label><label>Tile spacing<input id="pitch" type="range" min="55" max="180" value="110"></label><label>Animation speed<input id="speed" type="range" min=".2" max="3" step=".1" value="1"></label><label class="check"><input id="reflect-x" type="checkbox"> Mirror X</label><label class="check"><input id="reflect-y" type="checkbox"> Mirror Y</label></div></section></div>
<aside class="preview"><div class="preview-tabs" role="group" aria-label="Preview mode"><button id="model-tab" aria-pressed="true">3D mannequin</button><button id="photo-tab" aria-pressed="false">Photo fitting</button></div><div id="model-panel"><h2>Your pattern, in three dimensions</h2><p class="subtle">A generated mannequin wearing a textured dress. Drag to orbit, scroll to zoom.</p><div id="model" class="model"></div><div class="button-row"><button id="reset-view">Reset view</button><button id="export-model">Export 3D view</button></div><p class="fine">Procedurally generated geometry. A visual preview, not a garment measurement or cloth-physics simulation.</p></div>
<div id="photo-panel" hidden><h2>Fit a dress to your photograph</h2><p class="subtle">Choose a front-facing, full-body photo. We detect shoulders and hips, then you refine the fit.</p><label class="upload">Choose a photograph<input id="photo-file" type="file" accept="image/jpeg,image/png,image/webp" hidden><span>JPG, PNG or WebP · up to 20 MB</span></label><div id="photo-empty">Your photograph stays on this device. The dress overlay appears here after you select an image.</div><canvas id="photo" width="640" height="800" aria-label="Photograph with dress alignment handles" tabindex="0" hidden></canvas><div class="button-row"><button id="detect" disabled>Detect person again</button><button id="manual" disabled>Reset manual fit</button></div><p class="fine">Automatic pose detection estimates placement. Drag the six blue handles to correct the dress. Existing clothing may remain visible outside the new silhouette.</p><fieldset id="alignment" disabled><label>Alignment handle<select id="anchor-select">${["Left shoulder", "Right shoulder", "Right waist", "Right hem", "Left hem", "Left waist"].map((n, i) => `<option value="${i}">${i + 1}. ${n}</option>`).join("")}</select></label><div class="pair"><label>X position (%)<input id="anchor-x" type="number" min="0" max="100" step=".5"></label><label>Y position (%)<input id="anchor-y" type="number" min="0" max="100" step=".5"></label></div><label>Overlay opacity<input id="opacity" type="range" min=".3" max="1" step=".05" value="1"></label><label class="check"><input id="arm-mask" type="checkbox" checked> Keep detected forearms in front</label></fieldset><button id="export-photo" class="primary wide" disabled>Export fitted photograph</button></div></aside></main>
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
function sync() {
  const s = project.shapes.find((o) => o.id === selected);
  $("#undo").disabled = !past.length;
  $("#redo").disabled = !future.length;
  $("#delete").disabled = !s;
  $("#shape-fields").disabled = !s;
  $("#selection").textContent = s
    ? `Editing ${s.type}. Drag on the canvas to reposition.`
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
  document
    .querySelectorAll("[data-color]")
    .forEach((e) => (e.value = project.palette[+e.dataset.color]));
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
    `<dt>Hue harmony</dt><dd>${score.harmony}/100</dd><dt>Pattern readability</dt><dd>${score.readability}/100</dd><dt>Primary / background</dt><dd>${score.ratios[0].toFixed(2)}:1</dd><dt>Accent / background</dt><dd>${score.ratios[1].toFixed(2)}:1</dd><dt>Primary / accent</dt><dd>${score.ratios[2].toFixed(2)}:1</dd>`;
  $("#layers").replaceChildren(
    ...[...project.shapes].reverse().map((o, i) => {
      const b = document.createElement("button");
      b.className = "layer";
      b.textContent = `${project.shapes.length - i}. ${o.type}`;
      b.setAttribute("aria-pressed", o.id === selected);
      b.onclick = () => {
        selected = o.id;
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
      ? "Select a shape and drag to move it."
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
  if (tool === "select") {
    const s = [...project.shapes].reverse().find((s) => hitShape(s, x, y));
    selected = s?.id ?? null;
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
  sync();
  dirty = true;
};
editor.onpointermove = (e) => {
  if (!drag) return;
  const [px, py] = point(e, editor),
    x = Math.max(0, Math.min(400, px)),
    y = Math.max(0, Math.min(400, py)),
    s = project.shapes.find((s) => s.id === selected);
  if (!s) return;
  if (drag.kind === "move") {
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
  if (s?.type === "path") {
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
document.querySelectorAll("[data-role]").forEach(
  (b) =>
    (b.onclick = () => {
      role = +b.dataset.role;
      document
        .querySelectorAll("[data-role]")
        .forEach((e) =>
          e.setAttribute("aria-pressed", +e.dataset.role === role),
        );
      if (selected)
        change(() => {
          project.shapes.find((s) => s.id === selected).role = role;
        });
    }),
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
document
  .querySelectorAll("[data-color]")
  .forEach(
    (e) =>
      (e.onchange = () =>
        change(() => (project.palette[+e.dataset.color] = e.value))),
  );
$("#suggest").onclick = () =>
  change(() => {
    const choices = palettes
      .map((p) => ({ p, score: paletteScore(p).total }))
      .filter((x) => x.p.join() !== project.palette.join())
      .sort((a, b) => b.score - a.score);
    project.palette = [...choices[0].p];
    status(
      "Suggested palette selected. Review the harmony and contrast breakdown.",
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
  dirty = true;
}
$("#model-tab").onclick = () => switchMode("model");
$("#photo-tab").onclick = () => switchMode("photo");
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
  drawPhoto(photoCanvas, photo, pattern, anchors, {
    handles,
    opacity: +$("#opacity").value,
    landmarks,
    arms: $("#arm-mask").checked,
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
    anchors[+$("#anchor-select").value][index] = v / 100;
    dirty = true;
  };
$("#opacity").oninput = () => (dirty = true);
$("#arm-mask").onchange = () => (dirty = true);
let photoDrag = null;
photoCanvas.onpointerdown = (e) => {
  const [x, y] = point(e, photoCanvas);
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
    photoCanvas.setPointerCapture(e.pointerId);
    $("#anchor-select").value = photoDrag;
    anchorFields();
  }
};
photoCanvas.onpointermove = (e) => {
  if (photoDrag === null) return;
  const [x, y] = point(e, photoCanvas);
  anchors[photoDrag] = [
    Math.max(0, Math.min(1, x / photoCanvas.width)),
    Math.max(0, Math.min(1, y / photoCanvas.height)),
  ];
  anchorFields();
  dirty = true;
};
photoCanvas.onpointerup = photoCanvas.onpointercancel = () =>
  (photoDrag = null);
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
    $(e.shiftKey ? "#redo" : "#undo").click();
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
    paintMotif(editor, project, true, selected);
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
