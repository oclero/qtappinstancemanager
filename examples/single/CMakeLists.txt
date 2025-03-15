find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(SingleInstanceExample)
target_sources(SingleInstanceExample
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_link_libraries(SingleInstanceExample
  PRIVATE
    ${PROJECT_NAMESPACE}::${PROJECT_NAME}
    Qt::Core
)

set_target_properties(SingleInstanceExample PROPERTIES
  INTERNAL_CONSOLE ON
  EXCLUDE_FROM_ALL ON
  FOLDER examples
)
