export const palettes = [
  ["#f2ddb0", "#af4225", "#31553c"],
  ["#eee8da", "#26677b", "#be7130"],
  ["#f1deda", "#783b65", "#304b50"],
];
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
export function motif(kind) {
  const poly = (points, role) => ({
    id: crypto.randomUUID(),
    type: "polygon",
    x: 200,
    y: 200,
    w: 150,
    h: 150,
    rotation: 0,
    role,
    stroke: 3,
    filled: true,
    points,
  });
  if (kind === "diamond")
    return [
      poly(
        [
          [0, -1],
          [1, 0],
          [0, 1],
          [-1, 0],
        ],
        1,
      ),
      {
        ...poly(
          [
            [0, -1],
            [1, 0],
            [0, 1],
            [-1, 0],
          ],
          2,
        ),
        w: 58,
        h: 58,
      },
    ];
  if (kind === "flower")
    return Array.from({ length: 4 }, (_, i) => ({
      id: crypto.randomUUID(),
      type: "ellipse",
      x: 200 + Math.cos((i * Math.PI) / 2) * 48,
      y: 200 + Math.sin((i * Math.PI) / 2) * 48,
      w: 52,
      h: 104,
      rotation: i * 90 + 90,
      role: (i % 2) + 1,
      filled: true,
      stroke: 2,
    }));
  return [
    poly(
      [
        [-1, 1],
        [-1, 0.4],
        [-0.3, 0.4],
        [-0.3, -0.3],
        [0.4, -0.3],
        [0.4, -1],
        [1, -1],
        [1, 1],
      ],
      1,
    ),
  ];
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
      c.strokeStyle = "#172fbd";
      c.setLineDash([5, 4]);
      c.strokeRect(-s.w / 2 - 6, -s.h / 2 - 6, s.w + 12, s.h + 12);
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
  const ratios = [
    contrast(p[0], p[1]),
    contrast(p[0], p[2]),
    contrast(p[1], p[2]),
  ];
  const readability = Math.round(
    ((Math.min(ratios[0] / 4.5, 1) +
      Math.min(ratios[1] / 4.5, 1) +
      Math.min(ratios[2] / 3, 1)) /
      3) *
      100,
  );
  return {
    harmony,
    readability,
    total: Math.round(harmony * 0.5 + readability * 0.5),
    scheme: hs.length < 2 ? "Neutral" : best.name,
    ratios,
  };
}
export function validateProject(v) {
  if (
    !v ||
    v.version !== 1 ||
    !Array.isArray(v.palette) ||
    v.palette.length !== 3 ||
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
      ![0, 1, 2].includes(o.role) ||
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
    mid = (l[lh].x + l[rh].x) / 2,
    hemY = Math.min(0.97, hipY + torso * 1.4);
  return [
    [l[left].x, l[left].y + 0.025],
    [l[right].x, l[right].y + 0.025],
    [l[rh].x, hipY],
    [mid + width * 0.8, hemY],
    [mid - width * 0.8, hemY],
    [l[lh].x, hipY],
  ].map(([x, y]) => [Math.max(0, Math.min(1, x)), Math.max(0, Math.min(1, y))]);
}
