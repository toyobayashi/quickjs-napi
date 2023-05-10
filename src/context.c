#include <string.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include "context.h"
#include "helper_macro.h"
#include "conversion.h"
#include "runtime.h"

static napi_value qjs_context_constructor(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "Invalid Runtime")
  NAPI_UNWRAP(env, argv[0], &wrap, "Invalid Runtime");
  rt = wrap->value;
  JSContext* ctx = JS_NewContextRaw(rt);
  if (ctx == NULL) {
    NAPI_CALL(env, napi_throw_error(env, NULL, "failed to create context"));
    return NULL;
  }
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

#define ZERO_PARAM_METHOD_NAME(n) qjs_context_##n

#define DEF_CONTEXT_ZERO_PARAM_METHOD(NAME) \
  static napi_value ZERO_PARAM_METHOD_NAME(NAME) \
  (napi_env env, napi_callback_info info) { \
    JSContext* ctx = NULL; \
    NAPI_GET_CB_INFO_THIS(env, info) \
    NAPI_UNWRAP(env, this_arg, &ctx, "Invalid Context"); \
    JS_##NAME(ctx); \
    return NULL; \
  }

DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicBaseObjects)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicDate)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicEval)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicStringNormalize)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicRegExp)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicJSON)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicProxy)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicMapSet)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicTypedArrays)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicPromise)
DEF_CONTEXT_ZERO_PARAM_METHOD(AddIntrinsicBigInt)

static napi_value qjs_context_data(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_UNWRAP(env, this_arg, &ctx, "Invalid Context");

  napi_value ret;
  NAPI_CREATE_POINTER_VALUE(env, ctx, &ret);
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
    napi_value err = qjs_to_napi_value(env, ctx, error);
    JS_FreeValue(ctx, error);
    NAPI_CALL(env, napi_throw(env, err));
    return NULL;
  }

  napi_value ret = qjs_to_napi_value(env, ctx, value);
  JS_FreeValue(ctx, value);
  return ret;
}

static napi_value qjs_context_expose(napi_env env, napi_callback_info info) {
  JSContext* ctx = NULL;
  NAPI_GET_CB_INFO(env, info, 2, "Invalid parameters")
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_string, "typeof arguments[0] !== 'string'");
  NAPI_UNWRAP(env, this_arg, &ctx, "Invalid Context");

  JSValue value = qjs_from_napi_value(env, ctx, argv[1]);
  bool pending;
  napi_is_exception_pending(env, &pending);
  if (pending) {
    return NULL;
  }

  size_t len = 0;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &len));
  char* buf = (char*)malloc(len + 1);
  napi_status r = napi_get_value_string_utf8(env, argv[0], buf, len + 1, &len);
  if (r != napi_ok) {
    free(buf);
    NAPI_CALL(env, r);
  }
  *(buf + len) = '\0';

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, buf, value);
  free(buf);
  JS_FreeValue(ctx, global);

  return NULL;
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
    },
    {
      "expose", NULL,
      qjs_context_expose, NULL, NULL, NULL,
      NAPI_INSTANCE_METHOD_ATTR, NULL
    },
    { "addIntrinsicBaseObjects", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicBaseObjects), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicDate", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicDate), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicEval", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicEval), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicStringNormalize", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicStringNormalize), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicRegExp", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicRegExp), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicJSON", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicJSON), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicProxy", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicProxy), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicMapSet", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicMapSet), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicTypedArrays", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicTypedArrays), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicPromise", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicPromise), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "addIntrinsicBigInt", NULL, ZERO_PARAM_METHOD_NAME(AddIntrinsicBigInt), NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL }
  };

  size_t property_size = sizeof(properties) / sizeof(properties[0]);
  napi_value ctor;
  NAPI_CALL_VOID(env, napi_define_class(env, "Context", NAPI_AUTO_LENGTH,
                                        qjs_context_constructor, NULL,
                                        property_size, properties, &ctor));
  // NAPI_CALL_VOID(env, napi_create_reference(env, ctor, 1, &constructor_));
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "Context", ctor));
}
