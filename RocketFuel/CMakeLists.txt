add_library(RocketFuel STATIC CMakeLists.txt)

file(GLOB_RECURSE FUEL_HEADERS "include/*.h")
file(GLOB_RECURSE FUEL_SOURCES "src/*.cpp")

target_sources(RocketFuel
  PUBLIC
    FILE_SET fuel_headers
    TYPE HEADERS
    BASE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:include/RocketFuel>
      FILES ${FUEL_HEADERS}
  PRIVATE
    ${FUEL_SOURCES}
)

target_include_directories(RocketFuel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/RocketFuel>
)

target_link_libraries(RocketFuel PUBLIC Rocket)

# installation
install(TARGETS RocketFuel
  EXPORT RocketTargets
    RUNTIME DESTINATION "bin/${ROCKET_OUTPUT_NAME}"
    LIBRARY DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    ARCHIVE DESTINATION "lib/${ROCKET_OUTPUT_NAME}"
    FILE_SET fuel_headers DESTINATION "include/RocketFuel"
)

# solution display
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES 
    ${FUEL_HEADERS}
    ${FUEL_SOURCES}
)
