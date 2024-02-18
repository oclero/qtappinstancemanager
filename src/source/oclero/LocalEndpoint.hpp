#pragma once

#include <memory>

#include <QObject>
#include <QByteArray>
#include <QVector>

namespace oclero {
/**
 * @brief Allows communication using a local socket (locked file).
 * The first LocalEndpoint acts as the server, and the other ones act as clients.
 */
class LocalEndpoint : public QObject {
  Q_OBJECT

  Q_PROPERTY(Role role READ role NOTIFY roleChanged)
  Q_PROPERTY(Id id READ id NOTIFY idChanged)
  Q_PROPERTY(Id serverId READ serverId NOTIFY serverIdChanged)
  Q_PROPERTY(int secondaryInstanceCount READ secondaryInstanceCount NOTIFY clientCountChanged)

public:
  using Id = quint64;

  enum class Role {
    Unknown,
    Server,
    Client,
  };
  Q_ENUM(Role)

public:
  explicit LocalEndpoint(QObject* parent = nullptr);
  ~LocalEndpoint();

public:
  int secondaryInstanceCount() const;
  Id id() const;
  Id serverId() const;
  Role role() const;
  void sendToServer(const QByteArray& data);
  void sendToAllClients(const QByteArray& data, const QVector<Id>& exceptIds = {});
  void sendToClient(Id clientId, const QByteArray& data);

signals:
  /// Emitted when the endpoint's role has changed.
  void roleChanged();

  /// Emitted when the endpoint's id has changed.
  void idChanged();

  /// Emitted when the server's id has changed.
  void serverIdChanged();

  /// Emitted when a message from a secondary endpoint is received (usually called on primary endpoint).
  void clientMessageReceived(const Id clientId, const QByteArray& data);

  /// Emitted when a message from the primary endpoint is received (usually called on secondary endpoints).
  void serverMessageReceived(const QByteArray& data);

  /// Emitted when the number of endpoints has changed.
  void clientCountChanged();

private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
} // namespace oclero
