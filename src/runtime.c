#include <quickjs-libc.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "helper_macro.h"

static napi_value qjs_runtime_constructor(napi_env env, napi_callback_info info) {
  JSRuntime* rt;
  NAPI_GET_CB_INFO_THIS(env, info)
  rt = JS_NewRuntime();

  qjs_runtime_wrap_s* wrap = (qjs_runtime_wrap_s*)malloc(sizeof(qjs_runtime_wrap_s));
  wrap->env = env;
  wrap->value = rt;
  wrap->interrupt_handler = NULL;
  if (napi_ok != napi_create_reference(env, this_arg, 1, &wrap->this_arg)) {
    free(wrap);
    napi_throw_error(env, NULL, "Failed to create this reference");
    return NULL;
  }
  NAPI_CALL(env, napi_wrap(env, this_arg, wrap, NULL, NULL, NULL));
  return this_arg;
}

static napi_value qjs_runtime_data(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;

  napi_value ret;
  NAPI_CREATE_POINTER_VALUE(env, rt, &ret);
  return ret;
}

static napi_value qjs_runtime_dispose(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info)
  NAPI_REMOVE_WRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  JS_FreeRuntime(rt);
  if (wrap->interrupt_handler != NULL) {
    NAPI_CALL(env, napi_delete_reference(env, wrap->interrupt_handler));
  }
  NAPI_CALL(env, napi_delete_reference(env, wrap->this_arg));
  free(wrap);

  return NULL;
}

static napi_value qjs_runtime_set_memory_limit(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "limit requied");
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_number, "limit is not number");
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  uint32_t limit;
  NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &limit));

  JS_SetMemoryLimit(rt, limit);
  return NULL;
}

static napi_value qjs_runtime_set_gc_threshold(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "threshold requied");
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_number, "threshold is not number");
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  uint32_t threshold;
  NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &threshold));

  JS_SetGCThreshold(rt, threshold);
  return NULL;
}

static napi_value qjs_runtime_set_max_stack_size(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "max_stack_size requied");
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_number, "max_stack_size is not number");
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  uint32_t max_stack_size;
  NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &max_stack_size));

  JS_SetMaxStackSize(rt, max_stack_size);
  return NULL;
}

static napi_value qjs_runtime_run_gc(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info);
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;

  JS_RunGC(rt);
  return NULL;
}

static napi_value qjs_runtime_compute_memory_usage(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info);
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  JSMemoryUsage s;
  memset(&s, 0, sizeof(JSMemoryUsage));
  JS_ComputeMemoryUsage(rt, &s);
  napi_value ret, value;
  NAPI_CALL(env, napi_create_object(env, &ret));
  NAPI_CALL(env, napi_create_int64(env, s.malloc_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "mallocSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.malloc_limit, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "mallocLimit", value));
  NAPI_CALL(env, napi_create_int64(env, s.memory_used_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "memoryUsedSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.malloc_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "mallocCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.memory_used_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "memoryUsedCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.atom_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "atomCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.atom_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "atomSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.str_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "strCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.str_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "strSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.obj_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "objCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.obj_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "objSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.prop_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "propCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.prop_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "propSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.shape_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "shapeCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.shape_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "shapeSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.js_func_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "jsFuncCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.js_func_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "jsFuncSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.js_func_code_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "jsFuncCodeSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.js_func_pc2line_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "jsFuncPc2lineCound", value));
  NAPI_CALL(env, napi_create_int64(env, s.js_func_pc2line_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "jsFuncPc2lineSize", value));
  NAPI_CALL(env, napi_create_int64(env, s.c_func_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "cFuncCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.array_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "arrayCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.fast_array_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "fastArrayCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.fast_array_elements, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "fastArrayElements", value));
  NAPI_CALL(env, napi_create_int64(env, s.binary_object_count, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "binaryObjectCount", value));
  NAPI_CALL(env, napi_create_int64(env, s.binary_object_size, &value));
  NAPI_CALL(env, napi_set_named_property(env, ret, "binaryObjectSize", value));
  return ret;
}

static int qjs_interrupt_handler(JSRuntime *rt, void *opaque) {
  qjs_runtime_wrap_s* wrap = (qjs_runtime_wrap_s*) opaque;
  napi_value f, und, ret, runtime, tobool;
  bool b;
  napi_status r;
  napi_env env = wrap->env;
  napi_ref interrupt_handler = wrap->interrupt_handler;
  r = napi_get_reference_value(env, interrupt_handler, &f);
  if (napi_ok != r) return 1;
  r = napi_get_reference_value(env, wrap->this_arg, &runtime);
  if (napi_ok != r) return 1;
  r = napi_get_undefined(env, &und);
  if (napi_ok != r) return 1;
  r = napi_call_function(env, und, f, 1, &runtime, &ret);
  if (napi_ok != r) return 1;
  r = napi_coerce_to_bool(env, ret, &tobool);
  if (napi_ok != r) return 1;
  r = napi_get_value_bool(env, tobool, &b);
  if (napi_ok != r) return 1;
  return b ? 1 : 0;
}

static napi_value qjs_runtime_set_interrupt_handler(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "interrupt handler requied");
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_function, "interrupt handler is not function");
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;

  if (wrap->interrupt_handler != NULL) {
    NAPI_CALL(env, napi_delete_reference(env, wrap->interrupt_handler));
  }
  NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &wrap->interrupt_handler));

  JS_SetInterruptHandler(rt, qjs_interrupt_handler, wrap);
  return NULL;
}

static napi_value qjs_runtime_set_can_block(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO(env, info, 1, "boolean requied");
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_boolean, "not boolean");
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  bool b;
  NAPI_CALL(env, napi_get_value_bool(env, argv[0], &b));
  JS_SetCanBlock(rt, b);
  return NULL;
}

static napi_value qjs_runtime_is_job_pending(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info);
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  bool b;
  int r = JS_IsJobPending(rt);
  napi_value ret;
  NAPI_CALL(env, napi_get_boolean(env, (bool)r, &ret));
  return ret;
}

static napi_value qjs_runtime_execute_pending_job(napi_env env, napi_callback_info info) {
  qjs_runtime_wrap_s* wrap;
  JSRuntime* rt = NULL;
  NAPI_GET_CB_INFO_THIS(env, info);
  NAPI_UNWRAP(env, this_arg, &wrap, "Invalid Runtime");
  rt = wrap->value;
  bool b;
  JSContext* ctx;
  int r = JS_ExecutePendingJob(rt, &ctx);
  napi_value ret;
  NAPI_CALL(env, napi_create_int32(env, r, &ret));
  return ret;
}

void register_qjs_runtime_class(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    { "dispose", NULL, qjs_runtime_dispose, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "data", NULL, qjs_runtime_data, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "setMemoryLimit", NULL, qjs_runtime_set_memory_limit, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "setGCThreshold", NULL, qjs_runtime_set_gc_threshold, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "setMaxStackSize", NULL, qjs_runtime_set_max_stack_size, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "runGC", NULL, qjs_runtime_run_gc, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "computeMemoryUsage", NULL, qjs_runtime_compute_memory_usage, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "setInterruptHandler", NULL, qjs_runtime_set_interrupt_handler, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "setCanBlock", NULL, qjs_runtime_set_can_block, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "isJobPending", NULL, qjs_runtime_is_job_pending, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
    { "executePendingJob", NULL, qjs_runtime_execute_pending_job, NULL, NULL, NULL, NAPI_INSTANCE_METHOD_ATTR, NULL },
  };

  size_t property_size = sizeof(properties) / sizeof(properties[0]);
  napi_value ctor;
  NAPI_CALL_VOID(env, napi_define_class(env, "Runtime", NAPI_AUTO_LENGTH,
                                        qjs_runtime_constructor, NULL,
                                        property_size, properties, &ctor));
  // NAPI_CALL_VOID(env, napi_create_reference(env, ctor, 1, &constructor_));
  NAPI_CALL_VOID(env, napi_set_named_property(env, exports, "Runtime", ctor));
}
