import { defaults, motif, renderPattern, renderRepeat } from "./core.js";
import { paletteCatalog } from "./catalog.js";
import { sampleDressProfile } from "./dress-geometry.js";

export const designLibrary = [
  {
    id: "rust-diamonds",
    name: "Rust diamonds",
    description: "Layered diamonds with small seed dots.",
    theme: "Earth",
    style: "a-line",
    motifs: [["diamond", 200, 200, 1.3, 0]],
    dots: true,
    pitch: 100,
  },
  {
    id: "indigo-lattice",
    name: "Indigo lattice",
    description: "Crosses and diamonds in indigo and gold.",
    theme: "Indigo",
    style: "maxi",
    motifs: [
      ["cross", 200, 200, 1, 0],
      ["diamond", 65, 65, 0.35, 0],
      ["diamond", 335, 335, 0.35, 0],
    ],
    pitch: 105,
  },
  {
    id: "sunburst-gold",
    name: "Sunburst gold",
    description: "Petal medallions with a saffron accent.",
    theme: "Saffron",
    style: "fit-flare",
    motifs: [["sunburst", 200, 200, 1.1, 45]],
    dots: true,
    pitch: 120,
  },
  {
    id: "coastal-waves",
    name: "Coastal waves",
    description: "Flowing wave bands with coral seed details.",
    theme: "Ocean",
    style: "shift",
    motifs: [["wave", 200, 200, 1.55, 0]],
    dots: true,
    pitch: 140,
  },
  {
    id: "plum-rosette",
    name: "Plum rosette",
    description: "Eight-petal rosettes and corner medallions.",
    theme: "Plum",
    style: "midi",
    motifs: [
      ["rosette", 200, 200, 1.45, 0],
      ["rings", 55, 55, 0.24, 0],
      ["rings", 345, 345, 0.24, 0],
    ],
    pitch: 125,
  },
  {
    id: "forest-leaves",
    name: "Forest leaves",
    description: "Paired leaves in a warm woodland palette.",
    theme: "Forest",
    style: "maxi",
    motifs: [
      ["leaf", 130, 200, 0.85, -25],
      ["leaf", 270, 200, 0.85, 155],
    ],
    pitch: 110,
  },
  {
    id: "coral-cowries",
    name: "Coral cowries",
    description: "A shell cluster in coral, teal and amber.",
    theme: "Coral coast",
    style: "a-line",
    motifs: [
      ["cowrie", 110, 170, 0.7, -25],
      ["cowrie", 290, 170, 0.7, 25],
      ["cowrie", 200, 290, 0.55, 0],
    ],
    pitch: 125,
  },
  {
    id: "midnight-stars",
    name: "Midnight stars",
    description: "Bright stars over a deep midnight ground.",
    theme: "Midnight",
    style: "fit-flare",
    motifs: [
      ["star", 200, 200, 1.25, 0],
      ["spiral", 60, 60, 0.32, 0],
      ["spiral", 340, 340, 0.32, 180],
    ],
    pitch: 115,
  },
  {
    id: "earth-chevrons",
    name: "Earth chevrons",
    description: "Stepped chevrons and small ring details.",
    theme: "Terracotta",
    style: "midi",
    motifs: [
      ["chevron", 200, 200, 1.3, 0],
      ["rings", 60, 200, 0.22, 0],
      ["rings", 340, 200, 0.22, 0],
    ],
    pitch: 120,
  },
  {
    id: "teal-fans",
    name: "Teal fans",
    description: "Alternating fans with a turquoise palette.",
    theme: "Turquoise",
    style: "shift",
    motifs: [
      ["fan", 115, 125, 0.8, 0],
      ["fan", 285, 275, 0.8, 180],
    ],
    pitch: 120,
  },
];
function placedMotif([id, cx, cy, scale, angle]) {
  const a = (angle * Math.PI) / 180;
  return motif(id).map((s) => {
    const x = (s.x - 200) * scale,
      y = (s.y - 200) * scale;
    const next = {
      ...s,
      x: cx + x * Math.cos(a) - y * Math.sin(a),
      y: cy + x * Math.sin(a) + y * Math.cos(a),
      w: s.w * scale,
      h: s.h * scale,
      rotation: s.rotation + angle,
      stroke: Math.max(1, s.stroke * scale),
    };
    if (s.type === "path")
      next.points = s.points.map(([x, y]) => [x * scale, y * scale]);
    return next;
  });
}
export function libraryProject(id) {
  const entry = designLibrary.find((e) => e.id === id);
  if (!entry) throw Error("Unknown library design.");
  const project = defaults();
  project.palette = [
    ...paletteCatalog.find((p) => p.name === entry.theme).colors,
  ];
  project.dressStyle = entry.style;
  project.settings.pitch = entry.pitch;
  project.shapes = entry.motifs.flatMap(placedMotif);
  if (entry.dots)
    for (const [x, y] of [
      [65, 65],
      [335, 65],
      [65, 335],
      [335, 335],
    ])
      project.shapes.push({
        id: crypto.randomUUID(),
        type: "ellipse",
        x,
        y,
        w: 18,
        h: 18,
        rotation: 0,
        role: project.palette.length > 3 ? 3 : 2,
        stroke: 2,
        filled: true,
      });
  return project;
}

export function renderDress(canvas, project, time = 0) {
  const c = canvas.getContext("2d"),
    w = canvas.width,
    h = canvas.height;
  c.clearRect(0, 0, w, h);
  const pad = Math.min(w, h) * 0.06,
    scale = Math.min((w - 2 * pad) / 1.6, (h - 2 * pad) / 2.6),
    points = sampleDressProfile(96, project.dressStyle);
  const pixel = (r, y) => [w / 2 + r * scale, pad + (2.6 - y) * scale];
  const path = new Path2D();
  path.moveTo(...pixel(-points[0].x, points[0].y));
  points.forEach((p) => path.lineTo(...pixel(-p.x, p.y)));
  const neck = points.at(-1);
  path.quadraticCurveTo(...pixel(0, neck.y - 0.12), ...pixel(neck.x, neck.y));
  [...points].reverse().forEach((p) => path.lineTo(...pixel(p.x, p.y)));
  path.closePath();
  const tile = document.createElement("canvas");
  renderRepeat(tile, project, time, 512);
  const print = c.createPattern(tile, "repeat");
  const period = ((0.22 * project.settings.pitch) / 110) * scale;
  print.setTransform(
    new DOMMatrix().translate(w / 2 - period / 2, pad).scale(period / 512),
  );
  c.save();
  c.clip(path);
  c.fillStyle = print;
  c.fillRect(0, 0, w, h);
  const shade = c.createLinearGradient(w * 0.12, 0, w * 0.88, 0);
  shade.addColorStop(0, "#20160b30");
  shade.addColorStop(0.5, "#ffffff00");
  shade.addColorStop(1, "#20160b24");
  c.fillStyle = shade;
  c.fillRect(0, 0, w, h);
  c.restore();
  c.strokeStyle = "#332b2425";
  c.lineWidth = Math.max(0.5, w / 500);
  c.stroke(path);
}
export function renderLibraryThumbnail(canvas, entry) {
  const project = libraryProject(entry.id),
    c = canvas.getContext("2d"),
    half = canvas.width / 2;
  const fabric = document.createElement("canvas");
  fabric.width = 360;
  fabric.height = 450;
  renderPattern(fabric, project);
  c.drawImage(fabric, 0, 0, half, canvas.height);
  c.fillStyle = "#e9e5db";
  c.fillRect(half, 0, half, canvas.height);
  const dress = document.createElement("canvas");
  dress.width = 360;
  dress.height = 450;
  renderDress(dress, project);
  c.drawImage(dress, half, 0, half, canvas.height);
}
