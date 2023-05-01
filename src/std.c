#include <quickjs.h>
#include <quickjs-libc.h>
#include "std.h"
#include "helper_macro.h"

static napi_value qjs_std_loop(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Context")
  NAPI_UNWRAP(env, argv[0], &ctx, "Invalid Context");
  js_std_loop(ctx);
  return NULL;
}

static napi_value qjs_std_init_handlers(napi_env env, napi_callback_info info) {
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Runtime")
  NAPI_UNWRAP(env, argv[0], &rt, "Invalid Runtime");
  js_std_init_handlers(rt);
  return NULL;
}

void register_qjs_std_namespace(napi_env env, napi_value exports) {
  napi_value ns;
  NAPI_CALL_VOID(env, napi_create_object(env, &ns));
  NAPI_SET_FUNCTION_VOID(env, ns, "loop", qjs_std_loop);
  NAPI_SET_FUNCTION_VOID(env, ns, "initHandlers", qjs_std_init_handlers);
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "std", ns));
}
