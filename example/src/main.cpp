#include <QCoreApplication>
#include <QDebug>

#include <oclero/QtAppInstanceManager.hpp>

int main(int argc, char* argv[]) {
  // Necessary to get a socket name and to have an event loop running.
  QCoreApplication::setApplicationName("QtAppInstanceManagerExample");
  QCoreApplication::setApplicationVersion("1.0.0");
  QCoreApplication::setOrganizationName("oclero");
  QCoreApplication app(argc, argv);

  oclero::QtAppInstanceManager instanceManager;
  QCoreApplication::processEvents();

  // If we are the first instance to launch, we'll receive messages from other instances that will launch after.
  QObject::connect(&instanceManager, &oclero::QtAppInstanceManager::secondaryInstanceMessageReceived, &instanceManager,
    [&instanceManager](const unsigned int clientId, QByteArray const& data) {
      qDebug() << "Message received from secondary instance: " << data;
    });

  // If we are a secondary instance to launch, we'll receive messages from the primary instance that launch first.
  QObject::connect(&instanceManager, &oclero::QtAppInstanceManager::primaryInstanceMessageReceived, &instanceManager,
    [](QByteArray const& data) {
      qDebug() << "Message received from primary instance: " << data;
    });
  QObject::connect(
    &instanceManager, &oclero::QtAppInstanceManager::instanceRoleChanged, &instanceManager, [&instanceManager]() {
      if (!instanceManager.isPrimaryInstance() && !instanceManager.isSecondaryInstance()) {
        qDebug() << "Waiting for new instance role...";
      } else {
        qDebug() << "New instance role: " << (instanceManager.isPrimaryInstance() ? "Primary" : "Secondary");
      }
    });

  if (instanceManager.isPrimaryInstance()) {
    qDebug() << "Primary instance started";
  } else if (instanceManager.isSecondaryInstance()) {
    qDebug() << "Secondary instance started";
    // You may send a message to the primary to signal that a second instance was started
    const auto msg = "Secondary instance started with args: " + app.arguments().join(', ');
    instanceManager.sendMessageToPrimary(msg.toUtf8());

    // You may then exit to ensure only one instance is running at the same time.
    app.exit();
    std::exit(EXIT_SUCCESS);
  }

  return app.exec();
}
