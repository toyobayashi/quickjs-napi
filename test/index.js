if (typeof window !== 'undefined') {
  // const quickjs = require('../build/emscripten/qjs_binding.js')
  // const emnapi = require('@emnapi/runtime')
  
  quickjs().then(Module => {
    const binding = Module.emnapiInit({ context: emnapi.getDefaultContext() })
    main(binding)
  })
} else {
  main(require('..'))
}

/**
 * @param {import('..')} binding 
 */
function main (binding) {
  const { Runtime, Context, std } = binding
  console.log(binding)
  const rt = new Runtime()
  console.log('rt:', rt.data())
  std.initHandlers(rt)
  const ctx = new Context(rt)
  console.log('ctx:', ctx.data())
  ctx.addIntrinsicBaseObjects()
  ctx.addIntrinsicDate()
  ctx.addIntrinsicEval()
  ctx.addIntrinsicStringNormalize()
  ctx.addIntrinsicRegExp()
  ctx.addIntrinsicJSON()
  ctx.addIntrinsicProxy()
  ctx.addIntrinsicMapSet()
  ctx.addIntrinsicTypedArrays()
  ctx.addIntrinsicPromise()
  ctx.addIntrinsicBigInt()
  std.addHelpers(ctx)
  ctx.eval("Promise.resolve(Object.keys(globalThis))").then(console.log)
  console.log(ctx.eval('undefined'))
  console.log(ctx.eval('null'))
  console.log(ctx.eval('true'))
  console.log(ctx.eval('false'))
  console.log(ctx.eval('NaN'))
  console.log(ctx.eval('Date.now()'))
  console.log(ctx.eval('"string"'))
  console.log(ctx.eval('Symbol("local.symbol")'))
  console.log(ctx.eval('(a = { c: 1 }, a.b = a, a)'))
  console.log(ctx.eval('(x = [,1], x[2] = x, x[3] = a, x)'))
  console.log(ctx.eval('new TypeError("TypeError")'))
  const fn = ctx.eval('((f, a) => f(a))')
  console.log(fn((arg) => { console.log('ohhhhhhhhhhhhhhhhhhhhh'); return arg }, (a = { c: 1 }, a.b = a, a)))
  fn.dispose()
  try {
    ctx.eval('(() => { throw new RangeError("RangeError") })()')
  } catch (err) {
    console.log(err instanceof RangeError && err.message === 'RangeError')
  }
  console.log(ctx.eval('/^a\\\\$/gi'))
  ctx.expose("hostBinding", function (a) { console.log(a) })
  console.log(ctx.eval('hostBinding(a)'))
  std.evalBinary(ctx, new Uint8Array([
    0x02, 0x04, 0x0e, 0x63, 0x6f, 0x6e, 0x73, 0x6f,
    0x6c, 0x65, 0x06, 0x6c, 0x6f, 0x67, 0x12, 0x68,
    0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x71, 0x6a, 0x73,
    0x10, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x2e, 0x6a,
    0x73, 0x0e, 0x00, 0x06, 0x00, 0xa0, 0x01, 0x00,
    0x01, 0x00, 0x03, 0x00, 0x00, 0x14, 0x01, 0xa2,
    0x01, 0x00, 0x00, 0x00, 0x38, 0xe1, 0x00, 0x00,
    0x00, 0x42, 0xe2, 0x00, 0x00, 0x00, 0x04, 0xe3,
    0x00, 0x00, 0x00, 0x24, 0x01, 0x00, 0xcd, 0x28,
    0xc8, 0x03, 0x01, 0x00
  ]))
  std.loop(ctx)
  std.freeHandlers(rt)
  ctx.dispose()
  rt.dispose()
}
