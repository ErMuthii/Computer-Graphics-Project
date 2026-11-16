import test from "node:test";
import assert from "node:assert/strict";
import {
  resizeHandles,
  hitResizeHandle,
  resizeCursor,
  resizeFromHandle,
} from "../src/selection.js";
const shape = { type: "polygon", x: 150, y: 150, w: 100, h: 80, rotation: 0 };
test("corner resize anchors the opposite corner; side resize changes one dimension", () => {
  assert.equal(hitResizeHandle(shape, 200, 190), "se");
  assert.equal(hitResizeHandle(shape, 150, 150), null);
  const resized = resizeFromHandle(shape, "se", 250, 230);
  assert.equal(resized.w, 150);
  assert.equal(resized.h, 120);
  assert.deepEqual(resizeHandles(resized)[0], resizeHandles(shape)[0]);
  const edge = resizeFromHandle(shape, "e", 240, 150);
  assert.equal(edge.w, 140);
  assert.equal(edge.h, 80);
  assert.equal(edge.y, 150);
});
test("rotated resizing preserves the opposite corner in world coordinates", () => {
  const original = { ...shape, rotation: 37 };
  const se = resizeHandles(original).find((h) => h.key === "se");
  const a = (37 * Math.PI) / 180;
  const resized = resizeFromHandle(
    original,
    "se",
    se.x + 80 * Math.cos(a) - 40 * Math.sin(a),
    se.y + 80 * Math.sin(a) + 40 * Math.cos(a),
  );
  assert.ok(Math.abs(resized.w - 180) < 1e-8);
  assert.ok(Math.abs(resized.h - 120) < 1e-8);
  const before = resizeHandles(original)[0],
    after = resizeHandles(resized)[0];
  assert.ok(Math.hypot(before.x - after.x, before.y - after.y) < 1e-8);
  assert.equal(resizeCursor("e", 90), "ns-resize");
});
test("aspect locking, size limits and freehand scaling keep valid geometry", () => {
  const locked = resizeFromHandle(shape, "se", 270, 240, true);
  assert.ok(Math.abs(locked.w / locked.h - shape.w / shape.h) < 1e-8);
  const tiny = resizeFromHandle(shape, "se", 0, 0);
  assert.equal(tiny.w, 8);
  assert.equal(tiny.h, 8);
  const big = resizeFromHandle(shape, "se", 999, 999);
  assert.equal(big.w, 380);
  assert.equal(big.h, 380);
  const path = {
    ...shape,
    type: "path",
    points: [
      [-50, -40],
      [50, 40],
    ],
  };
  const resized = resizeFromHandle(path, "se", 300, 270);
  assert.deepEqual(resized.points, [
    [-100, -80],
    [100, 80],
  ]);
  assert.deepEqual(path.points, [
    [-50, -40],
    [50, 40],
  ]);
});
