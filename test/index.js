if (typeof window !== 'undefined') {
  // const quickjs = require('../build/emscripten/qjs_binding.js')
  // const emnapi = require('@emnapi/runtime')
  
  quickjs().then(Module => {
    const binding = Module.emnapiInit({ context: emnapi.getDefaultContext() })
    main(binding)
  })
} else {
  main(require('../build/native/Release/qjs_binding.node'))
}

function main (binding) {
  const { Runtime, Context, libc } = binding
  console.log(binding)
  const rt = new Runtime()
  const ctx = new Context(rt)
  ctx.eval("Promise.resolve(Object.keys(globalThis)).then(console.log)")
  libc.loop(ctx)
  ctx.dispose()
  rt.dispose()
}
