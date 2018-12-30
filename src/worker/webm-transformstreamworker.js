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

const defaultConfig = {
  width: 300,
  height: 150,
  timebaseNum: 1,
  timebaseDen: 30,
  bitrate: 200,
  realtime: false
};

function createTransformStream(module, userParams) {
  let instance;
  const ts = new TransformStream({
    start(controller) {
      const params = Object.assign({}, defaultConfig, userParams);
      instance = new module.WebmEncoder(
        params.timebaseNum,
        params.timebaseDen,
        params.width,
        params.height,
        params.bitrate,
        params.realtime,
        chunk => {
          const copy = new Uint8Array(chunk);
          controller.enqueue(copy.buffer);
        }
      );
      if (instance.lastError()) {
        console.error(instance.lastError());
        controller.close();
      }
    },
    transform(chunk, controller) {
      if (!instance.addRGBAFrame(chunk)) {
        console.error(instance.lastError());
        controller.close();
      }
    },
    flush() {
      // This will invoke the callback to flush
      instance.finalize();
      instance.delete();
    }
  });
  postMessage(ts, [ts]);
}

async function init() {
  const wasmPath = (await nextEvent(self, "message")).data;
  const module = await initWasmModule(webmWasm, wasmPath);
  addEventListener("message", ev => createTransformStream(module, ev.data));
  postMessage("READY");
}
init();
