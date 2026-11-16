// Handle geometry stays in motif coordinates, independent of canvas display size.
const handles = [
  ["nw", -1, -1],
  ["n", 0, -1],
  ["ne", 1, -1],
  ["e", 1, 0],
  ["se", 1, 1],
  ["s", 0, 1],
  ["sw", -1, 1],
  ["w", -1, 0],
];
function rotate(x, y, angle) {
  const a = (angle * Math.PI) / 180;
  return [x * Math.cos(a) - y * Math.sin(a), x * Math.sin(a) + y * Math.cos(a)];
}
export function resizeHandles(s) {
  return handles.map(([key, sx, sy]) => {
    const [x, y] = rotate((sx * s.w) / 2, (sy * s.h) / 2, s.rotation);
    return { key, x: s.x + x, y: s.y + y };
  });
}
export function hitResizeHandle(s, x, y, tolerance = 10) {
  return (
    resizeHandles(s)
      .map((h) => ({ ...h, distance: Math.hypot(h.x - x, h.y - y) }))
      .filter((h) => h.distance <= tolerance)
      .sort((a, b) => a.distance - b.distance)[0]?.key ?? null
  );
}
export function resizeCursor(key, rotation) {
  const [, sx, sy] = handles.find((h) => h[0] === key);
  const angle = (Math.atan2(sy, sx) * 180) / Math.PI + rotation;
  return ["ew-resize", "nwse-resize", "ns-resize", "nesw-resize"][
    ((Math.round(angle / 45) % 4) + 4) % 4
  ];
}
export function resizeFromHandle(original, key, x, y, keepAspect = false) {
  const [, sx, sy] = handles.find((h) => h[0] === key);
  const [px, py] = rotate(x - original.x, y - original.y, -original.rotation);
  const fixedX = sx ? (-sx * original.w) / 2 : 0,
    fixedY = sy ? (-sy * original.h) / 2 : 0;
  const clamp = (n) => Math.max(8, Math.min(380, n));
  let w = sx ? clamp(sx * (px - fixedX)) : original.w,
    h = sy ? clamp(sy * (py - fixedY)) : original.h;
  if (keepAspect && sx && sy) {
    const wx = w / original.w,
      hy = h / original.h;
    const dominant = Math.abs(wx - 1) > Math.abs(hy - 1) ? wx : hy;
    const ratio = Math.max(
      8 / Math.min(original.w, original.h),
      Math.min(380 / Math.max(original.w, original.h), dominant),
    );
    w = original.w * ratio;
    h = original.h * ratio;
  }
  const [cx, cy] = rotate(
    sx ? fixedX + (sx * w) / 2 : 0,
    sy ? fixedY + (sy * h) / 2 : 0,
    original.rotation,
  );
  const result = { ...original, x: original.x + cx, y: original.y + cy, w, h };
  if (original.type === "path")
    result.points = original.points.map(([px, py]) => [
      (px * w) / original.w,
      (py * h) / original.h,
    ]);
  return result;
}
