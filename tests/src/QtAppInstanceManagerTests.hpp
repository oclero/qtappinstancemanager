#pragma once

#include <QObject>

class Tests : public QObject {
  Q_OBJECT

public:
  using QObject::QObject;

private slots:
  void test_roles();
  void test_messages();
  void test_primaryInstanceKilled();
  void test_secondaryInstanceCount();
};
