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
    const encoder = new Module.WebmEncoder(...config, b => console.log(b));
    if(encoder.lastError()) {
      console.error(encoder.lastError());
      return;
    }
    addEventListener("message", function l(ev) {
      if(!ev.data) {
        // Copy from the actual wasm memory
        const data = new Uint8Array(encoder.finalize());
        postMessage(data.buffer, [data.buffer]);
        encoder.delete();
        removeEventListener("message", l);
        return;
      }
      encoder.addRGBAFrame(ev.data);
    });
  }
}
importScripts("../encoder.js");
