cmake_minimum_required(VERSION 3.31)

project(RocketEngine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${ROCKET_OUTPUT_NAME}")
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} CONFIG_UPPER)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_DIR}/lib")
  set(CMAKE_PDB_OUTPUT_DIRECTORY_${CONFIG_UPPER}     "${OUTPUT_DIR}")
endforeach()

set(RKE_SDK_DIR "${CMAKE_SOURCE_DIR}/RocketSDK")
set(CMAKE_INSTALL_PREFIX "${RKE_SDK_DIR}")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Third-Party
  # window support
  find_package(glfw3 CONFIG REQUIRED)

  # graphics api support
  find_package(glad CONFIG REQUIRED) # opengl
  find_package(unofficial-shaderc CONFIG REQUIRED) # glsl(opengl/vulkan)
  find_package(spirv_cross_core CONFIG REQUIRED)   # glsl(opengl/vulkan)
  find_package(spirv_cross_glsl CONFIG REQUIRED)   # glsl(opengl/vulkan)

  # 2D physics
  find_package(box2d CONFIG REQUIRED)
  # 3D math
  find_package(glm CONFIG REQUIRED)
  # entity-component-system
  find_package(EnTT CONFIG REQUIRED)

  # serialization
  find_package(yaml-cpp CONFIG REQUIRED)
# ---

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions (
    RKE_DEBUG
    RKE_ENABLE_ASSERTS
    RKE_ENABLE_STATISTICS
    RKE_ENABLE_PROFILE
    RKE_ENABLE_LOG
  )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions (
    RKE_RELEASE
    RKE_ENABLE_STATISTICS
    RKE_ENABLE_PROFILE
    RKE_ENABLE_LOG
  )
else()
  add_compile_definitions(RKE_SHIPPING)
endif()

if(WIN32)
  add_compile_definitions (
    RKE_PLATFORM_WINDOWS
    _CRT_SECURE_NO_WARNINGS
  )
endif()

add_subdirectory(Rocket)
add_subdirectory(RocketGlue)
add_subdirectory(RocketLauncher)

install(FILES "assets/RocketSDK.cmake" DESTINATION ".")
