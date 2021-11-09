find_package(Qt5 REQUIRED Core)

add_executable(MultipleInstanceExample)
target_sources(MultipleInstanceExample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(MultipleInstanceExample PRIVATE oclero::QtAppInstanceManager Qt5::Core)

set_target_properties(MultipleInstanceExample PROPERTIES
  INTERNAL_CONSOLE ON
  EXCLUDE_FROM_ALL ON
  FOLDER examples
)

############# Minimal example ends here #############
target_deploy_qt(MultipleInstanceExample)
