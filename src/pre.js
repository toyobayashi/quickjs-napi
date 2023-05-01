if (typeof WebAssembly !== 'object' && typeof WXWebAssembly !== 'undefined') {
  WXWebAssembly.RuntimeError = WXWebAssembly.RuntimeError || (function () {
    class RuntimeError extends Error {
      constructor (message) {
        super(message)

        if (!(this instanceof RuntimeError)) {
          const setPrototypeOf = Object.setPrototypeOf
          if (typeof setPrototypeOf === 'function') {
            setPrototypeOf.call(Object, this, RuntimeError.prototype)
          } else {
            this.__proto__ = RuntimeError.prototype
          }
          if (typeof Error.captureStackTrace === 'function') {
            Error.captureStackTrace(this, RuntimeError)
          }
        }
      }
    }

    Object.defineProperty(RuntimeError.prototype, 'name', {
      configurable: true,
      writable: true,
      value: 'RuntimeError'
    })

    return RuntimeError
  })();

  (function () {
    if (typeof globalThis !== 'undefined') return globalThis

    var g = (function () { return this })()
    var supportNewFunction = (function () {
      var f
      try {
        f = new Function()
      } catch (_) {
        return false
      }
      return typeof f === 'function'
    })()
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
  })().WebAssembly = WXWebAssembly
}
