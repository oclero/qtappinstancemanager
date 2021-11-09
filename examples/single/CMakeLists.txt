find_package(Qt5 REQUIRED Core)

add_executable(SingleInstanceExample)
target_sources(SingleInstanceExample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(SingleInstanceExample PRIVATE oclero::QtAppInstanceManager Qt5::Core)

set_target_properties(SingleInstanceExample PROPERTIES
  INTERNAL_CONSOLE ON
  EXCLUDE_FROM_ALL ON
  FOLDER examples
)

############# Minimal example ends here #############
target_deploy_qt(SingleInstanceExample)
