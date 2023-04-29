import { createRequire } from 'node:module'
import { fileURLToPath } from 'node:url'

const __dirname = fileURLToPath(import.meta.url)
const require = createRequire(__dirname)

const binding = require('./build/native/Release/qjs_binding.node')

export const {
  Runtime,
  Context,
  libc
} = binding

export default binding
