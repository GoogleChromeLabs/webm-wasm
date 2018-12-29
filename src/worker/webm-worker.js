import webmWasm from "../../dist/webm-wasm.js";

function nextEvent(target, name) {
  return new Promise(resolve => {
    target.addEventListener(name, resolve, { once: true });
  });
}

export function initWasmModule(moduleFactory, wasmUrl) {
  return new Promise(resolve => {
    const module = moduleFactory({
      // Just to be safe, don't automatically invoke any wasm functions
      noInitialRun: true,
      locateFile(url) {
        // Redirect the request for the wasm binary to whatever webpack gave us.
        if (url.endsWith(".wasm")) return wasmUrl;
        return url;
      },
      onRuntimeInitialized() {
        // An Emscripten is a then-able that resolves with itself, causing an infite loop when you
        // wrap it in a real promise. Delete the `then` prop solves this for now.
        // https://github.com/kripken/emscripten/issues/5820
        delete module.then;
        resolve(module);
      }
    });
  });
}

async function init() {
  const wasmPath = (await nextEvent(self, "message")).data;
  const module = await initWasmModule(webmWasm, wasmPath);
  postMessage("READY");
  const config = (await nextEvent(self, "message")).data;
  const instance = new module.WebmEncoder(...config, chunk => {
    const copy = new Uint8Array(chunk);
    postMessage(copy.buffer, [copy.buffer]);
  });
  addEventListener("message", function l(ev) {
    if (!ev.data) {
      // This will invoke the callback to flush
      instance.finalize();
      // signal the end-of-stream
      postMessage(null);
      instance.delete();
      return;
    }
    instance.addRGBAFrame(ev.data);
  });
}
init();
