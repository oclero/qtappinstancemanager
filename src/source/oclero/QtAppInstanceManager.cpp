#include <oclero/QtAppInstanceManager.hpp>

#include <QCoreApplication>
#include <QTimer>

#include "LocalEndpoint.hpp"

namespace oclero {
struct QtAppInstanceManager::Impl {
  QtAppInstanceManager& owner;
  LocalEndpoint endpoint;
  Mode mode{ Mode::MultipleInstances };
  AppExitMode appExitMode{ AppExitMode::Auto };

  Impl(QtAppInstanceManager& o)
    : owner(o) {
    QObject::connect(&endpoint, &LocalEndpoint::serverMessageReceived, &owner, [this](const QByteArray& data) {
      emit owner.primaryInstanceMessageReceived(data);
    });
    QObject::connect(
      &endpoint, &LocalEndpoint::clientMessageReceived, &owner, [this](const unsigned int id, const QByteArray& data) {
        emit owner.secondaryInstanceMessageReceived(id, data);
      });
    QObject::connect(&endpoint, &LocalEndpoint::roleChanged, &owner, [this]() {
      emit owner.instanceRoleChanged();
      quitIfRequired();
    });
  }

  void quitIfRequired() {
    // Force quit when only a single instance is allowed.
    if (mode == Mode::SingleInstance && endpoint.role() == LocalEndpoint::Role::Client) {
      // Send last message before quitting.
      auto args = QCoreApplication::arguments();
      args.removeFirst();
      const auto data = args.join(' ').toUtf8();
      endpoint.sendToServer(data);

      // Quit.
      emit owner.appExitRequested();
      if (appExitMode == AppExitMode::Auto) {
        QCoreApplication::quit();
        std::exit(EXIT_SUCCESS);
      }
    }
  }
};

QtAppInstanceManager::QtAppInstanceManager(QObject* parent)
  : QtAppInstanceManager(Mode::MultipleInstances, parent) {}

QtAppInstanceManager::QtAppInstanceManager(Mode mode, QObject* parent)
  : QtAppInstanceManager(Mode::MultipleInstances, AppExitMode::Auto, parent) {}

QtAppInstanceManager::QtAppInstanceManager(Mode mode, AppExitMode appExitMode, QObject* parent)
  : QObject(parent)
  , _impl(new Impl(*this)) {
  _impl->mode = mode;
  _impl->appExitMode = appExitMode;
  QTimer::singleShot(0, [this]() {
    _impl->quitIfRequired();
  });
}

QtAppInstanceManager::~QtAppInstanceManager() = default;

bool QtAppInstanceManager::isPrimaryInstance() const {
  return _impl->endpoint.role() == LocalEndpoint::Role::Server;
}

bool QtAppInstanceManager::isSecondaryInstance() const {
  return _impl->endpoint.role() == LocalEndpoint::Role::Client;
}

int QtAppInstanceManager::secondaryInstanceCount() const {
  return _impl->endpoint.secondaryInstanceCount();
}

void QtAppInstanceManager::sendMessageToPrimary(const QByteArray& data) {
  if (isSecondaryInstance()) {
    _impl->endpoint.sendToServer(data);
  }
}

void QtAppInstanceManager::sendMessageToSecondary(const unsigned int id, const QByteArray& data) {
  if (isPrimaryInstance()) {
    _impl->endpoint.sendToClient(id, data);
  }
}

QtAppInstanceManager::Mode QtAppInstanceManager::mode() const {
  return _impl->mode;
}

void QtAppInstanceManager::setMode(Mode mode) {
  if (mode != _impl->mode) {
    _impl->mode = mode;
    emit modeChanged();

    _impl->quitIfRequired();
  }
}

QtAppInstanceManager::AppExitMode QtAppInstanceManager::appExitMode() const {
  return _impl->appExitMode;
}

void QtAppInstanceManager::setAppExitMode(AppExitMode appExitMode) {
  if (appExitMode != _impl->appExitMode) {
    _impl->appExitMode = appExitMode;
    emit appExitModeChanged();
  }
}
} // namespace oclero
