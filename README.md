<div align="center">
	<img height="50" src="logo.svg">
</div>

# QtAppInstanceManager

[![License: MIT](https://img.shields.io/badge/license-MIT-green)](https://mit-license.org/)
[![CMake version](https://img.shields.io/badge/CMake-3.19+-064F8C?logo=cmake)](https://www.qt.io)
[![C++ version](https://img.shields.io/badge/C++-17-00599C?logo=++)](https://www.qt.io)
[![Qt version](https://img.shields.io/badge/Qt-5.15.2+-41CD52?logo=qt)](https://www.qt.io)

**QtAppInstanceManager** is tool to control how many instances of your Qt5 application are running at the same time, and to send messages between instances. It uses a local socket under the hood. You may then build upon this foundation any messaging system or protocol, such as JSON-RPC for instance (NB: not provided because out of the scope of this library).

It is intended to be a replacement for `QtSingleApplication`, the deprecated Qt4 official project.

Also, it differs from [itay-grudev's SingleApplication](https://github.com/itay-grudev/SingleApplication) because you don't need to inherit a child class of `QCoreApplication` to use `QtAppInstanceManager`, therefore it doesn't need configure-time flags and macros.

## Requirements

- Platform: Windows, MacOS, Linux.
- CMake 3.19+
- Qt 5.15+

## Usage

1. Add the library's repository as a Git submodule.

   ```bash
   git submodule add git@github.com:oclero/QtAppInstanceManager.git submodules/qtappinstancemanager
   ```

2. Download submodules.

   ```bash
   git submodule update --init --recursive
   ```

3. Add the library to your CMake project.

   ```cmake
   add_subdirectory(submodules/qtappinstancemanager)
   ```

4. Link with the library in CMake.

   ```cmake
   target_link_libraries(your_project oclero::QtAppInstanceManager)
   ```

5. Include the only necessary header in your C++ file.

   ```c++
   #include <oclero/QtAppInstanceManager.hpp>
   ```

## Examples

### Single application instance

Just create an `oclero::QtAppInstanceManager` and configure it to allow only one instance running at the same time with `setForceSingleInstance(true)`.

Complete example can be found in the [`/examples` directory](examples/single), including CMake and C++ file.

```c++
// Initialize instance manager to force only one instance running.
oclero::QtAppInstanceManager instanceManager;
instanceManager.setForceSingleInstance(true);

// When another instance will start, it will immediately quit and send its
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
