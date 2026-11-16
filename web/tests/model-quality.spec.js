import { test, expect } from "@playwright/test";
test("3D dress uses a seamless repeat, physical UV scale and floor-only shadows", async ({
  page,
}, info) => {
  const errors = [];
  page.on("pageerror", (e) => errors.push(e.message));
  await page.goto("/");
  const state = await page.evaluate(async () => {
    const { createModel } = await import("/src/model.js");
    const { defaults, motif, renderRepeat } = await import("/src/core.js");
    const project = defaults();
    project.shapes = motif("arch");
    const tile = document.createElement("canvas");
    renderRepeat(tile, project, 0, 512);
    const host = document.createElement("div");
    host.id = "quality-scene";
    Object.assign(host.style, {
      width: "640px",
      height: "900px",
      position: "fixed",
      left: "0",
      top: "0",
      zIndex: "100",
    });
    document.body.append(host);
    const model = createModel(host, tile, (message) => {
      throw Error(message);
    });
    window.qualityModel = model;
    model.update(165);
    const scaledRepeat = model.dress.material.map.repeat.x;
    model.update(110);
    return {
      textureWidth: model.dress.material.map.image.width,
      textureHeight: model.dress.material.map.image.height,
      vertexCount: model.dress.geometry.attributes.position.count,
      receivesShadow: model.dress.receiveShadow,
      shadowReceivers: model.dress.parent.children.filter(
        (o) => o.isMesh && o.receiveShadow,
      ).length,
      scaledRepeat,
      defaultRepeat: model.dress.material.map.repeat.x,
    };
  });
  expect(state.textureWidth).toBe(512);
  expect(state.textureHeight).toBe(512);
  expect(state.vertexCount).toBeGreaterThan(9000);
  expect(state.receivesShadow).toBe(false);
  expect(state.shadowReceivers).toBe(1);
  expect(state.scaledRepeat).toBeCloseTo(2 / 3);
  expect(state.defaultRepeat).toBe(1);
  await page
    .locator("#quality-scene canvas")
    .screenshot({ path: info.outputPath("dress-front.png") });
  await page.locator("#quality-scene canvas").hover();
  await page.mouse.wheel(0, -250);
  await page.waitForTimeout(250);
  await page
    .locator("#quality-scene canvas")
    .screenshot({ path: info.outputPath("dress-closeup.png") });
  await page.mouse.move(300, 450);
  await page.mouse.down();
  await page.mouse.move(430, 450, { steps: 10 });
  await page.mouse.up();
  await page.waitForTimeout(250);
  await page
    .locator("#quality-scene canvas")
    .screenshot({ path: info.outputPath("dress-side.png") });
  await page.evaluate(() => window.qualityModel.dispose());
  expect(errors).toEqual([]);
});
