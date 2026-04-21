add_library(Rocket SHARED CMakeLists.txt)

set(IMGUI_DIR vendor/imgui)
set(IMGUIZMO_DIR vendor/imguizmo)

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set_source_files_properties(${IMGUI_SOURCES} "${IMGUIZMO_DIR}/ImGuizmo.cpp"
  PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

file(GLOB_RECURSE ROCKET_HEADERS "include/*.h")
file(GLOB_RECURSE ROCKET_SOURCES "src/Rocket/*.cpp")
file(GLOB_RECURSE ROCKET_MODULES "src/Rocket/*.ixx")

file(GLOB_RECURSE ROCKET_BACKENDS_SOURCES "src/Backends/*.cpp")
file(GLOB_RECURSE ROCKET_BACKENDS_MODULES "src/Backends/*.ixx")

set_source_files_properties(${ROCKET_MODULES} ${ROCKET_BACKENDS_MODULES}
  PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

target_sources(Rocket
  PUBLIC
    FILE_SET rocket_headers
    TYPE HEADERS
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:../include/Rocket>
    FILES ${ROCKET_HEADERS}

    FILE_SET imgui_headers
    TYPE HEADERS
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}>
              $<INSTALL_INTERFACE:../include/imgui>
    FILES
      "${IMGUI_DIR}/imconfig.h"
      "${IMGUI_DIR}/imgui.h"
      "${IMGUI_DIR}/imgui_internal.h"
      "${IMGUI_DIR}/imstb_rectpack.h"
      "${IMGUI_DIR}/imstb_textedit.h"
      "${IMGUI_DIR}/imstb_truetype.h"

    FILE_SET imguizmo_headers
    TYPE HEADERS
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${IMGUIZMO_DIR}>
              $<INSTALL_INTERFACE:../include/imguizmo>
    FILES "${IMGUIZMO_DIR}/ImGuizmo.h"

    FILE_SET rocket_modules
    TYPE CXX_MODULES
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
              $<INSTALL_INTERFACE:../modules>
    FILES ${ROCKET_MODULES}
  PRIVATE
    FILE_SET backends_modules
    TYPE CXX_MODULES
    BASE_DIRS src/Backends
      FILES ${ROCKET_BACKENDS_MODULES}
  PRIVATE
    ${IMGUI_SOURCES}
    "${IMGUIZMO_DIR}/ImGuizmo.cpp"

    ${ROCKET_SOURCES}
    ${ROCKET_BACKENDS_SOURCES}
)

target_compile_definitions(Rocket 
  PRIVATE 
    RKE_BUILD_DLL 
    "IMGUI_IMPL_API="
    RKE_DEPENDENCY_GLFW
    IMGUI_DEFINE_MATH_OPERATORS
)

if(WIN32)
  file(GLOB_RECURSE ROCKET_OPENGL_SOURCES "src/Platform/OpenGL/*.cpp")
  file(GLOB_RECURSE ROCKET_OPENGL_MODULES "src/Platform/OpenGL/*.ixx")
  file(GLOB_RECURSE ROCKET_WINDOWS_SOURCES "src/Platform/Windows/*.cpp")

  set_source_files_properties(${ROCKET_OPENGL_MODULES}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

  target_sources(Rocket
    PRIVATE
      FILE_SET platform_modules
      TYPE CXX_MODULES
      BASE_DIRS src/Platform
      FILES ${ROCKET_OPENGL_MODULES}
    PRIVATE
      ${ROCKET_OPENGL_SOURCES}
      ${ROCKET_WINDOWS_SOURCES}
    )

  target_compile_definitions(Rocket
    PRIVATE "IMGUI_API=__declspec(dllexport)"
  )
endif()

target_include_directories(Rocket
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/Rocket>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}>
    $<INSTALL_INTERFACE:include/imgui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${IMGUIZMO_DIR}>
    $<INSTALL_INTERFACE:include/imguizmo>
)

target_link_libraries(Rocket
  PRIVATE
    glfw
    glad::glad
    box2d::box2d
    yaml-cpp::yaml-cpp
    unofficial::shaderc::shaderc
    spirv-cross-core
    spirv-cross-glsl
  PUBLIC
    glm::glm
    EnTT::EnTT
)

if(WIN32)
  target_link_libraries(Rocket PRIVATE opengl32)
endif()

target_precompile_headers(Rocket PRIVATE "include/rke_pch.h")

# installation
find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp)
if(GLM_INCLUDE_DIR)
  message(STATUS "Found GLM headers at: ${GLM_INCLUDE_DIR}")
  install(DIRECTORY "${GLM_INCLUDE_DIR}/glm" 
          DESTINATION "include")
endif()

find_path(ENTT_INCLUDE_DIR NAMES entt/entt.hpp)
if(ENTT_INCLUDE_DIR)
  message(STATUS "Found EnTT headers at: ${ENTT_INCLUDE_DIR}")
  install(DIRECTORY "${ENTT_INCLUDE_DIR}/entt" 
          DESTINATION "include")
endif()

install(TARGETS Rocket
  EXPORT RocketTargets
    RUNTIME DESTINATION "bin/${ROCKET_OUTPUT_NAME}"
    LIBRARY DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    ARCHIVE DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    
    FILE_SET rocket_modules   DESTINATION "modules"
    FILE_SET rocket_headers   DESTINATION "include/Rocket"
    FILE_SET imgui_headers    DESTINATION "include/imgui"
    FILE_SET imguizmo_headers DESTINATION "include/imguizmo"
)

# solution display
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES
    ${ROCKET_HEADERS}
    ${ROCKET_MODULES}
    ${ROCKET_SOURCES} 
    ${ROCKET_BACKENDS_SOURCES}
    ${ROCKET_BACKENDS_MODULES}
    if(WIN32)
      ${ROCKET_OPENGL_MODULES}
      ${ROCKET_OPENGL_SOURCES}
      ${ROCKET_WINDOWS_SOURCES}
    endif()

    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h"
    "${IMGUIZMO_DIR}/ImGuizmo.h"

    ${IMGUI_SOURCES}
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
)
