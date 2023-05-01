#include <quickjs.h>
#include <quickjs-libc.h>
#include "std.h"
#include "helper_macro.h"

#ifdef __wasm__
#include <emnapi.h>
#endif

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

static napi_value qjs_std_free_handlers(napi_env env, napi_callback_info info) {
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Runtime")
  NAPI_UNWRAP(env, argv[0], &rt, "Invalid Runtime");
  js_std_free_handlers(rt);
  return NULL;
}

static napi_value qjs_std_dump_error(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Context")
  NAPI_UNWRAP(env, argv[0], &ctx, "Invalid Context");
  js_std_dump_error(ctx);
  return NULL;
}

static JSValue eval_binary(JSContext *ctx, const uint8_t *buf, size_t buf_len,
                        int load_only)
{
    JSValue obj, val;
    obj = JS_ReadObject(ctx, buf, buf_len, JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj))
        return obj;
    if (load_only) {
        if (JS_VALUE_GET_TAG(obj) == JS_TAG_MODULE) {
            js_module_set_import_meta(ctx, obj, 0, 0);
        }
        return JS_UNDEFINED;
    } else {
        if (JS_VALUE_GET_TAG(obj) == JS_TAG_MODULE) {
            if (JS_ResolveModule(ctx, obj) < 0) {
                JS_FreeValue(ctx, obj);
                return JS_EXCEPTION;
            }
            js_module_set_import_meta(ctx, obj, 0, 1);
        }
        val = JS_EvalFunction(ctx, obj);
        return val;
    }
}

napi_value to_napi_value(napi_env env, JSContext* ctx, JSValue val);

static napi_value qjs_std_eval_binary(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  size_t argc = 3;
  napi_value argv[3];
  napi_value this_arg;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
  if (argc < 2) {
    NAPI_CALL(env, napi_throw_error(env, NULL, "Invalid arguments"));
    return NULL;
  }
  NAPI_UNWRAP(env, argv[0], &ctx, "Invalid Context");
  bool is_typedarray;
  int32_t load_only = 0;
  if (argc > 2) {
    napi_valuetype t;
    NAPI_CALL(env, napi_typeof(env, argv[2], &t));
    if (t == napi_number) {
      NAPI_CALL(env, napi_get_value_int32(env, argv[2], &load_only));
    } else if (t == napi_boolean) {
      bool load;
      NAPI_CALL(env, napi_get_value_bool(env, argv[2], &load));
      load_only = load ? 1 : 0;
    } else {
      NAPI_CALL(env, napi_throw_error(env, NULL, "Invalid flags"));
      return NULL;
    }
  }

  NAPI_CALL(env, napi_is_typedarray(env, argv[1], &is_typedarray));
  if (!is_typedarray) {
    NAPI_CALL(env, napi_throw_type_error(env, NULL, "Not TypedArray"));
    return NULL;
  }
  napi_typedarray_type t;
  size_t len = 0;
  void* data = NULL;
  NAPI_CALL(env, napi_get_typedarray_info(env, argv[1], &t, &len, &data, NULL, NULL));
  if (len == 0) {
    return NULL;
  }
  switch (t) {
    case napi_int8_array:
    case napi_uint8_array:
    case napi_uint8_clamped_array:
      break;
    case napi_int16_array:
    case napi_uint16_array:
      len *= 2;
      break;
    case napi_int32_array:
    case napi_uint32_array:
    case napi_float32_array:
      len *= 4;
      break;
    case napi_float64_array:
    case napi_bigint64_array:
    case napi_biguint64_array:
      len *= 8;
      break;
    
    default:
      break;
  }
  JSValue val = eval_binary(ctx, (uint8_t*)data, len, load_only);
#ifdef __wasm__
  if (!emnapi_is_support_weakref()) {
    free(data);
  }
#endif
  if (JS_IsException(val)) {
    JSValue error = JS_GetException(ctx);
    JSValue message = JS_GetPropertyStr(ctx, error, "message");
    const char* msg = JS_ToCString(ctx, message);
    NAPI_CALL(env, napi_throw_error(env, NULL, msg));
    JS_FreeValue(ctx, message);
    JS_FreeValue(ctx, error);
  }

  napi_value ret = to_napi_value(env, ctx, val);
  JS_FreeValue(ctx, val);
  return ret;
}

void register_qjs_std_namespace(napi_env env, napi_value exports) {
  napi_value ns;
  NAPI_CALL_VOID(env, napi_create_object(env, &ns));
  NAPI_SET_FUNCTION_VOID(env, ns, "loop", qjs_std_loop);
  NAPI_SET_FUNCTION_VOID(env, ns, "initHandlers", qjs_std_init_handlers);
  NAPI_SET_FUNCTION_VOID(env, ns, "freeHandlers", qjs_std_free_handlers);
  NAPI_SET_FUNCTION_VOID(env, ns, "dumpError", qjs_std_dump_error);
  NAPI_SET_FUNCTION_VOID(env, ns, "evalBinary", qjs_std_eval_binary);
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "std", ns));
}
