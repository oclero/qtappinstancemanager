#pragma once

#include <QObject>
#include <memory>

namespace oclero {
/**
 * @brief Used to ensure only one instance of the application is running at the same time.
 */
class QtAppInstanceManager : public QObject {
  Q_OBJECT

public:
  enum class Mode {
    MultipleInstances,
    SingleInstance,
  };

  enum class AppExitMode {
    Manual,
    Auto,
  };

  explicit QtAppInstanceManager(QObject* parent = nullptr);
  explicit QtAppInstanceManager(Mode mode, QObject* parent = nullptr);
  explicit QtAppInstanceManager(Mode mode, AppExitMode appExitMode, QObject* parent = nullptr);

  ~QtAppInstanceManager();

  bool isPrimaryInstance() const;
  bool isSecondaryInstance() const;
  int secondaryInstanceCount() const;

  Mode mode() const;
  void setMode(Mode mode);

  AppExitMode appExitMode() const;
  void setAppExitMode(AppExitMode appExitMode);

public slots:
  void sendMessageToPrimary(const QByteArray& data);
  void sendMessageToSecondary(const unsigned int id, const QByteArray& data);

signals:
  void instanceRoleChanged();
  void primaryInstanceMessageReceived(const QByteArray& data);
  void secondaryInstanceMessageReceived(const unsigned int id, const QByteArray& data);
  void modeChanged();
  void appExitModeChanged();
  void appExitRequested();

private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
} // namespace oclero
