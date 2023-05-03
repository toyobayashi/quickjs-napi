#include <string.h>
#include "helper_macro.h"
#include "conversion.h"

typedef struct cfunc_promise_data {
  napi_env env;
  napi_deferred deferred;
} cfunc_promise_data;

static JSValue promise_then_cb_fulfill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  free(data);
  JS_FreeValue(ctx, *func_data);
  napi_resolve_deferred(env, deferred, qjs_to_napi_value(env, ctx, *argv));
  return *argv;
}

static JSValue promise_then_cb_reject(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  free(data);
  JS_FreeValue(ctx, *func_data);
  napi_reject_deferred(env, deferred, qjs_to_napi_value(env, ctx, *argv));
  JS_Throw(ctx, *argv);
  return JS_UNDEFINED;
}

napi_value qjs_to_napi_value(napi_env env, JSContext* ctx, JSValue val) {
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
    JS_FreeCString(ctx, str);
  } else if (JS_IsBigInt(ctx, val)) {
    int64_t tmp;
    JS_ToBigInt64(ctx, &tmp, val);
    NAPI_CALL(env, napi_create_bigint_int64(env, tmp, &ret));
  } else if (JS_IsSymbol(val)) {
    JSAtom atom = JS_ValueToAtom(ctx, val);
    JSValue str = JS_AtomToString(ctx, atom);
    napi_value desc;
    const char* cstr = JS_ToCString(ctx, str);
    napi_create_string_utf8(env, cstr, NAPI_AUTO_LENGTH, &desc);
    napi_create_symbol(env, desc, &ret);
    JS_FreeCString(ctx, cstr);
    JS_FreeValue(ctx, str);
    JS_FreeAtom(ctx, atom);
  } else if (JS_IsObject(val)) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue promise_ctor = JS_GetPropertyStr(ctx, global, "Promise");
    if (!JS_IsUndefined(promise_ctor) && JS_IsInstanceOf(ctx, val, promise_ctor)) {
      JSValue then = JS_GetPropertyStr(ctx, val, "then");
      JSValue deferred_ref = JS_NewObject(ctx);
      napi_deferred deferred;
      cfunc_promise_data* data = (cfunc_promise_data*)malloc(sizeof(cfunc_promise_data));
      data->env = env;
      napi_create_promise(env, &deferred, &ret);
      data->deferred = deferred;
      JS_SetOpaque(deferred_ref, (void*)data);
      JSValue argv[2] = {
        JS_NewCFunctionData(ctx, promise_then_cb_fulfill, 1, 0, 1, &deferred_ref),
        JS_NewCFunctionData(ctx, promise_then_cb_reject, 1, 0, 1, &deferred_ref)
      };
      JSValue return_val = JS_Call(ctx, then, val, 2, argv);
      JS_FreeValue(ctx, argv[0]);
      JS_FreeValue(ctx, argv[1]);
      JS_FreeValue(ctx, return_val);
      JS_FreeValue(ctx, then);
      JS_FreeValue(ctx, promise_ctor);
      JS_FreeValue(ctx, global);
      return ret;
    }
    JS_FreeValue(ctx, promise_ctor);

    JSValue date_ctor = JS_GetPropertyStr(ctx, global, "Date");
    if (!JS_IsUndefined(date_ctor) && JS_IsInstanceOf(ctx, val, date_ctor)) {
      JSValue get_time = JS_GetPropertyStr(ctx, val, "getTime");
      JSValue return_val = JS_Call(ctx, get_time, val, 0, NULL);
      double time =JS_VALUE_GET_FLOAT64(return_val);
      JS_FreeValue(ctx, return_val);
      JS_FreeValue(ctx, get_time);
      JS_FreeValue(ctx, date_ctor);
      JS_FreeValue(ctx, global);
      NAPI_CALL(env, napi_create_date(env, time, &ret));
      return ret;
    }
    JS_FreeValue(ctx, date_ctor);

    JSValue regexp_ctor = JS_GetPropertyStr(ctx, global, "RegExp");
    if (!JS_IsUndefined(regexp_ctor) && JS_IsInstanceOf(ctx, val, regexp_ctor)) {
      const char* cstr = JS_ToCString(ctx, val);
      char* last_slash = strrchr(cstr, '/');
      char* mode_str = last_slash + 1;
      napi_value args[2];
      napi_create_string_utf8(env, cstr + 1, (size_t)last_slash - (size_t)cstr - 1, args);
      napi_create_string_utf8(env, mode_str, NAPI_AUTO_LENGTH, args + 1);
      JS_FreeCString(ctx, cstr);
      JS_FreeValue(ctx, regexp_ctor);
      JS_FreeValue(ctx, global);
      napi_value g, regexp;
      NAPI_CALL(env, napi_get_global(env, &g));
      NAPI_CALL(env, napi_get_named_property(env, g, "RegExp", &regexp));
      NAPI_CALL(env, napi_new_instance(env, regexp, 2, args, &ret));
      return ret;
    }
    JS_FreeValue(ctx, regexp_ctor);

    JS_FreeValue(ctx, global);
    return ret;
  } else {
    NAPI_CALL(env, napi_throw_type_error(env, NULL, "unsupported type"));
    return NULL;
  }
  return ret;
}
