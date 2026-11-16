import test from "node:test";
import assert from "node:assert/strict";
import {
  motifCatalog,
  paletteCatalog,
  createMotif,
  swatches,
} from "../src/catalog.js";
import { defaults, validateProject, paletteScore } from "../src/core.js";
test("all 18 motifs contain valid editable geometry with unique layer IDs", () => {
  assert.equal(motifCatalog.length, 18);
  for (const [id] of motifCatalog) {
    const shapes = createMotif(id);
    assert.ok(shapes.length > 0);
    assert.equal(new Set(shapes.map((s) => s.id)).size, shapes.length);
    assert.equal(
      validateProject({ ...defaults(), shapes }).shapes.length,
      shapes.length,
    );
  }
});
test("84 swatches and 24 themes have valid colours; scores include every active pair", () => {
  assert.equal(swatches.length, 84);
  assert.equal(paletteCatalog.length, 24);
  assert.ok(swatches.every((s) => /^#[a-f\d]{6}$/i.test(s.hex)));
  for (const { colors } of paletteCatalog) {
    validateProject({ ...defaults(), palette: colors });
    assert.ok(paletteScore(colors).total >= 0);
  }
  const palette = swatches.slice(0, 32).map((s) => s.hex);
  const p = defaults();
  p.palette = palette;
  p.shapes[0].role = 31;
  assert.equal(validateProject(p).palette.length, 32);
  const score = paletteScore(palette);
  assert.equal(score.pairs.length, 496);
  assert.ok(score.total <= 100);
  assert.throws(() =>
    validateProject({ ...p, palette: [...palette, "#123456"] }),
  );
  assert.throws(() =>
    validateProject({ ...p, shapes: [{ ...p.shapes[0], role: 32 }] }),
  );
});
