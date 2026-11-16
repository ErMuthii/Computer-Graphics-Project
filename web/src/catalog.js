// Editable local vector geometry. The first six entries match the native catalogue;
// browser curves are sampled here, independently of the C++ implementation.
export const motifCatalog = [
  ["diamond", "Diamond"],
  ["triangle", "Triangle"],
  ["cross", "Cross"],
  ["steps", "Steps"],
  ["sunburst", "Sunburst"],
  ["arch", "Bézier arch"],
  ["flower", "Flower"],
  ["leaf", "Leaf"],
  ["star", "Star"],
  ["spiral", "Spiral"],
  ["chevron", "Chevron"],
  ["rosette", "Rosette"],
  ["rings", "Rings"],
  ["fan", "Fan"],
  ["wave", "Wave"],
  ["cowrie", "Cowrie"],
  ["seed", "Seed cluster"],
  ["hexagon", "Hexagon"],
];
export const paletteCatalog = [
  ["Earth", ["#f2ddb0", "#af4225", "#31553c"]],
  ["Lagoon", ["#eee8da", "#26677b", "#be7130"]],
  ["Plum", ["#f1deda", "#783b65", "#304b50"]],
  ["Indigo", ["#ece5d4", "#263469", "#d6a024"]],
  ["Coral coast", ["#fff0dc", "#a73535", "#15635e", "#eea23d"]],
  ["Forest", ["#edf0d5", "#234b35", "#8a4928", "#bda343"]],
  ["Midnight", ["#171e35", "#e8b84c", "#ef7556", "#ece7d5"]],
  ["Terracotta", ["#f9e8d0", "#8d3324", "#285d67", "#b47b32"]],
  ["Papaya", ["#fff2d5", "#b84324", "#45622c", "#e9a433"]],
  ["Ocean", ["#e2f0ea", "#183f69", "#148276", "#df793b"]],
  ["Berry", ["#f5e4eb", "#792452", "#385c73", "#c06636"]],
  ["Monochrome", ["#f7f4ed", "#1e2427", "#6e726d"]],
  ["Gold & ink", ["#f2e8c9", "#202e3a", "#b37d16"]],
  ["Red clay", ["#f2d9c5", "#8b2825", "#592b44", "#476440"]],
  ["Turquoise", ["#e6f5ed", "#116c72", "#c14a30", "#c09828"]],
  ["Violet dusk", ["#eee5f1", "#512a78", "#c16e34", "#286869"]],
  ["Saffron", ["#f9e9bd", "#75372a", "#8c6510", "#274c63"]],
  ["Garden", ["#f4f0d9", "#295945", "#a23d69", "#cc8c29"]],
  ["Electric", ["#191c32", "#f0bb27", "#d65f9f", "#39a6ab"]],
  ["Sand & sea", ["#f2e3c6", "#244e67", "#a84d2d", "#747b40"]],
  ["Rosewood", ["#f2dfd5", "#6b2839", "#325b4b", "#c48b3b"]],
  ["Cobalt", ["#f2efdb", "#24398b", "#c64232", "#bd8724"]],
  ["Sunrise", ["#fff0cb", "#9a3524", "#b46b16", "#5e3b76"]],
  ["Night garden", ["#142c29", "#e6c668", "#d9778d", "#68a6a2"]],
].map(([name, colors]) => ({ name, colors }));

function hslHex(h, s, l) {
  const a = s * Math.min(l, 1 - l);
  const f = (n) => {
    const k = (n + h / 30) % 12;
    return Math.round(255 * (l - a * Math.max(-1, Math.min(k - 3, 9 - k, 1))))
      .toString(16)
      .padStart(2, "0");
  };
  return "#" + f(0) + f(8) + f(4);
}
const hues = [
  ["Red", 0],
  ["Orange", 25],
  ["Amber", 45],
  ["Yellow", 60],
  ["Lime", 85],
  ["Green", 125],
  ["Teal", 165],
  ["Cyan", 190],
  ["Blue", 220],
  ["Violet", 260],
  ["Purple", 290],
  ["Pink", 330],
];
export const swatches = [
  ...hues.flatMap(([family, h]) =>
    [
      ["Pale", 0.9],
      ["Light", 0.72],
      ["Mid", 0.5],
      ["Deep", 0.33],
      ["Dark", 0.19],
    ].map(([shade, l]) => ({
      family,
      name: `${shade} ${family.toLowerCase()}`,
      hex: hslHex(h, 0.65, l),
    })),
  ),
  ...[
    "#ffffff",
    "#f7f4ed",
    "#e6e2d8",
    "#ccc7bc",
    "#a6a197",
    "#807b72",
    "#625d55",
    "#45413b",
    "#302d29",
    "#20201e",
    "#111111",
    "#000000",
  ].map((hex, i) => ({ family: "Neutral", name: `Neutral ${i + 1}`, hex })),
  ...[
    "#f2ddb0",
    "#e8c790",
    "#c79d5a",
    "#af4225",
    "#843322",
    "#5d2b22",
    "#31553c",
    "#52633b",
    "#8a7738",
    "#b67c38",
    "#703e52",
    "#284f62",
  ].map((hex, i) => ({ family: "Earth", name: `Earth ${i + 1}`, hex })),
];

function shape(type, role = 1, extra = {}) {
  return {
    id: crypto.randomUUID(),
    type,
    x: 200,
    y: 200,
    w: 170,
    h: 170,
    rotation: 0,
    role,
    stroke: 6,
    filled: true,
    ...extra,
  };
}
const poly = (points, role = 1, extra = {}) =>
  shape("polygon", role, { points, ...extra });
const ellipse = (w, h, role = 1, extra = {}) =>
  shape("ellipse", role, { w, h, ...extra });
const polar = (n, r = 1, offset = -Math.PI / 2) =>
  Array.from({ length: n }, (_, i) => [
    r * Math.cos((i * 2 * Math.PI) / n + offset),
    r * Math.sin((i * 2 * Math.PI) / n + offset),
  ]);
function cubic(segments, steps = 24) {
  return segments.flatMap((c, index) =>
    Array.from({ length: steps + 1 }, (_, i) => {
      const t = i / steps,
        u = 1 - t;
      return [0, 1].map(
        (k) =>
          u ** 3 * c[0][k] +
          3 * u * u * t * c[1][k] +
          3 * u * t * t * c[2][k] +
          t ** 3 * c[3][k],
      );
    }).slice(index ? 1 : 0),
  );
}
export function createMotif(kind) {
  const diamond = [
    [0, -1],
    [1, 0],
    [0, 1],
    [-1, 0],
  ];
  if (kind === "diamond")
    return [poly(diamond), poly(diamond, 2, { w: 65, h: 65 })];
  if (kind === "triangle")
    return [
      poly([
        [0, -1],
        [1, 1],
        [-1, 1],
      ]),
      poly(
        [
          [-1, -0.6],
          [1, -0.6],
          [0, 1],
        ],
        2,
        { w: 75, h: 75, y: 223 },
      ),
    ];
  if (kind === "cross")
    return [
      poly([
        [-1, -0.4],
        [-0.36, -0.4],
        [-0.36, -1],
        [0.36, -1],
        [0.36, -0.4],
        [1, -0.4],
        [1, 0.4],
        [0.36, 0.4],
        [0.36, 1],
        [-0.36, 1],
        [-0.36, 0.4],
        [-1, 0.4],
      ]),
      shape("rectangle", 2, { w: 40, h: 40 }),
    ];
  if (kind === "steps")
    return [
      poly([
        [-1, 1],
        [-1, 0.5],
        [-0.5, 0.5],
        [-0.5, 0],
        [0, 0],
        [0, -0.5],
        [0.5, -0.5],
        [0.5, -1],
        [1, -1],
        [1, 1],
      ]),
      shape("rectangle", 2, { x: 238, y: 238, w: 32, h: 32 }),
    ];
  if (kind === "arch")
    return [
      poly(
        cubic([
          [
            [-0.8, 0.48],
            [-0.8, -0.96],
            [0.8, -0.96],
            [0.8, 0.48],
          ],
          [
            [0.8, 0.48],
            [0.64, 0.8],
            [0.16, 0.8],
            [0, 0.48],
          ],
          [
            [0, 0.48],
            [-0.16, 0.8],
            [-0.64, 0.8],
            [-0.8, 0.48],
          ],
        ]),
      ),
      poly(
        cubic([
          [
            [0, 0.28],
            [-0.4, 0.08],
            [-0.4, -0.32],
            [0, -0.56],
          ],
          [
            [0, -0.56],
            [0.4, -0.32],
            [0.4, 0.08],
            [0, 0.28],
          ],
        ]),
        2,
      ),
    ];
  const leaf = cubic([
    [
      [0, 1],
      [-1.1, 0.1],
      [-0.8, -0.9],
      [0, -1],
    ],
    [
      [0, -1],
      [0.8, -0.9],
      [1.1, 0.1],
      [0, 1],
    ],
  ]);
  if (kind === "sunburst")
    return [
      ...Array.from({ length: 4 }, (_, i) =>
        poly(leaf, 2, {
          x: 200 + Math.sin((i * Math.PI) / 2) * 58,
          y: 200 - Math.cos((i * Math.PI) / 2) * 58,
          w: 46,
          h: 105,
          rotation: i * 90,
        }),
      ),
      ellipse(78, 78),
      ellipse(40, 40, 2, { filled: false, stroke: 5 }),
    ];
  if (kind === "flower")
    return [
      ...Array.from({ length: 6 }, (_, i) =>
        ellipse(50, 105, (i % 2) + 1, {
          x: 200 + Math.cos((i * Math.PI) / 3) * 52,
          y: 200 + Math.sin((i * Math.PI) / 3) * 52,
          rotation: i * 60 + 90,
        }),
      ),
      ellipse(40, 40, 2),
    ];
  if (kind === "leaf")
    return [
      poly(leaf, 1, { w: 120, h: 200, rotation: 25 }),
      shape("path", 2, {
        w: 55,
        h: 150,
        rotation: 25,
        points: [
          [0, 70],
          [0, -70],
        ],
        filled: false,
        stroke: 6,
      }),
    ];
  if (kind === "star")
    return [
      poly(
        Array.from({ length: 10 }, (_, i) => {
          const a = (i * Math.PI) / 5 - Math.PI / 2,
            r = i % 2 ? 0.42 : 1;
          return [r * Math.cos(a), r * Math.sin(a)];
        }),
      ),
      poly(polar(5), 2, { w: 45, h: 45 }),
    ];
  if (kind === "spiral")
    return [
      shape("path", 1, {
        w: 190,
        h: 190,
        points: Array.from({ length: 220 }, (_, i) => {
          const a = (i / 219) * Math.PI * 5,
            r = 5 + (i / 219) * 85;
          return [Math.cos(a) * r, Math.sin(a) * r];
        }),
        filled: false,
        stroke: 12,
      }),
      ellipse(13, 13, 2),
    ];
  if (kind === "chevron")
    return [-40, 0, 40].map((y, i) =>
      poly(
        [
          [-1, 0],
          [0, -0.8],
          [1, 0],
          [1, 0.5],
          [0, -0.3],
          [-1, 0.5],
        ],
        (i % 2) + 1,
        { y: 200 + y, w: 180, h: 75 },
      ),
    );
  if (kind === "rosette")
    return [
      ...Array.from({ length: 8 }, (_, i) =>
        ellipse(38, 140, (i % 2) + 1, { rotation: i * 45 }),
      ),
      ellipse(40, 40, 2),
    ];
  if (kind === "rings")
    return [
      ellipse(190, 190, 1, { filled: false, stroke: 13 }),
      ellipse(130, 130, 2, { filled: false, stroke: 11 }),
      ellipse(55, 55),
    ];
  if (kind === "fan")
    return [
      poly([
        [0, 1],
        ...Array.from({ length: 41 }, (_, i) => [
          Math.cos(Math.PI + (i * Math.PI) / 40),
          0.8 + 1.6 * Math.sin(Math.PI + (i * Math.PI) / 40),
        ]),
      ]),
      ...[-0.7, -0.35, 0, 0.35, 0.7].map((x) =>
        shape("path", 2, {
          w: 160,
          h: 170,
          points: [
            [0, 65],
            [x * 74, -60 + Math.abs(x) * 30],
          ],
          filled: false,
          stroke: 5,
        }),
      ),
    ];
  if (kind === "wave")
    return [-40, 0, 40].map((y, i) =>
      shape("path", (i % 2) + 1, {
        w: 200,
        h: 50,
        y: 200 + y,
        points: Array.from({ length: 81 }, (_, j) => [
          -100 + j * 2.5,
          Math.sin((j / 80) * Math.PI * 2) * 23,
        ]),
        filled: false,
        stroke: 12,
      }),
    );
  if (kind === "cowrie")
    return [
      ellipse(116, 190),
      ellipse(54, 150, 2),
      shape("path", 0, {
        w: 8,
        h: 110,
        points: [
          [0, -55],
          [-6, -20],
          [6, 20],
          [0, 55],
        ],
        stroke: 7,
        filled: false,
      }),
    ];
  if (kind === "seed")
    return [
      [-42, -40],
      [42, -40],
      [0, 40],
    ].flatMap(([x, y], i) => [
      ellipse(64, 94, (i % 2) + 1, {
        x: 200 + x,
        y: 200 + y,
        rotation: i * 30 - 30,
      }),
      ellipse(15, 45, 0, { x: 200 + x, y: 200 + y, rotation: i * 30 - 30 }),
    ]);
  if (kind === "hexagon")
    return [
      poly(polar(6, 1, 0)),
      poly(polar(6, 1, 0), 2, { w: 110, h: 110 }),
      poly(polar(6, 1, 0), 0, { w: 45, h: 45 }),
    ];
  throw Error("Unknown starting motif.");
}
