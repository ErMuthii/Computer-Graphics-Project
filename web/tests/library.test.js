import test from "node:test";
import assert from "node:assert/strict";
import { designLibrary, libraryProject } from "../src/library.js";
import { validateProject, defaults } from "../src/core.js";
import { garmentStyles } from "../src/garment-styles.js";
import { createDressGeometry } from "../src/dress-geometry.js";
test("ten independent library projects load as editable validated designs", () => {
  assert.equal(designLibrary.length, 10);
  assert.equal(new Set(designLibrary.map((e) => e.id)).size, 10);
  const usedStyles = new Set();
  for (const entry of designLibrary) {
    const p = libraryProject(entry.id),
      fresh = libraryProject(entry.id);
    assert.ok(p.shapes.length > 1);
    assert.equal(validateProject(p).dressStyle, entry.style);
    usedStyles.add(entry.style);
    p.palette[0] = "#000000";
    p.shapes[0].x = 0;
    assert.notEqual(fresh.palette[0], "#000000");
    assert.notEqual(fresh.shapes[0].x, 0);
    assert.notEqual(p.shapes[0].id, fresh.shapes[0].id);
  }
  assert.equal(usedStyles.size, 5);
});
test("all garment styles create finite meshes, while older projects default to A-line", () => {
  for (const s of garmentStyles) {
    const g = createDressGeometry({ style: s.id, rows: 32, segments: 32 });
    assert.ok(
      [...g.attributes.position.array, ...g.attributes.uv.array].every(
        Number.isFinite,
      ),
    );
    g.dispose();
  }
  const old = defaults();
  delete old.dressStyle;
  assert.equal(validateProject(old).dressStyle, "a-line");
  assert.throws(() => validateProject({ ...old, dressStyle: "unknown" }));
  const saved = JSON.parse(JSON.stringify(libraryProject("indigo-lattice")));
  assert.equal(validateProject(saved).dressStyle, "maxi");
});
