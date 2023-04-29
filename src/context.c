#include <js_native_api.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include "helper_macro.h"

static napi_value qjs_context_constructor(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv;
  napi_value this_arg;
  JSRuntime* rt = NULL;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &this_arg, NULL));
  if (argc == 0) {
    napi_throw_error(env, NULL, "Invalid quickjs runtime");
    return NULL;
  }
  NAPI_CALL(env, napi_unwrap(env, argv, (void**)&rt));
  JSContext* ctx = JS_NewContextRaw(rt);
  if (ctx == NULL) {
    NAPI_CALL(env, napi_throw_error(env, NULL, "failed to create context"));
    return NULL;
  }
  JS_AddIntrinsicBaseObjects(ctx);
  JS_AddIntrinsicDate(ctx);
  JS_AddIntrinsicEval(ctx);
  JS_AddIntrinsicStringNormalize(ctx);
  JS_AddIntrinsicRegExp(ctx);
  JS_AddIntrinsicJSON(ctx);
  JS_AddIntrinsicProxy(ctx);
  JS_AddIntrinsicMapSet(ctx);
  JS_AddIntrinsicTypedArrays(ctx);
  JS_AddIntrinsicPromise(ctx);
  JS_AddIntrinsicBigInt(ctx);
  js_std_add_helpers(ctx, 0, NULL);
  NAPI_CALL(env, napi_wrap(env, this_arg, ctx, NULL, NULL, NULL));
  return this_arg;
}

static napi_value qjs_context_dispose(napi_env env, napi_callback_info info) {
  napi_value this_arg;
  NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
  JSContext* ctx = NULL;
  NAPI_CALL(env, napi_remove_wrap(env, this_arg, (void**)(&ctx)));

  JS_FreeContext(ctx);

  return NULL;
}

static napi_value qjs_context_eval(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv;
  napi_value this_arg;
  JSContext* ctx = NULL;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &this_arg, NULL));
  if (argc == 0) {
    napi_throw_error(env, NULL, "Invalid source input");
    return NULL;
  }
  napi_valuetype t;
  NAPI_CALL(env, napi_typeof(env, argv, &t));
  if (t != napi_string) {
    napi_throw_error(env, NULL, "Source is not string");
    return NULL;
  }

  size_t len = 0;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv, NULL, 0, &len));
  char* buf = (char*)malloc(len + 1);
  napi_status r = napi_get_value_string_utf8(env, argv, buf, len + 1, &len);
  if (r != napi_ok) {
    free(buf);
    NAPI_CALL(env, r);
  }
  *(buf + len) = '\0';
  
  NAPI_CALL(env, napi_unwrap(env, this_arg, (void**)(&ctx)));
  JSValue value = JS_Eval(ctx, buf, len, "<evalScript>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    JSValue error = JS_GetException(ctx);
    JSValue message = JS_GetPropertyStr(ctx, error, "message");
    const char* msg = JS_ToCString(ctx, message);
    napi_throw_error(env, NULL, msg);
    JS_FreeValue(ctx, message);
    JS_FreeValue(ctx, error);
  }
  // TODO: translate JSValue to host object

  return NULL;
}

void register_qjs_context_class(napi_env env, napi_value exports) {
  napi_property_descriptor properties[2] = {
    {
      "dispose", NULL,
      qjs_context_dispose, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    },
    {
      "eval", NULL,
      qjs_context_eval, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    }
  };

  size_t property_size = sizeof(properties) / sizeof(properties[0]);
  napi_value ctor;
  NAPI_CALL_VOID(env, napi_define_class(env, "Context", NAPI_AUTO_LENGTH,
                                        qjs_context_constructor, NULL,
                                        property_size, properties, &ctor));
  // NAPI_CALL_VOID(env, napi_create_reference(env, ctor, 1, &constructor_));
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "Context", ctor));
}
