#include <node_api.h>
#include "runtime.h"
#include "context.h"
#include "std.h"
#include "functions.h"

NAPI_MODULE_INIT() {
  register_qjs_runtime_class(env, exports);
  register_qjs_context_class(env, exports);
  register_qjs_std_namespace(env, exports);
  register_qjs_functions(env, exports);
  return exports;
}
