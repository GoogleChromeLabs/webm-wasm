Module = {
  locateFile(url) {
    if (url.endsWith('.wasm')) {
      return "../encoder.wasm";
    }
    return url;
  },
  async onRuntimeInitialized() {
    postMessage("READY");
    const config = await new Promise(resolve => {
      addEventListener("message", ev => resolve(ev.data), {once: true});
    });
    const encoder = new Module.WebmEncoder(...config, b => {
      const copy = new Uint8Array(b);
      postMessage(copy.buffer, [copy.buffer]);
    });
    if(encoder.lastError()) {
      console.error(encoder.lastError());
      return;
    }
    addEventListener("message", function l(ev) {
      if(!ev.data) {
        // This will invoke the callback to flush
        encoder.finalize();
        // signal the end-of-stream
        postMessage(null);
        encoder.delete();
        return;
      }
      encoder.addRGBAFrame(ev.data);
    });
  }
}
importScripts("../encoder.js");
