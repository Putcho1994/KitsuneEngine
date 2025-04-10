add_executable(${PROJECT_NAME}   "kitsune_types.h"  "hello_triangle.cpp"  "kitsune_engine.cpp"  "kitsune_windowing.cpp" )

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

target_compile_definitions(${PROJECT_NAME} PUBLIC
  
    # Your options here, project-dependent:
    # https://github.com/KhronosGroup/Vulkan-Hpp#configuration-options

    VULKAN_HPP_NO_SPACESHIP_OPERATOR=ON
    VK_ENABLE_BETA_EXTENSIONS=ON
    
  )

set(LIBS)
list(APPEND LIBS fmt::fmt SDL3::SDL3 Vulkan-Headers glm::glm-header-only)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} ${LIBS})

add_dependencies(${PROJECT_NAME} shaders)