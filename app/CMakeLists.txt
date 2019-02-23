project(TrophaxSE)

set(VITA_APP_NAME ${PROJECT_NAME})
set(VITA_TITLEID  "TROPHAXSE")

set(VITA_VERSION  "01.00")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1 -s CATEGORY=gdb")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS} -a 0x2808000000000000")

add_executable(${PROJECT_NAME}
  src/main.c
)

target_include_directories(${PROJECT_NAME}
  PRIVATE ../user/src ../kernel/src
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${CMAKE_BINARY_DIR}/user/user_stubs
)

target_link_libraries(${PROJECT_NAME}
	SceAppMgr_stub
	SceCtrl_stub
	SceDisplay_stub
	SceGxm_stub
	SceAppMgrUser_stub_weak
	SceNpTrophy_stub
	SceSysmodule_stub
	SceCommonDialog_stub
	taihen_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}

  FILE ${CMAKE_BINARY_DIR}/kernel/kernel.skprx module/kernel.skprx
  FILE ${CMAKE_BINARY_DIR}/user/user.suprx module/user.suprx

  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg0.png sce_sys/livearea/contents/bg0.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)