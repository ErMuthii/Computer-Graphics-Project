import { createMotif, paletteCatalog } from "./catalog.js";
import { resizeHandles } from "./selection.js";
export const palettes = paletteCatalog.map((p) => p.colors);
export const motif = createMotif;
export function defaults() {
  return {
    version: 1,
    palette: [...palettes[0]],
    settings: {
      angle: 0,
      scale: 1,
      reflectX: false,
      reflectY: false,
      pitch: 110,
      mode: "off",
      speed: 1,
    },
    shapes: motif("diamond"),
  };
}
export function shapePath(s) {
  const p = new Path2D();
  if (s.type === "rectangle") p.rect(-s.w / 2, -s.h / 2, s.w, s.h);
  else if (s.type === "ellipse")
    p.ellipse(0, 0, s.w / 2, s.h / 2, 0, 0, Math.PI * 2);
  else if (s.type === "polygon") {
    s.points.forEach(([x, y], i) =>
      i
        ? p.lineTo((x * s.w) / 2, (y * s.h) / 2)
        : p.moveTo((x * s.w) / 2, (y * s.h) / 2),
    );
    p.closePath();
  } else {
    s.points.forEach(([x, y], i) => (i ? p.lineTo(x, y) : p.moveTo(x, y)));
  }
  return p;
}
export function paintShape(ctx, s, palette) {
  ctx.save();
  ctx.translate(s.x, s.y);
  ctx.rotate((s.rotation * Math.PI) / 180);
  const p = shapePath(s);
  ctx.fillStyle = palette[s.role];
  ctx.strokeStyle = palette[s.role];
  ctx.lineWidth = s.stroke;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  if (s.type === "path" || !s.filled) ctx.stroke(p);
  else ctx.fill(p);
  ctx.restore();
}
export function paintMotif(canvas, project, editor = false, selected = null) {
  const c = canvas.getContext("2d");
  c.clearRect(0, 0, 400, 400);
  if (editor) {
    c.fillStyle = project.palette[0];
    c.fillRect(0, 0, 400, 400);
    c.strokeStyle = "#00000012";
    c.lineWidth = 1;
    for (let x = 0; x <= 400; x += 25) {
      c.beginPath();
      c.moveTo(x, 0);
      c.lineTo(x, 400);
      c.moveTo(0, x);
      c.lineTo(400, x);
      c.stroke();
    }
  }
  project.shapes.forEach((s) => paintShape(c, s, project.palette));
  if (editor && selected) {
    const s = project.shapes.find((x) => x.id === selected);
    if (s) {
      c.save();
      c.translate(s.x, s.y);
      c.rotate((s.rotation * Math.PI) / 180);
      c.strokeStyle = "#155dcc";
      c.lineWidth = 1.5;
      c.setLineDash([5, 4]);
      c.strokeRect(-s.w / 2, -s.h / 2, s.w, s.h);
      c.restore();
      c.save();
      c.setLineDash([]);
      c.lineWidth = 1.5;
      c.strokeStyle = "#155dcc";
      c.fillStyle = "#fff";
      resizeHandles(s).forEach((h) => {
        c.fillRect(h.x - 4.5, h.y - 4.5, 9, 9);
        c.strokeRect(h.x - 4.5, h.y - 4.5, 9, 9);
      });
      c.restore();
    }
  }
}
export function renderPattern(canvas, project, time = 0) {
  const c = canvas.getContext("2d"),
    s = project.settings;
  const tile = document.createElement("canvas");
  tile.width = 400;
  tile.height = 400;
  paintMotif(tile, project);
  c.fillStyle = project.palette[0];
  c.fillRect(0, 0, canvas.width, canvas.height);
  const angle =
      ((s.angle + (s.mode === "rotate" ? time * 30 * s.speed : 0)) * Math.PI) /
      180,
    scale =
      s.scale *
      (s.mode === "pulse" ? 1 + 0.18 * Math.sin(time * s.speed * 2) : 1);
  for (let y = -s.pitch; y < canvas.height + s.pitch; y += s.pitch)
    for (let x = -s.pitch; x < canvas.width + s.pitch; x += s.pitch) {
      c.save();
      c.translate(x + s.pitch / 2, y + s.pitch / 2);
      c.rotate(angle);
      c.scale((s.reflectX ? -1 : 1) * scale, (s.reflectY ? -1 : 1) * scale);
      c.drawImage(tile, -s.pitch / 2, -s.pitch / 2, s.pitch, s.pitch);
      c.restore();
    }
}
// A single exact grid period avoids a seam when repeated across a photograph.
export function renderRepeat(canvas, project, time = 0, resolution = null) {
  const pitch = resolution ?? Math.round(project.settings.pitch);
  if (canvas.width !== pitch) canvas.width = canvas.height = pitch;
  renderPattern(
    canvas,
    { ...project, settings: { ...project.settings, pitch } },
    time,
  );
}
export function hitShape(s, x, y) {
  const a = (-s.rotation * Math.PI) / 180,
    dx = x - s.x,
    dy = y - s.y;
  const px = dx * Math.cos(a) - dy * Math.sin(a),
    py = dx * Math.sin(a) + dy * Math.cos(a);
  return Math.abs(px) <= s.w / 2 + 8 && Math.abs(py) <= s.h / 2 + 8;
}
function rgb(hex) {
  return hex.match(/[a-f\d]{2}/gi).map((v) => parseInt(v, 16) / 255);
}
export function luminance(hex) {
  return rgb(hex)
    .map((x) => (x <= 0.04045 ? x / 12.92 : ((x + 0.055) / 1.055) ** 2.4))
    .reduce((a, x, i) => a + x * [0.2126, 0.7152, 0.0722][i], 0);
}
export function contrast(a, b) {
  const x = luminance(a),
    y = luminance(b);
  return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05);
}
function hue(hex) {
  const [r, g, b] = rgb(hex),
    max = Math.max(r, g, b),
    min = Math.min(r, g, b),
    d = max - min;
  if (!d) return null;
  let h =
    max === r
      ? (g - b) / d + (g < b ? 6 : 0)
      : max === g
        ? (b - r) / d + 2
        : (r - g) / d + 4;
  return h * 60;
}
export function paletteScore(p) {
  const hs = p.map(hue).filter((x) => x !== null),
    pairs = [];
  for (let i = 0; i < hs.length; i++)
    for (let j = i + 1; j < hs.length; j++) {
      let d = Math.abs(hs[i] - hs[j]);
      pairs.push(Math.min(d, 360 - d));
    }
  const schemes = [
    { name: "Analogous", targets: [0, 30, 60] },
    { name: "Complementary", targets: [0, 180] },
    { name: "Triadic", targets: [0, 120] },
  ];
  const best = schemes
    .map((s) => ({
      ...s,
      error: pairs.length
        ? pairs.reduce(
            (sum, d) =>
              sum + Math.min(...s.targets.map((t) => Math.abs(d - t))),
            0,
          ) / pairs.length
        : 0,
    }))
    .sort((a, b) => a.error - b.error)[0];
  const harmony =
    hs.length < 2 ? 100 : Math.round(Math.max(0, 100 - best.error * 1.5));
  const name = (i) =>
    i === 0 ? "Background" : i === 1 ? "Primary" : `Accent ${i - 1}`;
  const pairsWithContrast = [];
  for (let i = 0; i < p.length; i++)
    for (let j = i + 1; j < p.length; j++) {
      pairsWithContrast.push({
        label: `${name(j)} / ${name(i)}`,
        ratio: contrast(p[i], p[j]),
        target: i === 0 ? 4.5 : 3,
      });
    }
  const ratios = pairsWithContrast.map((pair) => pair.ratio);
  const readability = Math.round(
    (pairsWithContrast.reduce(
      (sum, pair) => sum + Math.min(pair.ratio / pair.target, 1),
      0,
    ) /
      pairsWithContrast.length) *
      100,
  );
  return {
    harmony,
    readability,
    total: Math.round(harmony * 0.5 + readability * 0.5),
    scheme: hs.length < 2 ? "Neutral" : best.name,
    ratios,
    pairs: pairsWithContrast,
  };
}
export function validateProject(v) {
  if (
    !v ||
    v.version !== 1 ||
    !Array.isArray(v.palette) ||
    v.palette.length < 3 ||
    v.palette.length > 32 ||
    !v.palette.every((c) => /^#[a-f\d]{6}$/i.test(c)) ||
    !Array.isArray(v.shapes) ||
    v.shapes.length > 300
  )
    throw Error("Choose a valid Kitenge project JSON file.");
  const s = v.settings;
  if (
    !s ||
    !Number.isFinite(s.angle) ||
    Math.abs(s.angle) > 3600 ||
    !Number.isFinite(s.scale) ||
    s.scale < 0.3 ||
    s.scale > 2 ||
    !Number.isFinite(s.pitch) ||
    s.pitch < 55 ||
    s.pitch > 180 ||
    !["off", "rotate", "pulse"].includes(s.mode) ||
    !Number.isFinite(s.speed) ||
    s.speed < 0.2 ||
    s.speed > 3 ||
    typeof s.reflectX !== "boolean" ||
    typeof s.reflectY !== "boolean"
  )
    throw Error("Project settings are out of range.");
  v.shapes.forEach((o) => {
    if (
      !o ||
      !["path", "polygon", "ellipse", "rectangle"].includes(o.type) ||
      ![o.x, o.y, o.w, o.h, o.rotation, o.stroke].every(Number.isFinite) ||
      Math.abs(o.x) > 1000 ||
      Math.abs(o.y) > 1000 ||
      o.w <= 0 ||
      o.h <= 0 ||
      o.w > 1000 ||
      o.h > 1000 ||
      Math.abs(o.rotation) > 3600 ||
      o.stroke < 1 ||
      o.stroke > 40 ||
      !Number.isInteger(o.role) ||
      o.role < 0 ||
      o.role >= v.palette.length ||
      typeof o.filled !== "boolean"
    )
      throw Error("Invalid motif geometry.");
    if (
      ["path", "polygon"].includes(o.type) &&
      (!Array.isArray(o.points) ||
        o.points.length < 2 ||
        o.points.length > 5000 ||
        !o.points.every(
          (p) =>
            Array.isArray(p) &&
            p.length === 2 &&
            p.every((n) => Number.isFinite(n) && Math.abs(n) < 2000),
        ))
    )
      throw Error("Invalid drawing points.");
    o.id = crypto.randomUUID();
  });
  return structuredClone(v);
}
export function poseAnchors(l) {
  const required = [11, 12, 23, 24];
  if (
    !l ||
    !required.every(
      (i) =>
        l[i] &&
        l[i].visibility > 0.45 &&
        Number.isFinite(l[i].x) &&
        Number.isFinite(l[i].y),
    )
  )
    throw Error(
      "Shoulders and hips are not clear. Use a front-facing photo or adjust the six handles manually.",
    );
  const left = l[11].x < l[12].x ? 11 : 12,
    right = left === 11 ? 12 : 11,
    lh = l[23].x < l[24].x ? 23 : 24,
    rh = lh === 23 ? 24 : 23;
  const width = Math.abs(l[right].x - l[left].x),
    torso = (l[lh].y + l[rh].y - (l[left].y + l[right].y)) / 2;
  if (width < 0.04 || torso < 0.04)
    throw Error(
      "The detected person is too small or tilted. Use another photo or manual correction.",
    );
  const hipY = (l[lh].y + l[rh].y) / 2,
    shoulderY = (l[left].y + l[right].y) / 2,
    shoulderMid = (l[left].x + l[right].x) / 2,
    mid = (l[lh].x + l[rh].x) / 2,
    waistMid = shoulderMid * 0.36 + mid * 0.64,
    waistY = shoulderY + torso * 0.64,
    waistHalf = Math.max(Math.abs(l[rh].x - l[lh].x) * 0.47, width * 0.34);
  const legsVisible = [25, 26, 27, 28].every(
    (i) => l[i]?.visibility > 0.5 && Number.isFinite(l[i].y) && l[i].y > hipY,
  );
  const legHem = legsVisible
    ? (l[25].y + l[26].y + l[27].y + l[28].y) / 4
    : hipY + torso * 1.12;
  const hemY = Math.min(0.97, Math.max(waistY + 0.1, legHem));
  return [
    [l[left].x, l[left].y + torso * 0.02],
    [l[right].x, l[right].y + torso * 0.02],
    [waistMid + waistHalf, waistY],
    [mid + width * 0.67, hemY],
    [mid - width * 0.67, hemY],
    [waistMid - waistHalf, waistY],
  ].map(([x, y]) => [Math.max(0, Math.min(1, x)), Math.max(0, Math.min(1, y))]);
}
