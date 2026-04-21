add_executable(RocketLauncher CMakeLists.txt)

file(GLOB_RECURSE LAUNCHER_SOURCES "src/*.cpp")
file(GLOB_RECURSE LAUNCHER_MODULES "src/*.ixx")

target_sources(RocketLauncher
  PRIVATE
    FILE_SET launcher_modules
    TYPE CXX_MODULES
    BASE_DIRS src
    FILES ${LAUNCHER_MODULES}
  PRIVATE
    ${LAUNCHER_SOURCES}
  
)

if(WIN32)
  target_sources(RocketLauncher
    PRIVATE "src/RocketLauncher.manifest"
  )
  target_compile_definitions(RocketLauncher
    PRIVATE "IMGUI_API=__declspec(dllimport)"
  )
endif()

target_precompile_headers(RocketLauncher PRIVATE "../Rocket/include/rke_pch.h")
target_link_libraries(RocketLauncher PRIVATE Rocket)
target_include_directories(RocketLauncher PRIVATE "../Rocket/include")

if(CMAKE_BUILD_TYPE STREQUAL "Shipping")
  set_target_properties(RocketLauncher PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# solution display
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES
    ${LAUNCHER_HEADERS}
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_MODULES}

    if(WIN32)
      "src/RocketLauncher.manifest"
    endif()
)
