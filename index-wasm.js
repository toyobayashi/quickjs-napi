const { getDefaultContext } = require('@emnapi/runtime')
const emscriptenInit = require('./build/emscripten/qjs_binding.js')

let promise = null

exports.init = function (options) {
  if (!promise) promise = emscriptenInit(options)
  return promise.then(Module => {
    return Module.emnapiInit({ context: getDefaultContext(), ...options })
  }).catch((err) => {
    promise = null
    throw err
  })
}
