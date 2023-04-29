cmake_minimum_required(VERSION 3.13)

project(napi_quickjs)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/node_modules/emnapi")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs")

set(QJS_BINDING_SRC
  "src/binding.c"
  "src/context.c"
  "src/runtime.c"
  "src/libc.c"
)

add_executable(qjs ${QJS_BINDING_SRC})

target_include_directories(qjs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs")
target_link_libraries(qjs emnapi quickjs)
target_link_options(qjs PRIVATE
  "-sEXPORTED_FUNCTIONS=['_napi_register_wasm_v1','_malloc','_free']"
  "-sALLOW_MEMORY_GROWTH"
  "-sMODULARIZE=1"
  "-sEXPORT_NAME=quickjs"
  "-sENVIRONMENT=web"
  "-sSAFE_HEAP=1"
  "-sSTACK_SIZE=5MB"
  "--minify=0"
  "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/src/pre.js"
  "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/src/post.js"
)
