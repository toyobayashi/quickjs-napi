var supportNewFunction = (function () {
  let f
  try {
    f = new Function()
  } catch (_) {
    return false
  }
  return typeof f === 'function'
})()
var _global = (function () {
  if (typeof globalThis !== 'undefined') return globalThis

  let g = (function () { return this })()
  if (!g && supportNewFunction) {
    try {
      g = new Function('return this')()
    } catch (_) {}
  }

  if (!g) {
    if (typeof __webpack_public_path__ === 'undefined') {
      if (typeof global !== 'undefined') return global
    }
    if (typeof window !== 'undefined') return window
    if (typeof self !== 'undefined') return self
  }

  return g
})()
var WebAssembly = typeof wx !== 'undefined' ? _global.WXWebAssembly : _global.WebAssembly
var window = _global
WebAssembly.RuntimeError = WebAssembly.RuntimeError || function RuntimeError (msg) {
  this.name = 'RuntimeError'
  this.message = msg
}
