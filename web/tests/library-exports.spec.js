import { test, expect } from "@playwright/test";
import { readFile } from "node:fs/promises";
const entries = [
  ["rust-diamonds", "a-line"],
  ["indigo-lattice", "maxi"],
  ["sunburst-gold", "fit-flare"],
  ["coastal-waves", "shift"],
  ["plum-rosette", "midi"],
  ["forest-leaves", "maxi"],
  ["coral-cowries", "a-line"],
  ["midnight-stars", "fit-flare"],
  ["earth-chevrons", "midi"],
  ["teal-fans", "shift"],
];
test("library has ten sets, loads each silhouette and supports undo", async ({
  page,
}, info) => {
  const errors = [];
  page.on("pageerror", (e) => errors.push(e.message));
  await page.goto("/");
  await page.locator("#library-nav").click();
  await expect(page.locator("[data-library]")).toHaveCount(10);
  await expect(page.locator("#library-nav")).toHaveCSS(
    "background-color",
    "rgb(60, 81, 57)",
  );
  await page.screenshot({
    path: info.outputPath("library-desktop.png"),
    fullPage: true,
  });
  for (const [id, style] of entries) {
    await page.locator(`[data-library="${id}"]`).click();
    await expect(page.locator("#dress-style")).toHaveValue(style);
    expect(await page.locator("#layers button").count()).toBeGreaterThan(1);
    await page.locator("#library-nav").click();
  }
  await page.locator('[data-library="indigo-lattice"]').click();
  await page.locator("#dress-style").selectOption("shift");
  await page.locator("#undo").click();
  await expect(page.locator("#dress-style")).toHaveValue("maxi");
  const saving = page.waitForEvent("download");
  await page.locator("#save").click();
  const file = await (await saving).path();
  expect(JSON.parse(await readFile(file, "utf8")).dressStyle).toBe("maxi");
  await page.locator("#dress-style").selectOption("midi");
  await page.locator("#open").setInputFiles(file);
  await expect(page.locator("#dress-style")).toHaveValue("maxi");
  expect(errors).toEqual([]);
});
async function pngDownload(page, selector, name, info) {
  const downloading = page.waitForEvent("download");
  await page.locator(selector).click();
  const download = await downloading;
  expect(download.suggestedFilename()).toBe(name);
  const target = info.outputPath(name);
  await download.saveAs(target);
  const png = await readFile(target);
  expect([...png.subarray(0, 8)]).toEqual([137, 80, 78, 71, 13, 10, 26, 10]);
  expect(png.length).toBeGreaterThan(500);
  expect(await page.locator("#export-download").getAttribute("href")).toMatch(
    /^blob:/,
  );
  return [png.readUInt32BE(16), png.readUInt32BE(20)];
}
test("fabric, dress, 3D and photo export real PNG files with a persistent retry link", async ({
  page,
}, info) => {
  await page.goto("/");
  expect(
    await pngDownload(page, "#export", "kitenge-fabric.png", info),
  ).toEqual([720, 480]);
  await expect(page.locator("#export-thumbnail")).toBeVisible();
  await expect(page.locator("#export-preview")).toHaveAttribute(
    "href",
    /^blob:/,
  );
  const retry = page.waitForEvent("download");
  await page.locator("#export-download").click();
  expect((await retry).suggestedFilename()).toBe("kitenge-fabric.png");
  expect(
    await pngDownload(page, "#export-dress", "kitenge-dress.png", info),
  ).toEqual([800, 1200]);
  const size = await pngDownload(
    page,
    "#export-model",
    "kitenge-3d-preview.png",
    info,
  );
  expect(size[0]).toBeGreaterThan(100);
  expect(size[1]).toBeGreaterThan(100);
  await page.locator("#photo-tab").click();
  await page.locator("#photo-file").setInputFiles("tests/person.jpg");
  await expect(page.locator("#status")).toContainText("Person detected", {
    timeout: 80000,
  });
  const photoSize = await page
    .locator("#photo")
    .evaluate((c) => [c.width, c.height]);
  expect(
    await pngDownload(page, "#export-photo", "kitenge-photo-fitting.png", info),
  ).toEqual(photoSize);
  await page.locator("#export-result").scrollIntoViewIfNeeded();
  await page.screenshot({ path: info.outputPath("export-receipt.png") });
});
test("mobile gallery fits the viewport and failed encoding reports an error", async ({
  page,
}, info) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto("/");
  await page.locator("#library-nav").click();
  await expect(page.locator("#library-nav")).toHaveCSS(
    "background-color",
    "rgb(60, 81, 57)",
  );
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= innerWidth,
    ),
  ).toBe(true);
  await page.screenshot({ path: info.outputPath("library-mobile.png") });
  await page.locator('[data-library="rust-diamonds"]').click();
  await page.evaluate(
    () =>
      (HTMLCanvasElement.prototype.toBlob = function (cb) {
        cb(null);
      }),
  );
  await page.locator("#export").click();
  await expect(page.locator("#status")).toContainText("Export failed");
  await expect(page.locator("#export")).toBeEnabled();
});
