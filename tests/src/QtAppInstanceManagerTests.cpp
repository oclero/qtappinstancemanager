#include "QtAppInstanceManagerTests.hpp"

#include <oclero/QtAppInstanceManager.hpp>
#include <QCoreApplication>
#include <QTest>
#include <QTimer>

#include <array>

using namespace oclero;

void Tests::test_roles() {
  // Primary instance..
  QtAppInstanceManager primaryInstance;
  QCoreApplication::processEvents();
  const auto primaryInstanceIsPrimary = primaryInstance.isPrimaryInstance();
  const auto primaryInstanceIsNotSecondary = !primaryInstance.isSecondaryInstance();
  QVERIFY(primaryInstanceIsPrimary);
  QVERIFY(primaryInstanceIsNotSecondary);

  // Secondary instances.
  QtAppInstanceManager secondaryInstance1;
  QCoreApplication::processEvents();
  const auto secondaryInstance1IsNotPrimary = !secondaryInstance1.isPrimaryInstance();
  const auto secondaryInstance1IsSecondary = secondaryInstance1.isSecondaryInstance();
  QVERIFY(secondaryInstance1IsNotPrimary);
  QVERIFY(secondaryInstance1IsSecondary);

  QtAppInstanceManager secondaryInstance2;
  QCoreApplication::processEvents();
  const auto secondaryInstance2IsNotPrimary = !secondaryInstance2.isPrimaryInstance();
  const auto secondaryInstance2IsSecondary = secondaryInstance2.isSecondaryInstance();
  QVERIFY(secondaryInstance2IsNotPrimary);
  QVERIFY(secondaryInstance2IsSecondary);
}

void Tests::test_messages() {
  constexpr auto expectedRequest = "request";
  constexpr auto expectedResponse = "response";

  // Primary instance.
  QtAppInstanceManager primaryInstance;
  QCoreApplication::processEvents();

  auto fail = false;
  auto actualRequestData = QByteArray{};

  // Secondary Instance.
  QtAppInstanceManager secondaryInstance;
  QCoreApplication::processEvents();

  QObject::connect(&primaryInstance, &QtAppInstanceManager::secondaryInstanceMessageReceived, &primaryInstance,
    [&primaryInstance, &actualRequestData, &expectedRequest, &fail, &expectedResponse](
      const unsigned int id, QByteArray const& data) {
      actualRequestData = data;

      // Answer to secondaryInstance.
      QTimer::singleShot(0, [&primaryInstance, id, &expectedResponse]() {
        primaryInstance.sendMessageToSecondary(id, expectedResponse);
      });
    });

  auto actualResponseData = QByteArray{};
  QObject::connect(&secondaryInstance, &QtAppInstanceManager::primaryInstanceMessageReceived, &secondaryInstance,
    [&actualResponseData](QByteArray const& data) {
      // Copy response to verify it.
      actualResponseData = data;
    });

  // Start interaction between primary instance and secondary one.
  secondaryInstance.sendMessageToPrimary({ expectedRequest });

  if (!QTest::qWaitFor(
        [&actualRequestData]() {
          return !actualRequestData.isEmpty();
        },
        1000)) {
    QFAIL("Received request data by primary instance from secondary ne is not what was expected.");
  }
  QCOMPARE(actualRequestData, expectedRequest);

  if (!QTest::qWaitFor(
        [&actualResponseData]() {
          return !actualResponseData.isEmpty();
        },
        1000)) {
    QFAIL("Received response data by secondary instanc from primary one is not what was expected.");
  }
  QCOMPARE(actualResponseData, expectedResponse);
}

void Tests::test_primaryInstanceKilled() {
  // Primary instance.
  auto primaryInstance = std::make_unique<QtAppInstanceManager>();
  QCoreApplication::processEvents();

  // Seecondary instances
  constexpr auto secondaryInstanceCount = 3;
  std::array<std::unique_ptr<QtAppInstanceManager>, secondaryInstanceCount> secondaryInstances;
  for (auto i = 0; i < secondaryInstanceCount; ++i) {
    auto secondaryInstance = std::make_unique<QtAppInstanceManager>();
    secondaryInstances[i] = std::move(secondaryInstance);
  }
  QTest::qWait(1000);

  // Check if the secondary instance becomes a primary one when the former has been shutdown.
  auto primaryInstanceReplaced = false;
  std::for_each(
    secondaryInstances.begin(), secondaryInstances.end(), [this, &primaryInstanceReplaced](auto& secondaryInstance) {
      QObject::connect(secondaryInstance.get(), &QtAppInstanceManager::instanceRoleChanged, secondaryInstance.get(),
        [this, &secondaryInstance, &primaryInstanceReplaced]() {
          if (secondaryInstance->isPrimaryInstance()) {
            primaryInstanceReplaced = true;
          }
        });
    });

  // Kill the primary instance.
  primaryInstance.reset();

  // Roles should have changed: one of the secondary instances should have become the primary one.
  if (!QTest::qWaitFor(
        [&primaryInstanceReplaced]() {
          return primaryInstanceReplaced;
        },
        1000)) {
    QFAIL("Primary instance was killed but not replaced by a secondary one taking the primary role.");
  }
}

void Tests::test_secondaryInstanceCount() {
  // Primary instance.
  QtAppInstanceManager primaryInstance;
  QCoreApplication::processEvents();

  // Secondary instances.
  constexpr auto expectedSecondaryInstanceCount = 3;
  std::array<std::unique_ptr<QtAppInstanceManager>, expectedSecondaryInstanceCount> secondaryInstances;
  for (auto i = 0; i < expectedSecondaryInstanceCount; ++i) {
    auto secondaryInstance = std::make_unique<QtAppInstanceManager>();
    const auto secondaryInstanceCount = secondaryInstance->secondaryInstanceCount();
    QVERIFY(secondaryInstanceCount == 0);
    secondaryInstances[i] = std::move(secondaryInstance);
  }
  QTest::qWait(1000);

  const auto actualCount = primaryInstance.secondaryInstanceCount();
  QCOMPARE(actualCount, expectedSecondaryInstanceCount);

  // Kill secondary instances.
  std::for_each(secondaryInstances.begin(), secondaryInstances.end(), [](auto& secondaryInstance) {
    secondaryInstance.reset();
  });
  QTest::qWait(1000);

  // Count after secondary instances are disconnected.
  const auto countAfterDisconnections = primaryInstance.secondaryInstanceCount();
  QCOMPARE(countAfterDisconnections, 0);
}
