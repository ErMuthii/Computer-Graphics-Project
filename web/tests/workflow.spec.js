import { test, expect } from "@playwright/test";
test("drawing, editable shapes, undo, palette, project and PNG exports", async ({
  page,
}, testInfo) => {
  const errors = [];
  page.on("pageerror", (e) => errors.push(e.message));
  await page.goto("/");
  await expect(page.locator("#model canvas")).toBeVisible();
  await page.locator("#clear").click();
  await page.getByRole("button", { name: "Rectangle", exact: true }).click();
  const b = await page.locator("#editor").boundingBox();
  await page.mouse.move(b.x + b.width * 0.2, b.y + b.height * 0.2);
  await page.mouse.down();
  await page.mouse.move(b.x + b.width * 0.6, b.y + b.height * 0.6, {
    steps: 5,
  });
  await page.mouse.up();
  await expect(page.locator("#layers button")).toHaveCount(1);
  await page.locator("#shape-w").fill("180");
  await page.locator("#shape-w").press("Tab");
  await expect(page.locator("#shape-w")).toHaveValue("180");
  await page.locator("#undo").click();
  await expect(page.locator("#layers button")).toHaveCount(1);
  await page.locator("#redo").click();
  await page.getByRole("button", { name: "Freehand", exact: true }).click();
  await page.mouse.move(b.x + 50, b.y + 50);
  await page.mouse.down();
  await page.mouse.move(b.x + 100, b.y + 150, { steps: 12 });
  await page.mouse.up();
  await expect(page.locator("#layers button")).toHaveCount(2);
  await page.locator("#suggest").click();
  await expect(page.locator("#score")).not.toHaveText("—");
  const saved = page.waitForEvent("download");
  await page.locator("#save").click();
  const d = await saved;
  const path = await d.path();
  await page.locator("#open").setInputFiles(path);
  await expect(page.locator("#status")).toContainText("Project opened");
  const before = await page.locator("#pattern").evaluate((c) => c.toDataURL());
  await page.locator("#play").click();
  await expect
    .poll(() => page.locator("#pattern").evaluate((c) => c.toDataURL()))
    .not.toBe(before);
  await page.locator("#play").click();
  const view = page.waitForEvent("download");
  await page.locator("#export-model").click();
  expect((await view).suggestedFilename()).toBe("kitenge-3d-preview.png");
  const exported = page.waitForEvent("download");
  await page.locator("#export").click();
  expect((await exported).suggestedFilename()).toBe("kitenge-fabric.png");
  await page.screenshot({
    path: testInfo.outputPath("desktop.png"),
    fullPage: true,
  });
  expect(errors).toEqual([]);
});
test("real pose detection, manual correction and photo export", async ({
  page,
}, testInfo) => {
  await page.goto("/");
  await page.locator("#photo-tab").click();
  await page.locator("#photo-file").setInputFiles("tests/person.jpg");
  await expect(page.locator("#status")).toContainText("Person detected", {
    timeout: 80000,
  });
  await expect(page.locator("#photo")).toBeVisible();
  const correction = (
    Number(await page.locator("#anchor-x").inputValue()) + 1
  ).toFixed(1);
  await page.locator("#anchor-x").fill(correction);
  await page.locator("#anchor-x").press("Tab");
  await expect(page.locator("#anchor-x")).toHaveValue(correction);
  const d = page.waitForEvent("download");
  await page.locator("#export-photo").click();
  expect((await d).suggestedFilename()).toBe("kitenge-photo-fitting.png");
  await page.screenshot({
    path: testInfo.outputPath("photo.png"),
    fullPage: true,
  });
});
test("no-person photo and unavailable model retain manual fitting", async ({
  page,
}) => {
  await page.goto("/");
  const png = await page.locator("#editor").evaluate((c) => {
    const blank = document.createElement("canvas");
    blank.width = blank.height = 100;
    blank.getContext("2d").fillRect(0, 0, 100, 100);
    return blank.toDataURL().split(",")[1];
  });
  await page.locator("#photo-tab").click();
  await page.locator("#photo-file").setInputFiles({
    name: "empty.png",
    mimeType: "image/png",
    buffer: Buffer.from(png, "base64"),
  });
  await expect(page.locator("#status")).toContainText(
    "Shoulders and hips are not clear",
    { timeout: 80000 },
  );
  await expect(page.locator("#anchor-x")).toBeEnabled();
  await page.locator("#manual").click();
  await expect(page.locator("#anchor-x")).toHaveValue("34.0");
  await expect(page.locator("#export-photo")).toBeEnabled();
  await page.reload();
  await page.route("**/models/*.task", (route) => route.abort());
  await page.locator("#photo-tab").click();
  await page.locator("#photo-file").setInputFiles("tests/person.jpg");
  await expect(page.locator("#status")).toContainText(
    "Automatic alignment is unavailable",
  );
  await expect(page.locator("#anchor-x")).toBeEnabled();
  await expect(page.locator("#detect")).toBeEnabled();
  await page.unroute("**/models/*.task");
  await page.locator("#detect").click();
  await expect(page.locator("#status")).toContainText("Person detected", {
    timeout: 80000,
  });
});
test("mobile layout and malformed projects have recoverable feedback", async ({
  page,
}, testInfo) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto("/");
  await page.locator("#open").setInputFiles({
    name: "broken.json",
    mimeType: "application/json",
    buffer: Buffer.from("{}"),
  });
  await expect(page.locator("#status")).toContainText("valid Kitenge");
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= innerWidth,
    ),
  ).toBe(true);
  await page.screenshot({
    path: testInfo.outputPath("mobile.png"),
    fullPage: true,
  });
});
