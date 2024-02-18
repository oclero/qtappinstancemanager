# Changelog

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
