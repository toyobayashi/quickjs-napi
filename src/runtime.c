#include <quickjs.h>
#include <quickjs-libc.h>
#include "runtime.h"
#include "helper_macro.h"

static napi_value qjs_runtime_constructor(napi_env env, napi_callback_info info) {
  JSRuntime* rt;
  NAPI_GET_CB_INFO_THIS(env, info)
  rt = JS_NewRuntime();
  NAPI_CALL(env, napi_wrap(env, this_arg, rt, NULL, NULL, NULL));
  return this_arg;
}

static napi_value qjs_runtime_data(napi_env env, napi_callback_info info) {
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_UNWRAP(env, this_arg, &rt, "Invalid Runtime");

  napi_value ret;
  NAPI_CREATE_POINTER_VALUE(env, rt, &ret);
  return ret;
}

static napi_value qjs_runtime_dispose(napi_env env, napi_callback_info info) {
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_REMOVE_WRAP(env, this_arg, &rt, "Invalid Runtime");

  JS_FreeRuntime(rt);
  return NULL;
}

void register_qjs_runtime_class(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    {
      "dispose", NULL,
      qjs_runtime_dispose, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    },
    {
      "data", NULL,
      qjs_runtime_data, NULL, NULL, NULL,
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
