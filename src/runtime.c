#include <js_native_api.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include "helper_macro.h"

static napi_value qjs_runtime_constructor(napi_env env, napi_callback_info info) {
  napi_value this_arg;
  NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
  JSRuntime* rt = JS_NewRuntime();
  js_std_init_handlers(rt);
  NAPI_CALL(env, napi_wrap(env, this_arg, rt, NULL, NULL, NULL));
  return this_arg;
}

static napi_value qjs_runtime_dispose(napi_env env, napi_callback_info info) {
  napi_value this_arg;
  NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
  JSRuntime* rt = NULL;
  NAPI_CALL(env, napi_remove_wrap(env, this_arg, (void**)(&rt)));

  JS_FreeRuntime(rt);

  return NULL;
}

void register_qjs_runtime_class(napi_env env, napi_value exports) {
  napi_property_descriptor properties[1] = {
    {
      "dispose", NULL,
      qjs_runtime_dispose, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    }
  };

  size_t property_size = sizeof(properties) / sizeof(properties[0]);
  napi_value ctor;
  NAPI_CALL_VOID(env, napi_define_class(env, "Runtime", NAPI_AUTO_LENGTH,
                                        qjs_runtime_constructor, NULL,
                                        property_size, properties, &ctor));
  // NAPI_CALL_VOID(env, napi_create_reference(env, ctor, 1, &constructor_));
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "Runtime", ctor));
}
