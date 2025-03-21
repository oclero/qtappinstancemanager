# Changelog

## v1.3.0

- Switch to Qt6.
- Switch to Ninja for macOS builds.
- Add `QTAPPINSTANCEMANAGER_TESTS` and `QTAPPINSTANCEMANAGER_EXAMPLES` options to enable tests and examples.

## v1.2.1

- Recommend FetchContent instead of submodule.

## v1.2.0

- Rename `lib` folder into `src`, and `src` subfolder into `source`.
- Add CI jobs to build and test on Windows, Linux and MacOS.
- Rename `setForceSingleInstance(bool)` into `setMode(SingleInstance|MultipleInstances)`.
- Add new test for single instance mode.
- Replace Bash scripts by CMake presets.
- Fix warnings.

## v1.1.0

- Linux compatibility fixes.

## v1.0.1

- Deploy Qt for examples and unit tests.

## v1.0.0

- Allow for communication between application instances.
- Allow for only one application instance to run at the same time.
