#include <oclero/QtAppInstanceManager.hpp>

#include "LocalEndpoint.hpp"

namespace oclero {
struct QtAppInstanceManager::Impl {
  QtAppInstanceManager& owner;
  LocalEndpoint endpoint;

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
    });
  }
};

QtAppInstanceManager::QtAppInstanceManager(QObject* parent)
  : QObject(parent)
  , _impl(new Impl(*this)) {}

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
} // namespace oclero
