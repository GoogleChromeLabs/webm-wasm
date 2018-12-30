import webmWasm from "../../dist/webm-wasm.js";

// Below you find the “Interop between Node and Web is fun” section.
// On the web you can communicate with the main thread via `self`.
// In node land you need to get the `parentPort` from the `worker_threads`
// module.
let parentPort;
try {
  ({ parentPort } = require("worker_threads"));
} catch (_) {
  parentPort = self;
}

// On the web you get a `MessageEvent` which has the message payload on
// it’s `.data` property.
// In node land the event is the message payload itself.
function onMessage(target, f) {
  if ("on" in target) {
    return target.on("message", f);
  }
  return target.addEventListener("message", e => f(e.data));
}

function nextMessage(target) {
  return new Promise(resolve => {
    if ("once" in target) {
      return target.once("message", resolve);
    }
    return target.addEventListener("message", e => resolve(e.data), {
      once: true
    });
  });
}

export function initWasmModule(moduleFactory, wasmUrl) {
  return new Promise(resolve => {
    const module = moduleFactory({
      // Just to be safe, don't automatically invoke any wasm functions
      noInitialRun: true,
      locateFile(url) {
        if (url.endsWith(".wasm")) {
          return wasmUrl;
        }
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
  const wasmPath = await nextMessage(parentPort, "message");
  const module = await initWasmModule(webmWasm, wasmPath);
  parentPort.postMessage("READY");
  const config = await nextMessage(parentPort, "message");
  const instance = new module.WebmEncoder(...config, chunk => {
    const copy = new Uint8Array(chunk);
    parentPort.postMessage(copy.buffer, [copy.buffer]);
  });
  onMessage(parentPort, msg => {
    // A false-y message indicated the end-of-stream.
    if (!msg) {
      // This will invoke the callback to flush
      instance.finalize();
      // Signal the end-of-stream
      parentPort.postMessage(null);
      // Free up the memory.
      instance.delete();
      return;
    }
    instance.addRGBAFrame(msg);
  });
}
init();
