# webm-wasm

webm-wasm lets you create webm videos in JavaScript via WebAssembly. The library consumes raw RGBA32 buffers (4 bytes per pixel) and turns them into a webm video with the given framerate and quality. This makes it compatible out-of-the-box with [`ImageData`][imagedata] from a `<canvas>`. With realtime mode you can also use webm-wasm for streaming webm videos.

Works in all major browsers (although Safari can’t play webm 🐼).

The wasm module was created by [emscripten’ing][emscripten] [libvpx], [libwebm] and [libyuv].

```
$ npm install --save webm-wasm
```

> Note: This is a proof-of-concept and not a production-grade library.

## Usage

webm-wasm runs in a worker by default. It works on the web and in in Node, although you need Node 11+ with the `--experimental-worker` flag.

### Quickstart

```js
// 1. Load the `webm-wasm.js` file in a worker
const worker = new Worker("webm-worker.js");
// 2. Send the path to the `.wasm` file
worker.postMessage("./webm-wasm.wasm");
// 3. Wait for the worker to be ready
await nextMessage(worker);
// 4. Send the parameters for the constructor
worker.postMessage({
  width: 512,
  height: 512
  // ... more constructor options below
});
// 5. Start sending frames!
while (hasNextFrame()) {
  // ArrayBuffer containing RGBA24 data
  const buffer = getFrame();
  worker.postMessage(buffer, [buffer]);
}
// 6. Signal end-of-stream
worker.postMessage(null);
// 7. Get the webm file as an ArrayBuffer
const webm = await nextMessage(worker);
// 8. Cleanup
worker.terminate();
```

(You can find an implementation of `nextMessage()` in [`src/worker/webm-worker.js`][nextmessage])

### Constructor options

- `width` (default: `300`): Width of the video
- `height` (default: `150`): Height of the video
- `timebaseNum` (default: `1`): Numerator of the fraction for the length of a frame
- `timebaseDen` (default: `30`): Denominator of the fraction for the length of a frame
- `bitrate` (default: `200`): Bitrate in kbps
- `realtime` (default: `false`): Prioritize encoding speed over compression ratio and quality. With realtime mode turned off the worker will send a single `ArrayBuffer` containing the entire webm video file once input stream has ended. With realtime mode turned on the worker will send an `ArrayBuffer` in regular intervals.

### From a CDN

Worker code can’t be loaded from another origin directly, even when the source is CORS-enabled. It is, however, still possible to load webm-wasm from a CDN like [unpkg.com] with a little workaround:

```js
const buffer = await fetch(
  "https://unpkg.com/webm-wasm@<version>/dist/webm-worker.js"
).then(r => r.arrayBuffer());
const worker = new Worker(
  URL.createObjectURL(new Blob([buffer], { type: "text/javascript" }))
);
worker.postMessage("https://unpkg.com/webm-wasm@<version>/dist/webm-wasm.wasm");
// Continue as normal
```

### WebAssembly

If you just want to use the WebAssembly module directly, you can grab `webm-wasm.wasm` as well as the the [Emscripten] glue code `webm-wasm.js`.

The WebAssembly module exposes a C++ class via [embind]:

```c++
class WebmEncoder {
  public:
    // Same options as above. `cb` is a callback function that takes an ArrayBuffer.
    WebmEncoder(int timebase_num, int timebase_den, unsigned int width, unsigned int height, unsigned int bitrate, bool realtime, val cb);
    bool addRGBAFrame(std::string rgba);
    bool finalize();
    std::string lastError();
    // ...
}
```

### Experimental: TransformStreams

[Transferable Streams] are behind the “Experimental Web Platform Features” flag in Chrome Canary. The alternative `webm-transformstreamworker.js` makes use of them to expose the webm encoder. Take a look at the demos to see the usage.

## Demos

To run the web demos, start the websever using

```
$ npm run serve
```

To run the node demos, run them directly (requires Node 11+):

```
$ node --experimental-worker ./node-simple.js
```

## Building

Because the build process is completely [Dockerized][docker], Docker is required for building webm-wasm.

```
$ npm install
$ npx napa
$ npm run build
```

---

Apache 2.0

[imagedata]: https://developer.mozilla.org/en-US/docs/Web/API/ImageData
[nextmessage]: https://github.com/GoogleChromeLabs/webm-wasm/blob/63b96a4f0e2821f34f972827f800259222ef9142/src/worker/webm-worker.js#L37-L46
[unpkg.com]: https://unpkg.com
[docker]: https://www.docker.com/
[transferable streams]: https://www.chromestatus.com/features/5298733486964736
[embind]: https://developers.google.com/web/updates/2018/08/embind
[emscripten]: https://kripken.github.io/emscripten-site/
[libvpx]: https://github.com/webmproject/libvpx
[libwebm]: https://github.com/webmproject/libwebm
[libyuv]: https://chromium.googlesource.com/libyuv/libyuv/
