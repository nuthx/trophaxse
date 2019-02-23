project(SceAppMgrUser)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

add_executable(${PROJECT_NAME}
  src/main.c
)

target_include_directories(${PROJECT_NAME}
  PRIVATE ../kernel/src
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${CMAKE_BINARY_DIR}/kernel/kernel_stubs
)

target_link_libraries(${PROJECT_NAME}
  ${CMAKE_BINARY_DIR}/kernel/kernel_stubs/libSceAppMgrKernel2_stub.a
)

vita_create_self(user.suprx ${PROJECT_NAME} CONFIG exports.yml UNSAFE)

vita_create_stubs(user_stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/exports.yml)
