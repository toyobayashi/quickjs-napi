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
  ctx.eval("Promise.resolve(Object.keys(globalThis)).then(console.log)")
  std.loop(ctx)
  ctx.dispose()
  rt.dispose()
}
