#include "LocalEndpoint.hpp"

#include <QLoggingCategory>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSharedMemory>
#include <QString>
#include <QTimer>
#include <QDataStream>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QRegularExpression>

#include <vector>

Q_LOGGING_CATEGORY(LOGCAT_LOCALENDPOINT, "oclero.localEndpoint")

#if !defined LOGCAT_LOCALENDPOINT
#  define LOGCAT_LOCALENDPOINT 0
#endif

namespace oclero {
enum class Step {
  Handshake,
  Header,
  Body,
};

struct SocketConnectionInfo {
  QLocalSocket* socket{ nullptr };
  LocalEndpoint::Id id{ 0u };
  Step step{ Step::Handshake };
  quint64 pid{ 0u };
  quint64 bodySize{ 0u };
};

struct LocalEndpoint::Impl {
  LocalEndpoint& owner;
  const QString socketName{ Impl::getSocketName() };
  QSharedMemory sharedMemory{ socketName };
  Role role{ Role::Unknown };

  std::unique_ptr<QLocalServer> server{};
  std::vector<SocketConnectionInfo> serverClients;

  SocketConnectionInfo clientSocketInfo;
  std::unique_ptr<QLocalSocket> client{};

  Impl(LocalEndpoint& o)
    : owner(o) {}

  ~Impl() {
#if LOGCAT_LOCALENDPOINT
    if (role == Role::Server) {
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Destructor...";
    } else if (role == Role::Client) {
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Destructor...";
    }
#endif

    clearServer();
    clearClient();
  }

  void clear() {
    role = Role::Unknown;

    clearServer();
    clearClient();

    emit owner.serverIdChanged();
    emit owner.roleChanged();
  }

  void restart() {
    sharedMemory.detach();
    QTimer::singleShot(0, [this]() {
      init();
    });
  }

  void init() {
    // We use a shared memory as a mutex, so we have only one server instance at a time.
    static constexpr auto SHARED_MEMORY_SIZE = 1;

    clear();

    if (sharedMemory.create(SHARED_MEMORY_SIZE, QSharedMemory::ReadOnly)) {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "Starting in server mode...";
#endif
      role = Role::Server;
      initServer();
      emit owner.roleChanged();
    } else if (sharedMemory.attach()) {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "Starting in client mode...";
#endif
      role = Role::Client;
      initClient();
      emit owner.roleChanged();
    } else {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "Restarting...";
#endif
      restart();
    }
  }

#pragma region Server

  void clearServer() {
    // Reset server.
    for (auto& item : serverClients) {
      if (item.socket) {
        // Disconnect from signals.
        item.socket->disconnect();

        // Disconnect server.
        item.socket->disconnectFromServer();
        item.socket->close();

        // Reset pointer.
        item.socket->deleteLater();
        item.socket = nullptr;
      }
    }
    serverClients.clear();
    server.reset();
  }

  std::vector<SocketConnectionInfo>::iterator findClient(const QLocalSocket* const socket) {
    return std::find_if(serverClients.begin(), serverClients.end(), [socket](const SocketConnectionInfo& item) {
      return item.socket == socket;
    });
  }

  std::vector<SocketConnectionInfo>::iterator findClient(decltype(SocketConnectionInfo::id) const clientId) {
    return std::find_if(serverClients.begin(), serverClients.end(), [clientId](const auto& item) {
      return item.id == clientId;
    });
  }

  void initServer() {
#if defined(Q_OS_UNIX)
    {
      // By explicitly attaching it and then deleting it we make sure that
      // the memory is deleted even after the process has crashed on Unix.
      auto m = std::make_unique<QSharedMemory>(socketName);
      m->attach();
    }
#endif

    server = std::make_unique<QLocalServer>();
    server->setSocketOptions(QLocalServer::SocketOption::WorldAccessOption);

    QObject::connect(server.get(), &QLocalServer::newConnection, &owner, [this]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] New client connection";
#endif
      addClient(server->nextPendingConnection());
    });

    server->listen(socketName);
  }

  void addClient(QLocalSocket* const socket) {
    if (!socket)
      return;

    if (findClient(socket) != serverClients.end())
      return;

    const auto id = socket->socketDescriptor();
    auto& it =
      serverClients.emplace_back(SocketConnectionInfo{ socket, static_cast<decltype(SocketConnectionInfo::id)>(id) });

    QObject::connect(socket, &QLocalSocket::destroyed, &owner, [this, socket]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Client destroyed";
#endif
      removeClient(socket);
    });

#pragma warning(push)
#pragma warning(disable : 26812) // Warning about Qt not using enum class
    QObject::connect(
      socket, &QLocalSocket::errorOccurred, &owner, [this](QLocalSocket::LocalSocketError const errorCode) {
#if LOGCAT_LOCALENDPOINT
        qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Client error occurred:" << errorCode;
#else
				Q_UNUSED(errorCode);
#endif
      });
#pragma warning(pop)

    QObject::connect(socket, &QLocalSocket::disconnected, &owner, [this, socket]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Client disconnected";
#endif
      removeClient(socket);
    });

    QObject::connect(socket, &QLocalSocket::aboutToClose, &owner, [this, socket]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Client about to close";
#endif
      removeClient(socket);
    });

    QObject::connect(socket, &QLocalSocket::readyRead, &owner, [this, socket]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Client message received";
#endif
      onClientMessageReceived(socket);
    });

    // Maybe already some data?
    if (socket->bytesAvailable() > 0) {
      onClientMessageReceived(socket);
    }

    emit owner.clientCountChanged();
  }

  void removeClient(QLocalSocket* const socket) {
    if (!socket)
      return;

    const auto it = std::remove_if(serverClients.begin(), serverClients.end(), [socket](const auto& item) {
      return item.socket == socket;
    });
    if (it != serverClients.end()) {
      socket->disconnect();
      serverClients.erase(it, serverClients.end());
      emit owner.clientCountChanged();
    }
  }

  void onClientMessageReceived(const QLocalSocket* const socket) {
    if (!socket)
      return;

    const auto it = findClient(socket);
    if (it != serverClients.end()) {
      auto& socketInfo = *it;
      switch (socketInfo.step) {
        case Step::Handshake:
          readClientHandshake(socketInfo);
          sendHandshakeToClient(socketInfo);
          // There is some bytes left to read: it should be the header.
          if (socketInfo.socket->bytesAvailable() >= sizeof(SocketConnectionInfo::bodySize)) {
            readClientMessageHeader(socketInfo);
          }
          break;
        case Step::Header:
          readClientMessageHeader(socketInfo);
          break;
        case Step::Body:
          readClientMessageBody(socketInfo);
          break;
        default:
          break;
      }
    }
  }

  void readClientHandshake(SocketConnectionInfo& socketInfo) const {
    socketInfo.pid = {};
    if (socketInfo.socket->bytesAvailable() < static_cast<quint64>(sizeof(SocketConnectionInfo::pid))) {
      return;
    }

    // Read client PID.
    auto clientPid = decltype(SocketConnectionInfo::pid){};
    {
      QDataStream stream(socketInfo.socket);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> clientPid;
    }

    socketInfo.pid = clientPid;

    // Move state machine to next step.
    socketInfo.step = Step::Header;
  }

  void readClientMessageHeader(SocketConnectionInfo& socketInfo) const {
    socketInfo.bodySize = {};

    if (socketInfo.socket->bytesAvailable() < static_cast<qint64>(sizeof(SocketConnectionInfo::bodySize))) {
      return;
    }

    // Read body size.
    auto bodySize = decltype(SocketConnectionInfo::bodySize){};
    {
      QDataStream stream(socketInfo.socket);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> bodySize;
    }

    socketInfo.bodySize = bodySize;

    // Move state machine to next step.
    socketInfo.step = Step::Body;

    // There is some bytes left to read: it should be the body.
    if (socketInfo.socket->bytesAvailable() >= bodySize) {
      readClientMessageBody(socketInfo);
    }
  }

  void readClientMessageBody(SocketConnectionInfo& socketInfo) const {
    if (socketInfo.socket->bytesAvailable() < socketInfo.bodySize || socketInfo.step != Step::Body) {
      // Wait for more bytes to be written.
      return;
    }

    // Read whole body.
    QByteArray body;
    {
      QDataStream stream(socketInfo.socket);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> body;
    }

    // Move state machine to next step (go back to Header step).
    socketInfo.step = Step::Header;

#if LOGCAT_LOCALENDPOINT
    qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Received from client:" << socketInfo.bodySize << "bytes";
#endif
    emit owner.clientMessageReceived(socketInfo.id, body);
  }

  void sendHandshakeToClient(const SocketConnectionInfo& socketInfo) {
    const auto& socket = socketInfo.socket;
    if (socket && socket->isValid()) {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Server] Sending handshake to client" << socket->socketDescriptor();
#endif
      socket->write(getServerHandshake(socketInfo));
      socket->flush();
    }
  }

  QByteArray getServerHandshake(const SocketConnectionInfo& socketInfo) const {
    QByteArray handshake;
    {
      QDataStream stream(&handshake, QIODevice::WriteOnly);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      // Send its id to the client.
      const auto clientId = socketInfo.socket->socketDescriptor();
      stream << static_cast<quint64>(clientId);
    }

    return handshake;
  }

  void sendMessageToClient(const SocketConnectionInfo& socketInfo, const QByteArray& data) const {
    const auto& socket = socketInfo.socket;
    if (socketInfo.step != Step::Handshake && socket
        && socket->state() == QLocalSocket::LocalSocketState::ConnectedState) {
      socket->write(Impl::getHeader(data));
      socket->write(Impl::getBody(data));
      socket->flush();
    }
  }

#pragma endregion

#pragma region Client

  void clearClient() {
    if (client) {
      // Disconnect from signals.
      client->disconnect();

      // Disconnect from server.
      if (client->state() == QLocalSocket::ConnectedState) {
#if LOGCAT_LOCALENDPOINT
        qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Disconnecting from server";
#endif
        client->disconnectFromServer();

        if (client->state() != QLocalSocket::UnconnectedState) {
          const auto disconnectResult = client->waitForDisconnected(1000);

#if LOGCAT_LOCALENDPOINT
          if (client->state() != QLocalSocket::UnconnectedState || !disconnectResult) {
            qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Error: can't disconnect from server. Code:" << client->error();
          }
#endif
        }
      }
      client->close();

      // Reset pointer.
      client.reset();
    }
    clientSocketInfo = {};
  }

  void initClient() {
    clientSocketInfo = {};
    client = std::make_unique<QLocalSocket>();
    clientSocketInfo.socket = client.get();
    clientSocketInfo.pid = QCoreApplication::applicationPid();

    QObject::connect(
      client.get(), &QLocalSocket::errorOccurred, &owner, [this](QLocalSocket::LocalSocketError const errorCode) {
#if LOGCAT_LOCALENDPOINT
        qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Error occurred:" << errorCode;
#endif
        if (errorCode == QLocalSocket::ServerNotFoundError || errorCode == QLocalSocket::ConnectionRefusedError) {
          restart();
        }
      });

    QObject::connect(client.get(), &QLocalSocket::connected, &owner, [this]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Connected to server";
#endif
      sendHandshakeToServer();
    });

    QObject::connect(client.get(), &QLocalSocket::disconnected, &owner, [this]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Disconnected from server";
#endif
      restart();
    });

    QObject::connect(client.get(), &QLocalSocket::readyRead, &owner, [this]() {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Server message received";
#endif
      onMessageReceivedFromServer();
    });

    client->connectToServer(socketName);
  }

  void onMessageReceivedFromServer() {
    switch (clientSocketInfo.step) {
      case Step::Handshake:
        readServerHandshake();
        // There is some bytes left to read: it should be the header.
        if (clientSocketInfo.socket->bytesAvailable() >= sizeof(SocketConnectionInfo::bodySize)) {
          readServerMessageHeader();
        }
        break;
      case Step::Header:
        readServerMessageHeader();
        break;
      case Step::Body:
        readServerMessageBody();
        break;
      default:
        break;
    };
  }

  void readServerHandshake() {
    clientSocketInfo.id = 0u;
    if (clientSocketInfo.socket->bytesAvailable() < (quint64) sizeof(SocketConnectionInfo::id)) {
      return;
    }

    // Read the id that the server gave us.
    auto idGivenByServer = quint64{};
    {
      QDataStream stream(client.get());
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> idGivenByServer;
    }

    clientSocketInfo.id = idGivenByServer;
    emit owner.serverIdChanged();

#if LOGCAT_LOCALENDPOINT
    qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] ID given by server is:" << clientSocketInfo.id;
#endif

    // Move state machine to next step.
    clientSocketInfo.step = Step::Header;
  }

  void readServerMessageHeader() {
    clientSocketInfo.bodySize = {};

    if (client->bytesAvailable() < (quint64) sizeof(SocketConnectionInfo::bodySize)) {
      return;
    }

    // Read body size.
    auto bodySize = decltype(SocketConnectionInfo::bodySize){};
    {
      QDataStream stream(client.get());
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> bodySize;
    }

    clientSocketInfo.step = Step::Body;
    clientSocketInfo.bodySize = bodySize;

    // There is some bytes left to read: it should be the body.
    if (client->bytesAvailable() >= bodySize) {
      readServerMessageBody();
    }
  }

  void readServerMessageBody() {
    if (client->bytesAvailable() < clientSocketInfo.bodySize || clientSocketInfo.step != Step::Body) {
      // Wait for more bytes to be written.
      return;
    }

    // Read whole body.
    QByteArray body;
    {
      QDataStream stream(client.get());
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream >> body;
    }

    clientSocketInfo.step = Step::Header;

#if LOGCAT_LOCALENDPOINT
    qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Received from server:" << clientSocketInfo.bodySize << "bytes";
#endif
    emit owner.serverMessageReceived(body);
  }

  void sendHandshakeToServer() const {
    if (client && client->isValid()) {
#if LOGCAT_LOCALENDPOINT
      qCDebug(LOGCAT_LOCALENDPOINT) << "[Client] Sending handshake to server.";
#endif
      client->write(getClientHandshake());
      client->flush();
    }
  }

  QByteArray getClientHandshake() const {
    QByteArray handshake;
    {
      QDataStream stream(&handshake, QIODevice::WriteOnly);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      // Send id to server.
      const auto clientId = QCoreApplication::applicationPid();
      stream << static_cast<quint64>(clientId);
    }

    return handshake;
  }

  void sendMessageToServer(const QByteArray& data) const {
    if (client && client->isValid()) {
      client->write(Impl::getHeader(data));
      client->write(Impl::getBody(data));
      client->flush();
    }
  }

#pragma endregion

#pragma region Static

  static QByteArray getHeader(const QByteArray& body) {
    QByteArray header;
    {
      QDataStream stream(&header, QIODevice::WriteOnly);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream << static_cast<quint64>(body.length());
    }

    return header;
  }

  static QByteArray getBody(const QByteArray& data) {
    QByteArray body;
    {
      QDataStream stream(&body, QIODevice::WriteOnly);
      stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
      stream << data;
    }

    return body;
  }

  static QString getSocketName() {
    QCryptographicHash appData(QCryptographicHash::Sha256);
    QRegularExpression const filterRegExp{ QStringLiteral("[^a-zA-Z0-9]") };
    const auto organizationName = QCoreApplication::organizationName().remove(filterRegExp).toUtf8();
    const auto applicationName = QCoreApplication::applicationName().remove(filterRegExp).toUtf8();
    const auto applicationVersion = QCoreApplication::applicationVersion().toUtf8();
    appData.addData(organizationName);
    appData.addData(applicationName);
    appData.addData(applicationVersion);

    // Replace the backslash in RFC 2045 Base64 [a-zA-Z0-9+/=] to comply with server naming requirements.
    return appData.result().toBase64().replace('/', '_');
  }

#pragma endregion
};

LocalEndpoint::LocalEndpoint(QObject* parent)
  : QObject(parent)
  , _impl(new Impl(*this)) {
  _impl->init();
}

LocalEndpoint::~LocalEndpoint() = default;

int LocalEndpoint::secondaryInstanceCount() const {
  return static_cast<int>(_impl->serverClients.size());
}

LocalEndpoint::Id LocalEndpoint::id() const {
  switch (role()) {
    case Role::Server:
      return {};
    case Role::Client:
      return _impl->clientSocketInfo.id;
    default:
      return {};
  }
}

LocalEndpoint::Id LocalEndpoint::serverId() const {
  return 0;
}

LocalEndpoint::Role LocalEndpoint::role() const {
  return _impl->role;
}

void LocalEndpoint::sendToServer(const QByteArray& data) {
  if (role() == Role::Client) {
    _impl->sendMessageToServer(data);
  }
}

void LocalEndpoint::sendToAllClients(const QByteArray& data, const QVector<LocalEndpoint::Id>& exceptIds) {
  if (role() == Role::Server) {
    for (const auto& client : _impl->serverClients) {
      if (!exceptIds.contains(client.id)) {
        _impl->sendMessageToClient(client, data);
      }
    }
    //for (auto it = _impl->serverClients.begin(); it != _impl->serverClients.end(); it++) {
    //  if (!exceptIds.contains(it->id)) {
    //    _impl->sendMessageToClient(*it, data);
    //  }
    //}
  }
}

void LocalEndpoint::sendToClient(LocalEndpoint::Id clientId, const QByteArray& data) {
  if (role() == Role::Server && clientId != serverId()) {
    const auto it = _impl->findClient(clientId);
    if (it != _impl->serverClients.end()) {
      _impl->sendMessageToClient(*it, data);
    }
  }
}
} // namespace oclero

#if defined LOGCAT_LOCALENDPOINT
#  undef LOGCAT_LOCALENDPOINT
#endif
