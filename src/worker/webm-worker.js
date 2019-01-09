/**
 * Copyright 2018 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

function initWasmModule(moduleFactory, wasmUrl) {
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

import defaultConfig from "./defaults.js";

async function init() {
  const wasmPath = await nextMessage(parentPort, "message");
  const module = await initWasmModule(webmWasm, wasmPath);
  parentPort.postMessage("READY");
  const userParams = await nextMessage(parentPort, "message");
  const params = Object.assign({}, defaultConfig, userParams);
  if(!('kLive' in params)) {
    params.kLive = params.realtime;
  }
  const instance = new module.WebmEncoder(
    params.timebaseNum,
    params.timebaseDen,
    params.width,
    params.height,
    params.bitrate,
    params.realtime,
    params.kLive,
    chunk => {
      const copy = new Uint8Array(chunk);
      parentPort.postMessage(copy.buffer, [copy.buffer]);
    }
  );
  onMessage(parentPort, msg => {
    // A false-y message indicates the end-of-stream.
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
