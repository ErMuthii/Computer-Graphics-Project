import { FilesetResolver, PoseLandmarker } from "@mediapipe/tasks-vision";
import { poseAnchors } from "./core.js";
let detectorPromise;
export async function detectPose(image) {
  if (!detectorPromise)
    detectorPromise = loadDetector().catch((error) => {
      detectorPromise = null;
      throw error;
    });
  const detector = await detectorPromise;
  const result = detector.detect(image);
  return {
    anchors: poseAnchors(result.landmarks[0]),
    landmarks: result.landmarks[0],
  };
}
async function loadDetector() {
  const vision = await FilesetResolver.forVisionTasks(
    import.meta.env.BASE_URL + "vision",
  );
  return PoseLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath:
        import.meta.env.BASE_URL + "models/pose_landmarker_lite.task",
      delegate: "CPU",
    },
    runningMode: "IMAGE",
    numPoses: 1,
  });
}
export function manualAnchors() {
  return [
    [0.34, 0.27],
    [0.66, 0.27],
    [0.62, 0.51],
    [0.78, 0.92],
    [0.22, 0.92],
    [0.38, 0.51],
  ];
}

const mix = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t];
const add = (a, b, t = 1) => [a[0] + b[0] * t, a[1] + b[1] * t];
export function dressPath(d, neckline = "round", neckDepth = 0.2) {
  const [sl, sr, wr, hr, hl, wl] = d;
  const top = mix(sl, sr, 0.5),
    waist = mix(wl, wr, 0.5);
  const down = [waist[0] - top[0], waist[1] - top[1]];
  const across = [sr[0] - sl[0], sr[1] - sl[1]];
  const left = mix(sl, sr, 0.025),
    right = mix(sl, sr, 0.975);
  const nl = mix(sl, sr, neckline === "boat" ? 0.23 : 0.33),
    nr = mix(sl, sr, neckline === "boat" ? 0.77 : 0.67);
  const ar = add(mix(sr, wr, 0.35), across, -0.09),
    al = add(mix(sl, wl, 0.35), across, 0.09);
  const p = new Path2D();
  p.moveTo(...left);
  p.lineTo(...nl);
  if (neckline === "v") {
    p.lineTo(...add(top, down, neckDepth));
    p.lineTo(...nr);
  } else
    p.quadraticCurveTo(
      ...add(top, down, neckline === "boat" ? 0.12 : neckDepth * 2),
      ...nr,
    );
  p.lineTo(...right);
  // Curved armholes and side seams replace the rectangular chest panel.
  p.quadraticCurveTo(...add(sr, down, 0.28), ...ar);
  p.quadraticCurveTo(...mix(ar, wr, 0.65), ...wr);
  p.quadraticCurveTo(...mix(wr, hr, 0.5), ...hr);
  const hem = mix(hl, hr, 0.5);
  p.quadraticCurveTo(...add(hem, down, 0.025), ...hl);
  p.quadraticCurveTo(...mix(hl, wl, 0.5), ...wl);
  p.quadraticCurveTo(...mix(wl, al, 0.35), ...al);
  p.quadraticCurveTo(...add(sl, down, 0.28), ...left);
  p.closePath();
  return p;
}

// Store correction strokes as normalized points, so undo needs no large image snapshots.
export function paintMaskStroke(canvas, stroke) {
  const c = canvas.getContext("2d"),
    w = canvas.width,
    h = canvas.height;
  c.save();
  c.globalCompositeOperation =
    stroke.mode === "protect" ? "source-over" : "destination-out";
  c.strokeStyle = c.fillStyle = "#ffffff";
  c.lineCap = c.lineJoin = "round";
  c.lineWidth = stroke.size * w;
  const [first, ...rest] = stroke.points;
  c.beginPath();
  c.arc(first[0] * w, first[1] * h, c.lineWidth / 2, 0, Math.PI * 2);
  c.fill();
  c.beginPath();
  c.moveTo(first[0] * w, first[1] * h);
  rest.forEach(([x, y]) => c.lineTo(x * w, y * h));
  c.stroke();
  c.restore();
}
export function rebuildMask(canvas, strokes) {
  canvas.getContext("2d").clearRect(0, 0, canvas.width, canvas.height);
  strokes.forEach((stroke) => paintMaskStroke(canvas, stroke));
}
const buffers = new WeakMap();
export function drawPhoto(
  canvas,
  image,
  tile,
  anchors,
  {
    opacity = 1,
    handles = true,
    landmarks = null,
    arms = false,
    mask = null,
    printScale = 1,
    neckline = "round",
    neckDepth = 0.2,
    shading = true,
  } = {},
) {
  const c = canvas.getContext("2d"),
    w = canvas.width,
    h = canvas.height;
  c.clearRect(0, 0, w, h);
  if (!image) {
    c.fillStyle = "#e9e5db";
    c.fillRect(0, 0, w, h);
    return;
  }
  c.drawImage(image, 0, 0, w, h);
  const d = anchors.map(([x, y]) => [x * w, y * h]);
  const outline = dressPath(d, neckline, neckDepth);
  const span = Math.hypot(d[1][0] - d[0][0], d[1][1] - d[0][1]);
  const angle = Math.atan2(d[1][1] - d[0][1], d[1][0] - d[0][0]);
  const repeat = c.createPattern(tile, "repeat");
  // One uniform sampling transform over the whole dress: no stretched triangles
  // or discontinuity across the waist. Print size is independent of garment length.
  const scale = (Math.max(1, span) * 0.36 * printScale) / tile.width;
  repeat.setTransform(
    new DOMMatrix()
      .translate(d[0][0], d[0][1])
      .rotate((angle * 180) / Math.PI)
      .scale(scale),
  );
  c.save();
  c.globalAlpha = opacity;
  c.clip(outline);
  c.fillStyle = repeat;
  c.fillRect(0, 0, w, h);
  if (shading) {
    const shade = c.createLinearGradient(d[5][0], d[5][1], d[2][0], d[2][1]);
    shade.addColorStop(0, "#20160b38");
    shade.addColorStop(0.22, "#ffffff08");
    shade.addColorStop(0.55, "#ffffff00");
    shade.addColorStop(0.85, "#20160b0a");
    shade.addColorStop(1, "#20160b38");
    c.fillStyle = shade;
    c.fillRect(0, 0, w, h);
    c.strokeStyle = "#24180d25";
    c.lineWidth = Math.max(1, span * 0.004);
    c.beginPath();
    c.moveTo(...d[5]);
    c.quadraticCurveTo(
      ...add(mix(d[5], d[2], 0.5), [0, span * 0.018]),
      ...d[2],
    );
    c.stroke();
  }
  c.restore();
  // Optional original forearm pixels include any sleeves in the source photograph.
  // Off by default: manual masks let the user protect precisely the hands/hair wanted.
  if (arms && landmarks) {
    const radius = span * 0.065;
    c.save();
    c.beginPath();
    for (const [i, j] of [
      [13, 15],
      [14, 16],
    ]) {
      const a = landmarks[i],
        b = landmarks[j];
      if (!a || !b || a.visibility < 0.5 || b.visibility < 0.5) continue;
      const ax = a.x * w,
        ay = a.y * h,
        bx = b.x * w,
        by = b.y * h,
        angle = Math.atan2(by - ay, bx - ax),
        nx = Math.sin(angle) * radius,
        ny = -Math.cos(angle) * radius;
      c.moveTo(ax + nx, ay + ny);
      c.lineTo(bx + nx, by + ny);
      c.lineTo(bx - nx, by - ny);
      c.lineTo(ax - nx, ay - ny);
      c.closePath();
    }
    c.clip();
    c.drawImage(image, 0, 0, w, h);
    c.restore();
  }
  if (mask) {
    let buffer = buffers.get(canvas);
    if (!buffer) {
      buffer = document.createElement("canvas");
      buffers.set(canvas, buffer);
    }
    if (buffer.width !== w || buffer.height !== h) {
      buffer.width = w;
      buffer.height = h;
    }
    const b = buffer.getContext("2d");
    b.clearRect(0, 0, w, h);
    b.drawImage(image, 0, 0, w, h);
    b.globalCompositeOperation = "destination-in";
    b.drawImage(mask, 0, 0, w, h);
    b.globalCompositeOperation = "source-over";
    c.drawImage(buffer, 0, 0);
  }
  if (handles) {
    c.strokeStyle = "#155dcc";
    c.lineWidth = Math.max(2, w / 450);
    c.setLineDash([8, 5]);
    c.stroke(outline);
    c.setLineDash([]);
    d.forEach(([x, y], i) => {
      c.fillStyle = "#fff";
      c.beginPath();
      c.arc(x, y, Math.max(8, w / 65), 0, Math.PI * 2);
      c.fill();
      c.stroke();
      c.fillStyle = "#155dcc";
      c.font = `bold ${Math.max(12, w / 55)}px sans-serif`;
      c.textAlign = "center";
      c.textBaseline = "middle";
      c.fillText(String(i + 1), x, y);
    });
  }
}
