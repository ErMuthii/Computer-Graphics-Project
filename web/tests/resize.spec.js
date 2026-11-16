import { test, expect } from "@playwright/test";
async function drawTriangle(page) {
  await page.goto("/");
  await page.locator("#clear").click();
  await page.locator('[data-tool="triangle"]').click();
  await page.locator("#editor").scrollIntoViewIfNeeded();
  let box = await page.locator("#editor").boundingBox();
  const pos = (x, y) => ({
    x: box.x + (x * box.width) / 400,
    y: box.y + (y * box.height) / 400,
  });
  let p = pos(80, 90);
  await page.mouse.move(p.x, p.y);
  await page.mouse.down();
  p = pos(200, 210);
  await page.mouse.move(p.x, p.y, { steps: 5 });
  await page.mouse.up();
  await expect(page.locator('[data-tool="select"]')).toHaveAttribute(
    "aria-pressed",
    "true",
  );
  // The longer selection hint can move the canvas after drawing on narrow screens.
  box = await page.locator("#editor").boundingBox();
  return pos;
}
test("triangle corner and side dragging resize immediately with one-step undo and redo", async ({
  page,
}, info) => {
  const pos = await drawTriangle(page);

  let p = pos(200, 210);
  await page.mouse.move(p.x, p.y);
  await expect(page.locator("#editor")).toHaveCSS("cursor", "nwse-resize");
  await page.mouse.down();
  p = pos(250, 250);
  await page.mouse.move(p.x, p.y, { steps: 6 });
  await page.mouse.up();
  await expect(page.locator("#shape-w")).toHaveValue("170");
  await expect(page.locator("#shape-h")).toHaveValue("160");
  await page.locator("#undo").click();
  await page.locator("#layers button").click();
  await expect(page.locator("#shape-w")).toHaveValue("120");
  await page.locator("#redo").click();
  await page.locator("#layers button").click();
  await expect(page.locator("#shape-w")).toHaveValue("170");
  // Right-edge handle: widen without altering height.
  p = pos(250, 170);
  await page.mouse.move(p.x, p.y);
  await page.mouse.down();
  p = pos(280, 170);
  await page.mouse.move(p.x, p.y, { steps: 5 });
  await page.mouse.up();
  await expect(page.locator("#shape-w")).toHaveValue("200");
  await expect(page.locator("#shape-h")).toHaveValue("160");
  await page
    .locator("#editor")
    .screenshot({ path: info.outputPath("resize-handles.png") });
});
test("corner resizing works on a scaled mobile canvas and Shift preserves aspect ratio", async ({
  page,
}, info) => {
  await page.setViewportSize({ width: 390, height: 844 });
  const pos = await drawTriangle(page);
  let p = pos(200, 210);
  await page.mouse.move(p.x, p.y);
  await page.keyboard.down("Shift");
  await page.mouse.down();
  p = pos(250, 230);
  await page.mouse.move(p.x, p.y, { steps: 5 });
  await page.mouse.up();
  await page.keyboard.up("Shift");
  await expect(page.locator("#shape-w")).toHaveValue("170");
  await expect(page.locator("#shape-h")).toHaveValue("170");
  await page
    .locator("#editor")
    .screenshot({ path: info.outputPath("mobile-resize.png") });
});
