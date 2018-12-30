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

const { Worker } = require("worker_threads");
const { writeFile } = require("fs").promises;

const width = 512;
const height = 512;
const framerate = 30;
const bitrate = 200;

// Returns the next <name> event of `target`.
function nextEvent(target, name) {
  return new Promise(resolve => {
    target.once(name, resolve);
  });
}

async function init() {
  const worker = new Worker("../dist/webm-worker.js");
  worker.postMessage("../dist/webm-wasm.wasm");
  await nextEvent(worker, "message");

  worker.postMessage({
    timebaseDen: framerate,
    width,
    height,
    bitrate
  });
  const maxFrames = 2 * framerate;
  for (let i = 0; i < maxFrames; i++) {
    const buffer = new ArrayBuffer(width * height * 4);
    const view = new Uint32Array(buffer);
    view.fill(Math.floor((255 * i) / maxFrames) | (0xff << 24));
    console.log(
      `Generating frame #${i} out of ${maxFrames} filled with 0x${new Uint8Array(
        buffer
      )[0].toString(16)}...`
    );
    worker.postMessage(buffer, [buffer]);
  }
  worker.postMessage(null);
  console.log(`Waiting for file...`);
  const webmFile = await nextEvent(worker, "message");
  console.log(`Writing to generated.webm...`);
  await writeFile("generated.webm", new Uint8Array(webmFile));
  console.log(`Done`);
  worker.terminate();
}
init();
