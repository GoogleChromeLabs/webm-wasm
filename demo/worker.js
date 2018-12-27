Module = {
  locateFile(url) {
    if (url.endsWith('.wasm')) {
      return "../encoder.wasm";
    }
    return url;
  },
  async onRuntimeInitialized() {
    postMessage("READY");
    addEventListener("message", createInstance);
  }
}
importScripts("../encoder.js");

function createInstance(ev) {
  let controller;
  const encoder = new Module.WebmEncoder(...ev.data, b => {
    const copy = new Uint8Array(b);
    controller.enqueue(copy.buffer);
  });
  if(encoder.lastError()) {
    console.error(encoder.lastError());
    return;
  }
  const ts = new TransformStream({
    start(controller_) {
      controller = controller_;
    },
    transform(chunk) {
      if(!encoder.addRGBAFrame(chunk)) {
        console.error(encoder.lastError());
      }
    },
    flush(controller) {
      // This will invoke the callback to flush
      encoder.finalize();
      encoder.delete();
    }
  });
  postMessage(ts, [ts]);
}
