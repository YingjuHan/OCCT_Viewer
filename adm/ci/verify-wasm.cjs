const fs = require('node:fs');
const crypto = require('node:crypto');

const file = process.argv[2];
if (!file) throw new Error('Usage: node verify-wasm.cjs path/to/Viewer-wasm.wasm');
const bytes = fs.readFileSync(file);
if (!WebAssembly.validate(bytes)) throw new Error(`Invalid WebAssembly module: ${file}`);
console.log(JSON.stringify({
  file,
  bytes: bytes.length,
  sha256: crypto.createHash('sha256').update(bytes).digest('hex'),
  valid: true,
}, null, 2));
