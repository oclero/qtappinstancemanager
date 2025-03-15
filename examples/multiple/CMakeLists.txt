find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(MultipleInstanceExample)
target_sources(MultipleInstanceExample
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_link_libraries(MultipleInstanceExample
  PRIVATE
    ${PROJECT_NAMESPACE}::${PROJECT_NAME}
    Qt::Core
)

set_target_properties(MultipleInstanceExample PROPERTIES
  INTERNAL_CONSOLE ON
  EXCLUDE_FROM_ALL ON
  FOLDER examples
)
