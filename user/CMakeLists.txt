project(SceAppMgrUser)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

add_executable(${PROJECT_NAME}
  src/main.c
)

add_dependencies(${PROJECT_NAME} kernel_stubs)

target_include_directories(${PROJECT_NAME}
  PRIVATE ../kernel/src
)

target_link_libraries(${PROJECT_NAME}
  -L${CMAKE_BINARY_DIR}/kernel/kernel_stubs
  SceAppMgrKernel2_stub
)

vita_create_self(user.suprx ${PROJECT_NAME} CONFIG exports.yml UNSAFE)

vita_create_stubs(user_stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/exports.yml)
