#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EmployeeTab; class LeaveTab; class PayrollTab; class AuditTab;
class ReportsTab; class DashboardTab; class PerformanceTab;
class OrgTab; class ProfileChangeTab; class AttendTaxTab; class RbacTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int empId, QString role, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionChangePassword_triggered();
    void on_actionLogout_triggered();

private:
    void logAction(const QString &action, const QString &target = QString());
    void notifyUser(int empId, const QString &title, const QString &content);
    void notifyAdmins(const QString &title, const QString &content);
    void refreshBell();
    void showNotifications();
    void addNavItem(const QString &icon, const QString &label, QWidget *page, bool visible = true);
    void updateStatusBar();
    void refreshActiveTab();

    Ui::MainWindow *ui;
    QPushButton *m_bellBtn;
    class QTimer *m_bellTimer;
    class QTimer *m_pollTimer;
    bool m_bellFlashState = false;

    DashboardTab *m_dashboard; EmployeeTab *m_empTab; LeaveTab *m_leaveTab;
    PayrollTab *m_payrollTab; AuditTab *m_auditTab; ReportsTab *m_reportsTab;
    PerformanceTab *m_perfTab; ProfileChangeTab *m_profileTab;
    AttendTaxTab *m_attTaxTab; OrgTab *m_orgTab; RbacTab *m_rbacTab;

    int m_empId; QString m_role; QString m_empName;
};

#endif
