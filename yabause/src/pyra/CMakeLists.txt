project(yabause-pyra)

include_directories(${PORT_INCLUDE_DIRS})
add_definitions(${PORT_CFLAGS})

set(yabause_pyra_SOURCES
    main.c)

set(yabause_main_HEADERS
    main.h)

#link_directories(..)
add_executable(yabause-pyra ${yabause_pyra_SOURCES})
if (YAB_WANT_OPENGL)
	set_target_properties(yabause-pyra PROPERTIES OUTPUT_NAME yabause-gl)
else()
	set_target_properties(yabause-pyra PROPERTIES OUTPUT_NAME yabause)
endif()
target_link_libraries(yabause-pyra ${YABAUSE_LIBRARIES})
target_link_libraries(yabause-pyra ${PORT_LIBRARIES})
target_link_libraries(yabause-pyra yabause)
target_link_libraries(yabause-pyra m)
