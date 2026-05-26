#include "MyAttendanceTab.h"
#include "my_attendance/DailyClockWidget.h"
#include "my_attendance/AppCenterWidget.h"
#include <QVBoxLayout>
#include <QTabWidget>

MyAttendanceTab::MyAttendanceTab(int empId, const QString &role,
                                 std::function<void(const QString&, const QString&)> logFn,
                                 std::function<void(int, const QString&, const QString&)> notifyFn,
                                 QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QTabWidget *topTabs = new QTabWidget(this);
    topTabs->setObjectName("myAttendTabs");
    topTabs->setStyleSheet(
        "QTabWidget::panel {"
        "  border: 1px solid #e2e8f0;"
        "  background-color: #ffffff;"
        "  border-radius: 8px;"
        "}"
    );

    m_clockWidget = new DailyClockWidget(m_empId, m_role, this);
    m_appWidget = new AppCenterWidget(m_empId, m_role, this);

    topTabs->addTab(m_clockWidget, "⏰ 每日打卡");
    topTabs->addTab(m_appWidget, "📂 申请中心");

    mainLayout->addWidget(topTabs);

    // Connect signals and slots using lambdas for decoupling
    connect(m_clockWidget, &DailyClockWidget::logRequested, this, [this](const QString &act, const QString &det) {
        if (m_log) m_log(act, det);
    });
    connect(m_clockWidget, &DailyClockWidget::notificationRequested, this, [this](int targetId, const QString &t, const QString &c) {
        if (m_notify) m_notify(targetId, t, c);
    });

    connect(m_appWidget, &AppCenterWidget::logRequested, this, [this](const QString &act, const QString &det) {
        if (m_log) m_log(act, det);
    });
    connect(m_appWidget, &AppCenterWidget::notificationRequested, this, [this](int targetId, const QString &t, const QString &c) {
        if (m_notify) m_notify(targetId, t, c);
    });

    refresh();
}

void MyAttendanceTab::refresh()
{
    if (m_clockWidget) m_clockWidget->refresh();
    if (m_appWidget) m_appWidget->refresh();
}
