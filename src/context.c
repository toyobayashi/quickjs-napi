#include <quickjs.h>
#include <quickjs-libc.h>
#include "context.h"
#include "helper_macro.h"

static napi_value qjs_context_constructor(napi_env env, napi_callback_info info) {
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Runtime")
  NAPI_UNWRAP(env, argv[0], &rt, "Invalid Runtime");
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
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_REMOVE_WRAP(env, this_arg, &ctx, "Invalid Context");

  JS_FreeContext(ctx);
  return NULL;
}

static napi_value qjs_context_data(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_UNWRAP(env, this_arg, &ctx, "Invalid Context");

  napi_value ret;
  NAPI_CREATE_POINTER_VALUE(env, ctx, &ret);
  return ret;
}

napi_value to_napi_value(napi_env env, JSContext* ctx, JSValue val) {
  napi_value ret = NULL;
  if (JS_IsUndefined(val)) {
    NAPI_CALL(env, napi_get_undefined(env, &ret));
  } else if (JS_IsNull(val)) {
    NAPI_CALL(env, napi_get_null(env, &ret));
  } else if (JS_IsBool(val)) {
    NAPI_CALL(env, napi_get_boolean(env, (bool)JS_VALUE_GET_BOOL(val), &ret));
  } else if (JS_IsNumber(val)) {
    if (JS_VALUE_IS_NAN(val)) {
      napi_value global;
      NAPI_CALL(env, napi_get_global(env, &global));
      NAPI_CALL(env, napi_get_named_property(env, global, "NaN", &ret));
    } else if (JS_VALUE_GET_TAG(val) == JS_TAG_FLOAT64) {
      NAPI_CALL(env, napi_create_double(env, JS_VALUE_GET_FLOAT64(val), &ret));
    } else {
      NAPI_CALL(env, napi_create_double(env, JS_VALUE_GET_INT(val), &ret));
    }
  } else if (JS_IsString(val)) {
    const char* str = JS_ToCString(ctx, val);
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &ret));
  } else if (JS_IsBigInt(ctx, val)) {
    int64_t tmp;
    JS_ToBigInt64(ctx, &tmp, val);
    NAPI_CALL(env, napi_create_bigint_int64(env, tmp, &ret));
  }/*  else {
    NAPI_CALL(env, napi_throw_type_error(env, NULL, "unsupported type"));
    return NULL;
  } */
  return ret;
}

static napi_value qjs_context_eval(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid source input")
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_string, "Source is not string");

  size_t len = 0;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &len));
  char* buf = (char*)malloc(len + 1);
  napi_status r = napi_get_value_string_utf8(env, argv[0], buf, len + 1, &len);
  if (r != napi_ok) {
    free(buf);
    NAPI_CALL(env, r);
  }
  *(buf + len) = '\0';

  NAPI_UNWRAP(env, this_arg, &ctx, "Invalid Context");
  JSValue value = JS_Eval(ctx, buf, len, "<evalScript>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    JSValue error = JS_GetException(ctx);
    JSValue message = JS_GetPropertyStr(ctx, error, "message");
    const char* msg = JS_ToCString(ctx, message);
    napi_throw_error(env, NULL, msg);
    JS_FreeValue(ctx, message);
    JS_FreeValue(ctx, error);
  }

  napi_value ret = to_napi_value(env, ctx, value);
  JS_FreeValue(ctx, value);
  return ret;
}

void register_qjs_context_class(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    {
      "dispose", NULL,
      qjs_context_dispose, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    },
    {
      "data", NULL,
      qjs_context_data, NULL, NULL, NULL,
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
