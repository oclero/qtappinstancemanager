set(EXAMPLE_TARGET_NAME ${PROJECT_NAME}Example)

find_package(Qt5
  REQUIRED
    Core
)

add_executable(${EXAMPLE_TARGET_NAME})
set_target_properties(${EXAMPLE_TARGET_NAME}
  PROPERTIES
    INTERNAL_CONSOLE ON
    EXCLUDE_FROM_ALL ON
    FOLDER examples
)
set(EXAMPLE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
target_sources(${EXAMPLE_TARGET_NAME}
  PRIVATE
    ${EXAMPLE_SOURCES}
)
target_link_libraries(${EXAMPLE_TARGET_NAME}
  PRIVATE
    ${PROJECT_NAMESPACE}::${PROJECT_NAME}
    Qt5::Core
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLE_SOURCES})

############# Minimal example ends here #############

# Create an imported target based on Qt's deploying tool.
if(WIN32)
  set(DEPLOYQT_NAME "windeployqt")
elseif(APPLE)
  set(DEPLOYQT_NAME "macdeployqt")
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

if (DEPLOYQT_NAME)
  get_target_property(QMAKE_LOCATION Qt5::qmake IMPORTED_LOCATION)
  get_filename_component(QT_BINARY_DIR ${QMAKE_LOCATION} DIRECTORY)
  find_program(DEPLOYQT_EXE "${DEPLOYQT_NAME}" HINTS "${QT_BINARY_DIR}" REQUIRED)
  set(QTDEPLOY_TARGET_NAME Qt5::deploy)
  add_executable(${QTDEPLOY_TARGET_NAME} IMPORTED)
  set_property(TARGET ${QTDEPLOY_TARGET_NAME} PROPERTY IMPORTED_LOCATION ${DEPLOYQT_EXE})

  # Deploy Qt dependencies to help launch individual instances.
  if(TARGET ${QTDEPLOY_TARGET_NAME})
    add_custom_command(TARGET ${EXAMPLE_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt..."
      COMMAND ${QTDEPLOY_TARGET_NAME} --verbose 0 --no-patchqt --no-compiler-runtime --no-webkit2 --no-system-d3d-compiler --no-translations --no-angle --no-opengl-sw --dir "$<TARGET_FILE_DIR:${EXAMPLE_TARGET_NAME}>" "$<TARGET_FILE:${EXAMPLE_TARGET_NAME}>"
    )
  endif()
endif()
