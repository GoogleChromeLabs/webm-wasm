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
const http = require("http");

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

function timeout(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function handler(req, res) {
  if (req.url !== "/video") {
    return res.end(`
      <!doctype html>
      <video src="/video" controls autoplay muted></video>
    `);
  }

  const worker = new Worker("../dist/webm-worker.js");
  worker.postMessage("./webm-wasm.wasm");
  await nextEvent(worker, "message");

  worker.postMessage({
    timebaseDen: framerate,
    width,
    height,
    bitrate,
    realtime: true
  });
  res.setHeader("Content-Type", "video/webm");
  worker.on("message", chunk => {
    res.write(Buffer.from(chunk));
  });

  let done = false;
  res.once("close", () => {
    worker.terminate();
    done = true;
  });

  let c = 0;
  while (!done) {
    for (let i = 0; i < 60; i++) {
      const buffer = new ArrayBuffer(width * height * 4);
      const view = new Uint32Array(buffer);
      view.fill(c++ | (0xff << 24));
      worker.postMessage(buffer, [buffer]);
    }
    await timeout(1000);
  }
}

const server = http.createServer(handler).listen(0);
console.log(`Started web server on http://localhost:${server.address().port}`);
