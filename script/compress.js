const zlib = require('zlib')
const fs = require('fs')
const path = require('path')
const { pipeline } = require('stream')

const js = process.argv[2]
const wasm = path.join(path.dirname(js), path.basename(js, path.extname(js)) + '.wasm')

pipeline(
  fs.createReadStream(wasm),
  zlib.createBrotliCompress(),
  fs.createWriteStream(wasm + '.br'),
  (err) => {
    if (err) {
      console.error(err)
      process.exit(1)
    }
  }
)
