export function canvasPNG(canvas) {
  return new Promise((resolve, reject) => {
    if (!canvas.width || !canvas.height)
      return reject(
        Error("The preview is empty. Try again after it finishes rendering."),
      );
    try {
      canvas.toBlob(
        (blob) =>
          blob?.size
            ? resolve(blob)
            : reject(Error("The browser could not create the PNG.")),
        "image/png",
      );
    } catch (error) {
      reject(error);
    }
  });
}

// Data-URL downloads can silently fail in embedded WebKit browsers. Keep a Blob
// URL alive for a real download link and a visible preview/retry path instead.
export function createExporter(host, onStatus) {
  let currentURL = null;
  host.innerHTML = `<img id="export-thumbnail" alt="Prepared image export" hidden><div class="export-copy"><h2 id="export-filename"></h2><p id="export-info"></p></div><a id="export-download" class="button primary">Save file again</a><a id="export-preview" class="button" target="_blank" rel="noopener">Open preview</a><button id="export-dismiss" aria-label="Dismiss prepared export">Close</button>`;
  const find = (id) => host.querySelector("#" + id);
  const dismiss = () => {
    host.hidden = true;
    if (currentURL) URL.revokeObjectURL(currentURL);
    currentURL = null;
    find("export-thumbnail").removeAttribute("src");
  };
  find("export-dismiss").onclick = dismiss;
  window.addEventListener("beforeunload", dismiss);
  return {
    deliver(blob, filename) {
      if (!(blob instanceof Blob) || !blob.size)
        throw Error("The export is empty. Please try again.");
      if (currentURL) URL.revokeObjectURL(currentURL);
      currentURL = URL.createObjectURL(blob);
      const download = find("export-download"),
        preview = find("export-preview"),
        image = find("export-thumbnail");
      download.href = currentURL;
      download.download = filename;
      preview.href = currentURL;
      const isImage = blob.type.startsWith("image/");
      image.hidden = !isImage;
      preview.hidden = !isImage;
      if (isImage) image.src = currentURL;
      else image.removeAttribute("src");
      find("export-filename").textContent = filename;
      find("export-info").textContent =
        `${(blob.size / 1024).toFixed(1)} KB ready. Check Downloads. If no file appears, use Save file again or open the preview.`;
      host.hidden = false;
      // Keep the anchor in the document and trigger it while its Blob URL is live.
      download.click();
      onStatus(
        `Download requested: ${filename}. The save link remains available at the top of the studio.`,
      );
      return currentURL;
    },
    async canvas(canvas, filename) {
      this.deliver(await canvasPNG(canvas), filename);
    },
    json(value, filename) {
      this.deliver(
        new Blob([JSON.stringify(value, null, 2)], {
          type: "application/json",
        }),
        filename,
      );
    },
  };
}
