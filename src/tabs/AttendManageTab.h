#ifndef ATTENDMANAGETAB_H
#define ATTENDMANAGETAB_H

#include <QWidget>
#include <functional>

class AttendBoardWidget;
class AttendApprovalWidget;
class ShiftSettingsWidget;

class AttendManageTab : public QWidget
{
    Q_OBJECT
public:
    AttendManageTab(int empId, const QString &role,
                    std::function<void(const QString&, const QString&)> logFn,
                    std::function<void(int, const QString&, const QString&)> notifyFn,
                    QWidget *parent = nullptr);
    void refresh();

private:
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;

    AttendBoardWidget *m_boardWidget = nullptr;
    AttendApprovalWidget *m_approvalWidget = nullptr;
    ShiftSettingsWidget *m_settingsWidget = nullptr;
};

#endif // ATTENDMANAGETAB_H
