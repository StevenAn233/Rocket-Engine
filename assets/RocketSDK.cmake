set(ROCKET_INCLUDE_DIRS
  "${CMAKE_CURRENT_LIST_DIR}/include"
  "${CMAKE_CURRENT_LIST_DIR}/include/Rocket"
  "${CMAKE_CURRENT_LIST_DIR}/include/imgui"
  "${CMAKE_CURRENT_LIST_DIR}/include/imguizmo"
  "${CMAKE_CURRENT_LIST_DIR}/include/RocketGlue"
)

set(ROCKET_LIBS
  "${CMAKE_CURRENT_LIST_DIR}/lib/${ROCKET_OUTPUT_NAME}/Rocket.lib"
  "${CMAKE_CURRENT_LIST_DIR}/lib/${ROCKET_OUTPUT_NAME}/RocketGlue.lib"
)

set(ROCKET_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/modules)
file(GLOB_RECURSE ROCKET_MODULE_FILES "${CMAKE_CURRENT_LIST_DIR}/modules/*.ixx")
set(ROCKET_PCH "${CMAKE_CURRENT_LIST_DIR}/include/Rocket/rke_pch.h")

function(add_rocket_engine_to TARGET_NAME)
  target_compile_definitions(${TARGET_NAME}
    PRIVATE 
      "IMGUI_IMPL_API="
      RKE_SCRIPT_BUILD_DLL
      IMGUI_DEFINE_MATH_OPERATORS
  )

  if(WIN32)
    target_compile_definitions(${TARGET_NAME}
      PRIVATE
        "IMGUI_API=__declspec(dllimport)"
        RKE_PLATFORM_WINDOWS
        _CRT_SECURE_NO_WARNINGS
    )
  endif()

  target_include_directories(${TARGET_NAME} PRIVATE ${ROCKET_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} PRIVATE ${ROCKET_LIBS})

  target_sources(${TARGET_NAME} PRIVATE
    FILE_SET rocket_sdk_modules
    TYPE CXX_MODULES
    BASE_DIRS "${ROCKET_MODULE_DIR}"
    FILES "${ROCKET_MODULE_FILES}"
  )
  set_source_files_properties(${ROCKET_MODULE_FILES}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

  target_precompile_headers(${TARGET_NAME} PRIVATE ${ROCKET_PCH})
endfunction()
