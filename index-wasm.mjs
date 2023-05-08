import { getDefaultContext } from '@emnapi/runtime'
import emscriptenInit from './build/emscripten/qjs_binding.js'

let promise = null

export function init (options) {
  if (!promise) promise = emscriptenInit(options)
  return promise.then(Module => {
    return Module.emnapiInit({ context: getDefaultContext(), ...options })
  }).catch((err) => {
    promise = null
    throw err
  })
}
