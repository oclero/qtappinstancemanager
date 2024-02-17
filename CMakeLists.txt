cmake_minimum_required(VERSION 3.21.0)
enable_testing()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DeployQt)

# Set project information.
project("QtAppInstanceManager"
  LANGUAGES CXX
  VERSION 1.1.0.0
)
set(PROJECT_NAMESPACE "oclero")

# Global flags.
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The library.
add_subdirectory(src)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  # Tests.
  add_subdirectory(tests)

  # Examples.
  add_subdirectory(examples/single)
  add_subdirectory(examples/multiple)
endif()
