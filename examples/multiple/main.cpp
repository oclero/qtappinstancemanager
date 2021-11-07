#include <QCoreApplication>
#include <QDebug>

#include <oclero/QtAppInstanceManager.hpp>

int main(int argc, char* argv[]) {
  QCoreApplication::setApplicationName("QtAppInstanceManagerExample");
  QCoreApplication::setApplicationVersion("1.0.0");
  QCoreApplication::setOrganizationName("oclero");
  QCoreApplication app(argc, argv);

  oclero::QtAppInstanceManager instanceManager;

  QObject::connect(&instanceManager, &oclero::QtAppInstanceManager::secondaryInstanceMessageReceived, &instanceManager,
    [&instanceManager](const unsigned int id, QByteArray const& data) {
      qDebug() << "Message received from secondary instance: " << data;
    });

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
    const auto msg = "Secondary instance started with args: " + app.arguments().join(', ');
    instanceManager.sendMessageToPrimary(msg.toUtf8());
  }

  return app.exec();
}
