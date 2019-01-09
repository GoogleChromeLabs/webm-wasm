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

function nextEvent(target, name) {
  return new Promise(resolve => {
    target.addEventListener(name, resolve, { once: true });
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

function createTransformStream(module, userParams) {
  let instance;
  const ts = new TransformStream({
    start(controller) {
      const params = Object.assign({}, defaultConfig, userParams);
      if(!('kLive' in params)) {
        params.kLive = params.realtime;
      }
      instance = new module.WebmEncoder(
        params.timebaseNum,
        params.timebaseDen,
        params.width,
        params.height,
        params.bitrate,
        params.realtime,
        params.kLive,
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
