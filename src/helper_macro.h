#ifndef SRC_HELPER_MACRO_H_
#define SRC_HELPER_MACRO_H_

#include <stdint.h>
#include <js_native_api.h>

#if UINTPTR_MAX == 0xFFFFFFFF
#define NAPI_CREATE_POINTER_VALUE(env, value, result) NAPI_CALL((env), napi_create_int32((env), (int32_t)(value), (result)))
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define NAPI_CREATE_POINTER_VALUE(env, value, result) NAPI_CALL((env), napi_create_bigint_int64((env), (int64_t)(value), (result)))
#else
#error "Not supported platform"
#endif

#define NAPI_CALL_BASE(env, the_call, ...)                      \
  do {                                                          \
    if ((the_call) != napi_ok) {                                \
      const napi_extended_error_info *error_info;               \
      napi_get_last_error_info((env), &error_info);             \
      bool is_pending;                                          \
      const char* err_message = error_info->error_message;      \
      napi_is_exception_pending((env), &is_pending);            \
      if (!is_pending) {                                        \
        const char* error_message = err_message != NULL ?       \
          err_message :                                         \
          "empty error message";                                \
        napi_throw_error((env), NULL, error_message);           \
      }                                                         \
      return __VA_ARGS__;                                       \
    }                                                           \
  } while (0)

#define NAPI_CALL(env, the_call)                                \
  NAPI_CALL_BASE(env, the_call, NULL)

#define NAPI_CALL_VOID(env, the_call)                           \
  NAPI_CALL_BASE(env, the_call)

#define NAPI_INSTANCE_METHOD_ATTR (napi_writable | napi_configurable)
#define NAPI_STATIC_METHOD_ATTR (napi_writable | napi_configurable | napi_static)

#define NAPI_CHECK_VALUE_TYPE(env, value, type, msg) \
  do { \
    napi_valuetype t; \
    NAPI_CALL((env), napi_typeof((env), (value), &t)); \
    if (t != (type)) { \
      NAPI_CALL((env), napi_throw_error((env), NULL, (msg))); \
      return NULL; \
    } \
  } while (0)

#define NAPI_UNWRAP(env, obj, data, msg) \
  do { \
    if (napi_ok != napi_unwrap((env), (obj), (void**)(data))) { \
      NAPI_CALL(env, napi_throw_error(env, NULL, (msg))); \
      return NULL; \
    } \
  } while (0)

#define NAPI_REMOVE_WRAP(env, obj, data, msg) \
  do { \
    if (napi_ok != napi_remove_wrap((env), (obj), (void**)(data))) { \
      NAPI_CALL(env, napi_throw_error(env, NULL, (msg))); \
      return NULL; \
    } \
  } while (0)

#define NAPI_GET_CB_INFO(env, info, argc_in, argc_msg) \
  size_t argc = (argc_in); \
  napi_value argv[(argc_in)]; \
  napi_value this_arg; \
  NAPI_CALL(env, napi_get_cb_info((env), (info), &argc, argv, &this_arg, NULL)); \
  if (argc < (argc_in)) { \
    NAPI_CALL(env, napi_throw_error(env, NULL, (argc_msg))); \
    return NULL; \
  }

#define NAPI_GET_CB_INFO_ARGS(env, info, argc_in, argc_msg) \
  size_t argc = (argc_in); \
  napi_value argv[(argc_in)]; \
  NAPI_CALL(env, napi_get_cb_info((env), (info), &argc, argv, NULL, NULL)); \
  if (argc < (argc_in)) { \
    NAPI_CALL(env, napi_throw_error(env, NULL, (argc_msg))); \
    return NULL; \
  }

#define NAPI_GET_CB_INFO_THIS(env, info) \
  napi_value this_arg; \
  NAPI_CALL(env, napi_get_cb_info((env), (info), NULL, NULL, &this_arg, NULL));

#define NAPI_SET_FUNCTION_VOID(env, exports, name, func) \
  do { \
    napi_value js_##func; \
    NAPI_CALL_VOID(env, napi_create_function(env, (name), NAPI_AUTO_LENGTH, (func), NULL, &(js_##func))); \
    NAPI_CALL_VOID(env, napi_set_named_property(env, (exports), (name), (js_##func))); \
  } while (0)

#endif
