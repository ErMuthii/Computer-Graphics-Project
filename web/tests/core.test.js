import test from "node:test";
import assert from "node:assert/strict";
import {
  defaults,
  validateProject,
  contrast,
  paletteScore,
  poseAnchors,
  hitShape,
} from "../src/core.js";
test("contrast is symmetric with exact black-white and equal-colour limits", () => {
  assert.equal(contrast("#000000", "#ffffff"), 21);
  assert.equal(contrast("#abcdef", "#abcdef"), 1);
  assert.equal(contrast("#123456", "#fedcba"), contrast("#fedcba", "#123456"));
});
test("palette score is bounded and uniform colours have no readable separation", () => {
  const s = paletteScore(["#777777", "#777777", "#777777"]);
  assert.ok(s.readability < 30);
  for (const p of [defaults().palette, ["#000000", "#ffffff", "#ff0000"]]) {
    const a = paletteScore(p);
    assert.ok(a.total >= 0 && a.total <= 100);
    assert.equal(a.ratios.length, 3);
  }
});
test("project round trip preserves data and rejects dangerous geometry", () => {
  const p = defaults();
  assert.equal(validateProject(JSON.parse(JSON.stringify(p))).shapes.length, 2);
  for (const bad of [
    null,
    {},
    { ...p, palette: ["red"] },
    { ...p, settings: { ...p.settings, pitch: 0 } },
    { ...p, shapes: [{ ...p.shapes[0], w: Infinity }] },
    { ...p, shapes: [{ ...p.shapes[0], role: 999 }] },
    { ...p, shapes: [null] },
    { ...p, settings: { ...p.settings, reflectX: "false" } },
  ])
    assert.throws(() => validateProject(bad));
});
test("pose alignment uses visible shoulders and hips and bounds the hem", () => {
  const l = Array.from({ length: 33 }, () => ({
    x: 0.5,
    y: 0.5,
    visibility: 1,
  }));
  Object.assign(l[11], { x: 0.3, y: 0.2 });
  Object.assign(l[12], { x: 0.7, y: 0.2 });
  Object.assign(l[23], { x: 0.38, y: 0.5 });
  Object.assign(l[24], { x: 0.62, y: 0.5 });
  const a = poseAnchors(l);
  assert.equal(a.length, 6);
  assert.ok(a[3][1] > 0.5 && a[3][1] <= 0.97);
  l[11].visibility = 0.1;
  assert.throws(() => poseAnchors(l));
  assert.throws(() => poseAnchors([]));
});
test("rotated shape picking respects local coordinates", () => {
  const s = { x: 100, y: 100, w: 100, h: 20, rotation: 90 };
  assert.equal(hitShape(s, 100, 140), true);
  assert.equal(hitShape(s, 140, 100), false);
});
