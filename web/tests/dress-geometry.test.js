import test from "node:test";
import assert from "node:assert/strict";
import {
  createDressGeometry,
  sampleDressProfile,
  PRINT_PERIOD,
} from "../src/dress-geometry.js";
test("smooth profile is bounded with strictly increasing physical height", () => {
  const points = sampleDressProfile();
  assert.equal(points.length, 97);
  for (let j = 0; j < points.length; j++) {
    assert.ok(
      Number.isFinite(points[j].x) &&
        points[j].x >= 0.129 &&
        points[j].x <= 0.621,
    );
    if (j) assert.ok(points[j].y > points[j - 1].y);
  }
});
test("dress has finite closed geometry and matched normals at its back seam", () => {
  const g = createDressGeometry({ segments: 32, rows: 32 });
  const p = g.attributes.position,
    n = g.attributes.normal;
  assert.ok(
    [...p.array, ...n.array, ...g.attributes.uv.array].every(Number.isFinite),
  );
  for (let j = 0; j <= 32; j++) {
    const last = 32 * 33 + j;
    assert.ok(
      Math.hypot(
        p.getX(j) - p.getX(last),
        p.getY(j) - p.getY(last),
        p.getZ(j) - p.getZ(last),
      ) < 1e-6,
    );
    assert.ok(p.getZ(j) < 0, "construction seam is behind the mannequin");
    assert.ok(
      Math.hypot(
        n.getX(j) - n.getX(last),
        n.getY(j) - n.getY(last),
        n.getZ(j) - n.getZ(last),
      ) < 1e-6,
    );
  }
  g.dispose();
});
test("UVs track physical height and local circumference instead of profile row index", () => {
  const rows = 32,
    segments = 32,
    g = createDressGeometry({ segments, rows }),
    p = g.attributes.position,
    uv = g.attributes.uv;
  for (let j = 0; j <= rows; j++) {
    assert.ok(Math.abs(uv.getY(j) - (p.getY(j) - 0.57) / PRINT_PERIOD) < 1e-5);
    assert.ok(
      Math.abs(uv.getX((segments / 2) * (rows + 1) + j)) < 1e-6,
      "front print remains centered",
    );
    for (let i = 1; i <= segments; i++) {
      const a = (i - 1) * (rows + 1) + j,
        b = i * (rows + 1) + j;
      const distance = Math.hypot(p.getX(a) - p.getX(b), p.getZ(a) - p.getZ(b));
      assert.ok(
        Math.abs((uv.getX(b) - uv.getX(a)) * PRINT_PERIOD - distance) < 1e-6,
      );
    }
  }
  g.dispose();
});
