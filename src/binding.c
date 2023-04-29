#include <node_api.h>
#include "runtime.h"
#include "context.h"
#include "libc.h"

NAPI_MODULE_INIT() {
  register_qjs_runtime_class(env, exports);
  register_qjs_context_class(env, exports);
  register_qjs_libc_namespace(env, exports);
  return exports;
}
