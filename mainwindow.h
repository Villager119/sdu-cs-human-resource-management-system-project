#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EmployeeTab;
class LeaveTab;
class PayrollTab;
class AuditTab;
class ReportsTab;
class DashboardTab;
class PerformanceTab;
class OrgTab;
class ProfileChangeTab;
class AttendTaxTab;

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
    Ui::MainWindow *ui;
    void logAction(const QString &action, const QString &target = QString());

    DashboardTab *m_dashboard;
    EmployeeTab *m_empTab;
    LeaveTab *m_leaveTab;
    PayrollTab *m_payrollTab;
    AuditTab *m_auditTab;
    ReportsTab *m_reportsTab;
    PerformanceTab *m_perfTab;
    OrgTab *m_orgTab;
    ProfileChangeTab *m_profileTab;
    AttendTaxTab *m_attTaxTab;
    int m_empId;
    QString m_role;
    QString m_empName;
};

#endif
