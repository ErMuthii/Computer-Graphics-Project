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
function triangle(ctx, img, src, dst) {
  const [a, b, c] = src,
    [p, q, r] = dst;
  const det =
    a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]);
  if (Math.abs(det) < 0.001) return;
  const solve = (v) => [
    (v[0] * (b[1] - c[1]) + v[1] * (c[1] - a[1]) + v[2] * (a[1] - b[1])) / det,
    (v[0] * (c[0] - b[0]) + v[1] * (a[0] - c[0]) + v[2] * (b[0] - a[0])) / det,
    (v[0] * (b[0] * c[1] - c[0] * b[1]) +
      v[1] * (c[0] * a[1] - a[0] * c[1]) +
      v[2] * (a[0] * b[1] - b[0] * a[1])) /
      det,
  ];
  const x = solve([p[0], q[0], r[0]]),
    y = solve([p[1], q[1], r[1]]);
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(...p);
  ctx.lineTo(...q);
  ctx.lineTo(...r);
  ctx.closePath();
  ctx.clip();
  ctx.transform(x[0], y[0], x[1], y[1], x[2], y[2]);
  ctx.drawImage(img, 0, 0);
  ctx.restore();
}
export function drawPhoto(
  canvas,
  image,
  pattern,
  anchors,
  { opacity = 1, handles = true, landmarks = null, arms = true } = {},
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
  const uv = [
    [0, 0],
    [pattern.width, 0],
    [pattern.width, pattern.height * 0.38],
    [pattern.width, pattern.height],
    [0, pattern.height],
    [0, pattern.height * 0.38],
  ];
  c.save();
  c.globalAlpha = opacity;
  for (const ids of [
    [0, 1, 2],
    [0, 2, 5],
    [5, 2, 3],
    [5, 3, 4],
  ])
    triangle(
      c,
      pattern,
      ids.map((i) => uv[i]),
      ids.map((i) => d[i]),
    );
  c.restore();
  // Re-composite the original pixels along detected forearms so crossing arms
  // can remain in front of the new dress. This is pose-guided occlusion, not segmentation.
  if (arms && landmarks) {
    const radius = Math.hypot(d[1][0] - d[0][0], d[1][1] - d[0][1]) * 0.085;
    c.save();
    c.beginPath();
    for (const ids of [
      [13, 15],
      [14, 16],
    ]) {
      const a = landmarks[ids[0]],
        b = landmarks[ids[1]];
      if (a.visibility < 0.5 || b.visibility < 0.5) continue;
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
  if (handles) {
    c.strokeStyle = "#155dcc";
    c.lineWidth = Math.max(2, w / 400);
    c.setLineDash([8, 5]);
    c.beginPath();
    d.forEach(([x, y], i) => (i ? c.lineTo(x, y) : c.moveTo(x, y)));
    c.closePath();
    c.stroke();
    c.setLineDash([]);
    d.forEach(([x, y], i) => {
      c.fillStyle = "#fff";
      c.strokeStyle = "#155dcc";
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
