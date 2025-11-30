cmake_minimum_required(VERSION 3.28)
project(SFML_Box2D_Template LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Config-specific output directories (Visual Studio multi-config safe)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${cfg}" CFG_UP)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UP} "${CMAKE_BINARY_DIR}/bin/${cfg}")
  endforeach()
endif()

include(FetchContent)

# SFML 2.6.2
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 2.6.2
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

# Box2D v2.4.1
FetchContent_Declare(
  Box2D
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v2.4.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Box2D)

# Sources (reconfigure when files are added)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp" "src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})

# Link SFML 2.x targets and Box2D
target_link_libraries(${PROJECT_NAME}
  PUBLIC sfml-graphics
  PUBLIC box2d
)

# If needed later, also link sfml-window and sfml-system explicitly:
# target_link_libraries(${PROJECT_NAME} PUBLIC sfml-graphics sfml-window sfml-system)

# Copy Assets directory next to the executable (per configuration)
if(EXISTS ${CMAKE_SOURCE_DIR}/Assets)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets
  )
endif()

# Optional: install target + assets for "cmake --install"
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Assets DESTINATION bin)