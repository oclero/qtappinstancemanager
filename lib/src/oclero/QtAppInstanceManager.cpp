#include <oclero/QtAppInstanceManager.hpp>

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "LocalEndpoint.hpp"

namespace oclero {
struct QtAppInstanceManager::Impl {
  QtAppInstanceManager& owner;
  LocalEndpoint endpoint;
  bool forceSingleInstance{ false };

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
    if (forceSingleInstance && endpoint.role() == LocalEndpoint::Role::Client) {
      // Send last message before quitting.
      auto args = QCoreApplication::arguments();
      args.removeFirst();
      const auto data = args.join(' ').toUtf8();
      endpoint.sendToServer(data);

      // Quit.
      QCoreApplication::quit();
      std::exit(EXIT_SUCCESS);
    }
  }
};

QtAppInstanceManager::QtAppInstanceManager(QObject* parent)
  : QObject(parent)
  , _impl(new Impl(*this)) {
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

bool QtAppInstanceManager::forceSingleInstance() const {
  return _impl->forceSingleInstance;
}

void QtAppInstanceManager::setForceSingleInstance(bool force) {
  if (force != _impl->forceSingleInstance) {
    _impl->forceSingleInstance = force;
    emit forceSingleInstanceChanged();

    _impl->quitIfRequired();
  }
}
} // namespace oclero
