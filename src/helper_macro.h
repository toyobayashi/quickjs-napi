#ifndef SRC_HELPER_MACRO_H_
#define SRC_HELPER_MACRO_H_

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

#endif
