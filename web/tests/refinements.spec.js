import { test, expect } from "@playwright/test";
test("motif library, theme library and multiple accents preserve editable project colours", async ({
  page,
}, info) => {
  const errors = [];
  page.on("pageerror", (e) => errors.push(e.message));
  await page.goto("/");
  await expect(page.locator("[data-preset]")).toHaveCount(18);
  await expect(page.locator("#palette-preset option")).toHaveCount(25);
  await expect(page.locator("#swatches button")).toHaveCount(84);
  for (const id of ["arch", "sunburst", "spiral", "cowrie"]) {
    await page.locator(`[data-preset="${id}"]`).click();
    expect(await page.locator("#layers button").count()).toBeGreaterThan(0);
  }
  await page.locator("#add-accent").click();
  await expect(page.locator("[data-color]")).toHaveCount(4);
  await page.getByLabel("Accent 2 hex", { exact: true }).fill("#00ffcc");
  await page.getByLabel("Accent 2 hex", { exact: true }).press("Tab");
  await expect(page.getByLabel("Accent 2 colour", { exact: true })).toHaveValue(
    "#00ffcc",
  );
  await page.locator("#layers button").first().click();
  await page.locator("#drawing-role").selectOption("3");
  const saved = page.waitForEvent("download");
  await page.locator("#save").click();
  const path = await (await saved).path();
  await page.locator("#remove-accent").click();
  await expect(page.locator("[data-color]")).toHaveCount(3);
  await page.locator("#undo").click();
  await expect(page.locator("[data-color]")).toHaveCount(4);
  await page.locator("#open").setInputFiles(path);
  await expect(page.locator("#status")).toContainText("Project opened");
  await page.locator("#layers button").first().click();
  await expect(page.locator("#drawing-role")).toHaveValue("3");
  await page.locator("#swatch-family").selectOption("Earth");
  await expect(page.locator("#swatches button")).toHaveCount(12);
  await page
    .getByRole("button", { name: "Earth 7 · #31553c", exact: true })
    .click();
  await expect(page.getByLabel("Accent 2 hex", { exact: true })).toHaveValue(
    "#31553c",
  );
  await page.locator("#palette-preset").selectOption("21");
  await page.locator('[data-preset="sunburst"]').click();
  await page.evaluate(() => scrollTo(0, 0));
  await page.screenshot({
    path: info.outputPath("expanded-studio.png"),
    fullPage: true,
  });
  expect(errors).toEqual([]);
});
test("enlarged fitting, neckline, print controls and correction brush undo", async ({
  page,
}, info) => {
  await page.goto("/");
  await page.locator("#photo-tab").click();
  await page.locator("#photo-file").setInputFiles("tests/person.jpg");
  await expect(page.locator("#status")).toContainText("Person detected", {
    timeout: 80000,
  });
  await expect(page.locator("#arm-mask")).not.toBeChecked();
  await page.locator("#enlarge-photo").click();
  await expect(page.locator("main")).toHaveClass(/fitting-focus/);
  await page.locator("#neckline").selectOption("v");
  await page.locator("#print-size").fill("140");
  await page.locator("#print-size").dispatchEvent("input");
  await page.locator("#photo").scrollIntoViewIfNeeded();
  const before = await page.locator("#photo").evaluate((c) => c.toDataURL());
  await page.locator('[data-photo-tool="protect"]').click();
  const b = await page.locator("#photo").boundingBox();
  await page.mouse.move(b.x + b.width * 0.47, b.y + b.height * 0.65);
  await page.mouse.down();
  await page.mouse.move(b.x + b.width * 0.53, b.y + b.height * 0.7, {
    steps: 8,
  });
  await page.mouse.up();
  await expect(page.locator("#mask-undo")).toBeEnabled();
  await expect
    .poll(() => page.locator("#photo").evaluate((c) => c.toDataURL()))
    .not.toBe(before);
  await page.locator("#mask-undo").click();
  await expect
    .poll(() => page.locator("#photo").evaluate((c) => c.toDataURL()))
    .toBe(before);
  await page
    .locator("#photo-panel")
    .screenshot({ path: info.outputPath("shaped-fitting.png") });
});
test("neckline exposes source pixels and correction mask can reveal and restore photograph", async ({
  page,
}) => {
  await page.goto("/");
  const result = await page.evaluate(async () => {
    const { drawPhoto, paintMaskStroke, rebuildMask } = await import(
      "/src/photo.js"
    );
    const canvas = document.createElement("canvas");
    canvas.width = 400;
    canvas.height = 600;
    const image = document.createElement("canvas");
    image.width = 400;
    image.height = 600;
    const c = image.getContext("2d");
    c.fillStyle = "#334455";
    c.fillRect(0, 0, 400, 600);
    const tile = document.createElement("canvas");
    tile.width = tile.height = 20;
    tile.getContext("2d").fillStyle = "#cc3322";
    tile.getContext("2d").fillRect(0, 0, 20, 20);
    const mask = document.createElement("canvas");
    mask.width = 400;
    mask.height = 600;
    const anchors = [
      [0.3, 0.2],
      [0.7, 0.2],
      [0.64, 0.5],
      [0.8, 0.9],
      [0.2, 0.9],
      [0.36, 0.5],
    ];
    const pixel = (x, y) =>
      [...canvas.getContext("2d").getImageData(x, y, 1, 1).data].slice(0, 3);
    const draw = () =>
      drawPhoto(canvas, image, tile, anchors, {
        mask,
        handles: false,
        shading: false,
      });
    draw();
    const neck = pixel(200, 125),
      chest = pixel(200, 200),
      bare = pixel(30, 300);
    paintMaskStroke(mask, {
      mode: "protect",
      size: 0.08,
      points: [[0.5, 0.6]],
    });
    draw();
    const protectedPixel = pixel(200, 360);
    paintMaskStroke(mask, {
      mode: "restore",
      size: 0.08,
      points: [[0.5, 0.6]],
    });
    draw();
    const restored = pixel(200, 360);
    rebuildMask(mask, []);
    draw();
    const cleared = pixel(200, 360);
    // A dotted texture must keep the same spacing at the bust and skirt.
    const t = tile.getContext("2d");
    t.fillStyle = "#00ff00";
    t.fillRect(0, 0, 20, 20);
    t.fillStyle = "#ff0000";
    t.fillRect(0, 0, 5, 20);
    draw();
    const row = (y) => {
      let transitions = [];
      for (let x = 145; x < 255; x++) {
        if (pixel(x, y)[0] > 127 && pixel(x - 1, y)[0] <= 127)
          transitions.push(x);
      }
      return transitions;
    };
    return {
      neck,
      chest,
      bare,
      protectedPixel,
      restored,
      cleared,
      upper: row(230),
      lower: row(430),
    };
  });
  expect(result.neck).toEqual([51, 68, 85]);
  expect(result.bare).toEqual([51, 68, 85]);
  expect(result.chest).toEqual([204, 51, 34]);
  expect(result.protectedPixel).toEqual([51, 68, 85]);
  expect(result.restored).toEqual([204, 51, 34]);
  expect(result.cleared).toEqual([204, 51, 34]);
  expect(result.upper.length).toBeGreaterThan(1);
  expect(result.lower).toEqual(result.upper);
});
