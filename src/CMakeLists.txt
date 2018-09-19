add_executable(${PROJECT_NAME} setultrastik360.cpp u360maps.h)
target_link_libraries(${PROJECT_NAME} usb-1.0)
install(TARGETS setultrastik360 DESTINATION bin)