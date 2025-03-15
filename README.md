<div align="center">
	<img height="50" src="branding/logo.svg">
</div>

# QtAppInstanceManager

[![License: MIT](https://img.shields.io/badge/license-MIT-green)](https://mit-license.org/)
[![CMake version](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)](https://www.qt.io)
[![C++ version](https://img.shields.io/badge/C++-17-00599C?logo=++)](https://www.qt.io)
[![Qt version](https://img.shields.io/badge/Qt-6.6.0+-41CD52?logo=qt)](https://www.qt.io)

**QtAppInstanceManager** is a tool to control how many instances of your Qt application are running at the same time, and to send messages between instances. It uses a local socket under the hood. You may then build upon this foundation any messaging system or protocol, such as JSON-RPC for instance (NB: not provided because out of the scope of this library).

It is intended to be a replacement for `QtSingleApplication`, the deprecated Qt4 official project.

Also, it differs from [itay-grudev's SingleApplication](https://github.com/itay-grudev/SingleApplication) because you don't need to inherit a child class of `QCoreApplication` to use `QtAppInstanceManager`, therefore it doesn't need configure-time flags and macros.

---

### Table of Contents

- [Requirements](#requirements)
- [Usage](#usage)
- [Examples](#examples)
  - [Single application](#single-application)
  - [Multiple application instances](#multiple-application-instances)
- [Author](#author)
- [License](#license)

---

## Requirements

- Platform: Windows, MacOS, Linux.
- [CMake 3.21+](https://cmake.org/download/)
- [Qt 6.6+](https://www.qt.io/download-qt-installer)

## Usage

1. Add the library as a dependency in your CMakeLists.txt, for example with FetchContent.

   ```cmake
   include(FetchContent)
   FetchContent_Declare(QtAppInstanceManager
    GIT_REPOSITORY "https://github.com/oclero/qtappinstancemanager.git"
   )
   FetchContent_MakeAvailable(QtAppInstanceManager)
   ```

2. Link with the library in CMake.

   ```cmake
   target_link_libraries(your_project oclero::QtAppInstanceManager)
   ```

3. Include the only necessary header in your C++ file.

   ```c++
   #include <oclero/QtAppInstanceManager.hpp>
   ```

## Examples

### Single application

Just create an `oclero::QtAppInstanceManager` and configure it to allow only one instance running at the same time with `setForceSingleInstance(true)`.

Complete example can be found in the [`/examples` directory](examples/single), including CMake and C++ file.

```c++
// Initialize instance manager to force only one instance running.
oclero::QtAppInstanceManager instanceManager;
instanceManager.setMode(QtAppInstanceManager::Mode::SingleInstance);

// When another instance will start, it will immediately quit the app and send its
// arguments to the primary instance.
QObject::connect(&instanceManager,
  &oclero::QtAppInstanceManager::secondaryInstanceMessageReceived,
  &instanceManager,
  [](const unsigned int id, QByteArray const& data) {
    // Do what you want:
    // - Raise the main window.
    // - Open a file in an another tab of your main window.
    // - ...
    qDebug() << "Secondary instance message received: " << data;
  });

// If you don't want for the app to quit, you can set the manual mode and handle this step by yourself.
instanceManager.setAppExitMode(QtAppInstanceManager::AppExitMode::Manual);
QObject::connect(&instanceManager,
  &QtAppInstanceManager::appExitRequested,
  &singleInstance,
  []() {
    // Do what you want.
    // Usually you should quit the app.
    qDebug() << "This app should exit";
    QCoreApplication::quit();
    std::exit(EXIT_SUCCESS);
  });
```

### Multiple application instances

```c++
oclero::QtAppInstanceManager instanceManager;

// If we are the FIRST instance to launch,
// we'll receive messages from other instances that will launch after us.
QObject::connect(&instanceManager,
                 &oclero::QtAppInstanceManager::secondaryInstanceMessageReceived,
                 &instanceManager,
  [](const unsigned int id, QByteArray const& data) {
    qDebug() << "Message received from secondary instance: " << data;
  });

// If we are another instance, we are able to receive messages from the primary one.
QObject::connect(&instanceManager,
                 &oclero::QtAppInstanceManager::primaryInstanceMessageReceived,
                 &instanceManager,
  [](QByteArray const& data) {
    qDebug() << "Message received from primary instance: " << data;
  });

// If ever the first instance to launch unexpectly shuts down,
// one of the secondary instances will immediately take of the role of the primary one.
QObject::connect(&instanceManager,
                 &oclero::QtAppInstanceManager::instanceRoleChanged,
                 &instanceManager,
  [&instanceManager]() {
    // There is a short period of time before roles are assigned again.
    if (!instanceManager.isPrimaryInstance() && !instanceManager.isSecondaryInstance()) {
      qDebug() << "Waiting for new role...";
    } else {
      qDebug() << "New role: " << (instanceManager.isPrimaryInstance() ? "Primary" : "Secondary");
    }
  });
```

## Author

**Olivier Cléro** | [email](mailto:oclero@pm.me) | [website](https://www.olivierclero.com) | [github](https://www.github.com/oclero)

Thanks to [Emilien Vallot](https://github.com/envt) for his help and advices.

## License

**QtAppInstanceManager** is available under the MIT license. See the [LICENSE](LICENSE) file for more info.
