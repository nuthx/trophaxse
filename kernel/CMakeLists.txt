project(SceAppMgrKernel2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

add_executable(${PROJECT_NAME}
  src/main.c
)

target_link_libraries(${PROJECT_NAME}
  SceSysmemForDriver_stub
  SceModulemgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceDebugForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self(kernel.skprx ${PROJECT_NAME} CONFIG exports.yml UNSAFE)

vita_create_stubs(kernel_stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/exports.yml KERNEL)
