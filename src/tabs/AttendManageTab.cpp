#include "AttendManageTab.h"
#include "attend_manage/AttendBoardWidget.h"
#include "attend_manage/AttendApprovalWidget.h"
#include "attend_manage/ShiftSettingsWidget.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QTabWidget>

AttendManageTab::AttendManageTab(int empId, const QString &role,
                                 std::function<void(const QString&, const QString&)> logFn,
                                 std::function<void(int, const QString&, const QString&)> notifyFn,
                                 QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QTabWidget *topTabs = new QTabWidget(this);
    topTabs->setObjectName("attendManageTabs");
    topTabs->setStyleSheet(
        "QTabWidget::panel {"
        "  border: 1px solid #e2e8f0;"
        "  background-color: #ffffff;"
        "  border-radius: 8px;"
        "}"
    );

    const bool canApproveLeave = SessionManager::instance()->hasPermission("approve_leave");
    const bool canApproveMakeup = SessionManager::instance()->hasPermission("approve_makeup");
    const bool canManageShifts = SessionManager::instance()->hasPermission("manage_shifts");

    if (canApproveLeave || canApproveMakeup) {
        m_boardWidget = new AttendBoardWidget(m_empId, m_role, this);
        m_approvalWidget = new AttendApprovalWidget(m_empId, m_role, this);
        topTabs->addTab(m_boardWidget, "📊 考勤看板");
        topTabs->addTab(m_approvalWidget, "✍ 审批中心");
    }

    if (canManageShifts) {
        m_settingsWidget = new ShiftSettingsWidget(m_empId, m_role, this);
        topTabs->addTab(m_settingsWidget, "⚙ 班次设置");
    }

    mainLayout->addWidget(topTabs);

    // Connect signals for decoupling
    if (m_boardWidget) {
        connect(m_boardWidget, &AttendBoardWidget::logRequested, this, [this](const QString &act, const QString &det) {
            if (m_log) m_log(act, det);
        });
    }

    if (m_approvalWidget) {
        connect(m_approvalWidget, &AttendApprovalWidget::logRequested, this, [this](const QString &act, const QString &det) {
            if (m_log) m_log(act, det);
        });
        connect(m_approvalWidget, &AttendApprovalWidget::notificationRequested, this, [this](int targetId, const QString &t, const QString &c) {
            if (m_notify) m_notify(targetId, t, c);
        });
    }

    if (m_settingsWidget) {
        connect(m_settingsWidget, &ShiftSettingsWidget::logRequested, this, [this](const QString &act, const QString &det) {
            if (m_log) m_log(act, det);
        });
    }

    refresh();
}

void AttendManageTab::refresh()
{
    if (m_boardWidget) m_boardWidget->refresh();
    if (m_approvalWidget) m_approvalWidget->refresh();
    if (m_settingsWidget) m_settingsWidget->refresh();
}
