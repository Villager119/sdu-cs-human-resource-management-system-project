#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSqlDatabase>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EmployeeTab; class MyAttendanceTab; class AttendManageTab; class PayrollTab; class AuditTab;
class DashboardTab; class PerformanceTab;
class OrgTab; class ProfileChangeTab; class RbacTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int empId, QString role, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void actionChangePasswordTriggered();
    void actionLogoutTriggered();
    void checkAuditLogs();

private:
    void logAction(const QString &action, const QString &target = QString());
    void notifyUser(int empId, const QString &title, const QString &content);
    void notifyAdmins(const QString &title, const QString &content);
    void notifyPermittedUsers(const QString &permissionKey, const QString &title, const QString &content);
    void refreshBell();
    void showNotifications();
    void addNavItem(const QString &icon, const QString &label, QWidget *page, bool visible = true);
    void updateStatusBar();
    void refreshActiveTab();
    bool confirmLeavePage(QWidget *page);
    void toggleSidebar();
    void loadCurrentUserName();
    void createContentTabs();
    void applyPermissionVisibility();
    QWidget *makeDynamicWrapper(const QList<QPair<QWidget*, QString>> &tabsList);
    void setupNavigation();
    void setupNotifications();
    void initializeAuditCursor();
    void setupSystemMenu();
    void connectGlobalRefreshSignals();
    void navigateFromShortcut(const QString &target);
    void selectNavByLabel(const QString &label);
    void selectCurrentSubTab(const QString &tabText);
    QSqlDatabase backgroundDatabase();

    struct NavItemInfo {
        QString icon;
        QString label;
        class QListWidgetItem *listItem;
    };
    QList<NavItemInfo> m_navItems;
    bool m_sidebarCollapsed = false;
    bool m_suppressNavigationChange = false;
    int m_lastSidebarRow = -1;
    QWidget *m_sidebarContainer = nullptr;

    Ui::MainWindow *ui;
    QPushButton *m_bellBtn;
    class QTimer *m_bellTimer;
    class QTimer *m_pollTimer;
    bool m_bellFlashState = false;

    DashboardTab *m_dashboard; EmployeeTab *m_empTab; MyAttendanceTab *m_myAttendTab;
    AttendManageTab *m_attendManageTab; PayrollTab *m_payrollTab; AuditTab *m_auditTab;
    PerformanceTab *m_perfTab; ProfileChangeTab *m_profileTab;
    OrgTab *m_orgTab; RbacTab *m_rbacTab;

    int m_empId; QString m_role; QString m_empName;
    int m_lastMaxLogId = -1;
    QString m_backgroundConnectionName;
};

#endif
