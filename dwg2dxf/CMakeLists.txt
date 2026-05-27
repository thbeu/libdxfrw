add_executable(dwg2dxf
    main.cpp
    dx_iface.cpp
)

target_link_libraries(dwg2dxf PRIVATE dxfrw)

target_include_directories(dwg2dxf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

install(TARGETS dwg2dxf RUNTIME DESTINATION bin)
