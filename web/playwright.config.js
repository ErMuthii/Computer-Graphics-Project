import { defineConfig } from "@playwright/test";
export default defineConfig({
  testDir: "./tests",
  testMatch: "*.spec.js",
  workers: 1,
  timeout: 90000,
  use: {
    baseURL: "http://127.0.0.1:5178",
    channel: process.env.PLAYWRIGHT_CHROME === "1" ? "chrome" : undefined,
    viewport: { width: 1440, height: 1000 },
    launchOptions: { args: ["--enable-unsafe-swiftshader"] },
  },
  webServer: {
    command: "npm run dev -- --port 5178 --strictPort",
    url: "http://127.0.0.1:5178",
    reuseExistingServer: true,
  },
  reporter: "list",
});
