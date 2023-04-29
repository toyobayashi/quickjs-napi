#include <js_native_api.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include "helper_macro.h"
#include "libc.h"

static napi_value qjs_std_loop(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv;
  napi_value this_arg;
  JSContext* ctx = NULL;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &this_arg, NULL));
  if (argc == 0) {
    napi_throw_error(env, NULL, "Invalid quickjs context");
    return NULL;
  }
  NAPI_CALL(env, napi_unwrap(env, argv, (void**)&ctx));
  js_std_loop(ctx);
  return NULL;
}

void register_qjs_libc_namespace(napi_env env, napi_value exports) {
  napi_value ns;
  NAPI_CALL_VOID(env, napi_create_object(env, &ns));
  napi_value loop;
  NAPI_CALL_VOID(env, napi_create_function(env, "loop", NAPI_AUTO_LENGTH,
                                        qjs_std_loop, NULL, &loop));
  // NAPI_CALL_VOID(env, napi_create_reference(env, ctor, 1, &constructor_));
  NAPI_CALL_VOID(env, napi_set_named_property(env, ns, "loop", loop));
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "libc", ns));
}
