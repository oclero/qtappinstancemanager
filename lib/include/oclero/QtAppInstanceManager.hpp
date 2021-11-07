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
  explicit QtAppInstanceManager(QObject* parent = nullptr);
  ~QtAppInstanceManager();

  bool isPrimaryInstance() const;
  bool isSecondaryInstance() const;
  int secondaryInstanceCount() const;
  
public slots:
  void sendMessageToPrimary(const QByteArray& data);
  void sendMessageToSecondary(const unsigned int id, const QByteArray& data);

signals:
  void instanceRoleChanged();
  void primaryInstanceMessageReceived(const QByteArray& data);
  void secondaryInstanceMessageReceived(const unsigned int id, const QByteArray& data);

private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
} // namespace oclero
