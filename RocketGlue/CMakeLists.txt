add_library(RocketGlue STATIC CMakeLists.txt)

file(GLOB_RECURSE GLUE_HEADERS "include/*.h")
file(GLOB_RECURSE GLUE_SOURCES "src/*.cpp")

target_sources(RocketGlue
  PUBLIC
    FILE_SET glue_headers
    TYPE HEADERS
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:include/RocketGlue>
      FILES ${GLUE_HEADERS}
  PRIVATE
    ${GLUE_SOURCES}
)

target_include_directories(RocketGlue PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/RocketGlue>
)

target_link_libraries(RocketGlue PUBLIC Rocket)

# installation
install(TARGETS RocketGlue
  EXPORT RocketTargets
    RUNTIME DESTINATION "bin/${ROCKET_OUTPUT_NAME}"
    LIBRARY DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    ARCHIVE DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    FILE_SET glue_headers DESTINATION "include/RocketGlue"
)

# solution display
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES 
    ${GLUE_HEADERS}
    ${GLUE_SOURCES}
)
