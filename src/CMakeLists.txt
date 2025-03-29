add_executable(${PROJECT_NAME} "kitsune_engine.cpp" "main.cpp" "kitsune_engine.hpp" "kitsune_types.h"  )

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")






set(LIBS)
list(APPEND LIBS fmt::fmt SDL3::SDL3 Vulkan-Headers imgui)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} ${LIBS})

add_dependencies(${PROJECT_NAME} shaders)