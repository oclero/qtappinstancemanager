#include <QCoreApplication>
#include <QDebug>

#include <oclero/QtAppInstanceManager.hpp>

int main(int argc, char* argv[]) {
  QCoreApplication::setApplicationName("SingleInstanceExample");
  QCoreApplication::setApplicationVersion("1.0.0");
  QCoreApplication::setOrganizationName("example");
  QCoreApplication app(argc, argv);

  oclero::QtAppInstanceManager instanceManager(oclero::QtAppInstanceManager::Mode::SingleInstance);
  qDebug() << "Instance role: " << (instanceManager.isPrimaryInstance() ? "Primary" : "Secondary");

  QObject::connect(&instanceManager, &oclero::QtAppInstanceManager::secondaryInstanceMessageReceived, &instanceManager,
    [&instanceManager](const unsigned int id, QByteArray const& data) {
      qDebug() << "Secondary instance started then immediately quit with args: " << data;
    });

  return app.exec();
}
