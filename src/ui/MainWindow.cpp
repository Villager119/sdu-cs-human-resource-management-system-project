#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ChangePasswordDialog.h"
#include "LoginWindow.h"
#include "../tabs/EmployeeTab.h"
#include "../tabs/LeaveTab.h"
#include "../tabs/PayrollTab.h"
#include "../tabs/AuditTab.h"
#include "../tabs/ReportsTab.h"
#include "../tabs/DashboardTab.h"
#include "../tabs/PerformanceTab.h"
#include "../tabs/OrgTab.h"
#include "../tabs/ProfileChangeTab.h"
#include "../tabs/AttendTaxTab.h"
#include "../tabs/RbacTab.h"
#include "../widgets/TaxConfigPanel.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QMenu>
#include <QTabWidget>
#include <QMessageBox>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>
#include <QDateTime>
#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTimer>

MainWindow::MainWindow(int empId, QString role, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_empId(empId), m_role(role)
{
    ui->setupUi(this);
    setWindowTitle("HRMS");

    { QSqlQuery q; q.prepare("SELECT name FROM employees WHERE emp_id=?"); q.addBindValue(m_empId); m_empName = (q.exec()&&q.next()) ? q.value(0).toString() : "未知"; }

    auto logFn = [this](const QString &a, const QString &t) { logAction(a, t); };
    auto notifyFn = [this](int e, const QString &t, const QString &c) { notifyUser(e, t, c); };

    // 创建所有原生页面
    m_dashboard = new DashboardTab(this);
    m_empTab    = new EmployeeTab(logFn, this);
    m_leaveTab  = new LeaveTab(m_empId, m_role, logFn, notifyFn, this);
    m_payrollTab= new PayrollTab(m_empId, m_role, logFn, this);
    m_auditTab  = new AuditTab(this);
    m_orgTab    = new OrgTab(logFn, this);
    m_perfTab   = new PerformanceTab(m_empId, m_role, logFn, this);
    m_reportsTab= new ReportsTab(this);
    m_profileTab= new ProfileChangeTab(m_empId, m_role, logFn, notifyFn, this);
    m_attTaxTab = new AttendTaxTab(m_empId, m_role, logFn, notifyFn, this);
    m_rbacTab   = new RbacTab(logFn, this);

    // Hide only unauthorized tabs to prevent them from rendering as floating child widgets at (0,0)
    if (!SessionManager::instance()->hasPermission("view_dashboard")) m_dashboard->hide();
    if (!SessionManager::instance()->hasPermission("view_reports")) m_reportsTab->hide();
    if (!SessionManager::instance()->hasPermission("manage_employees")) m_empTab->hide();
    if (!(SessionManager::instance()->hasPermission("request_profile_change") || SessionManager::instance()->hasPermission("approve_profile_change"))) m_profileTab->hide();
    if (!(SessionManager::instance()->hasPermission("apply_leave_makeup") || SessionManager::instance()->hasPermission("approve_makeup"))) m_attTaxTab->hide();
    if (!(SessionManager::instance()->hasPermission("apply_leave") || SessionManager::instance()->hasPermission("approve_leave"))) m_leaveTab->hide();
    if (!(SessionManager::instance()->hasPermission("view_personal_payroll") || SessionManager::instance()->hasPermission("calculate_payroll"))) m_payrollTab->hide();
    if (!(SessionManager::instance()->hasPermission("view_personal_performance") || SessionManager::instance()->hasPermission("evaluate_performance"))) m_perfTab->hide();
    if (!SessionManager::instance()->hasPermission("manage_org")) m_orgTab->hide();
    if (!SessionManager::instance()->hasPermission("view_audit_logs")) m_auditTab->hide();
    if (!SessionManager::instance()->hasPermission("manage_rbac")) m_rbacTab->hide();

    auto makeDynamicWrapper = [this](const QList<QPair<QWidget*, QString>> &tabsList) -> QWidget* {
        QList<QPair<QWidget*, QString>> activeTabs;
        for (const auto &pair : tabsList) {
            if (pair.first) {
                activeTabs.append(pair);
            }
        }
        if (activeTabs.isEmpty()) {
            return nullptr;
        }
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        if (activeTabs.size() > 1) {
            auto *tabs = new QTabWidget;
            tabs->setObjectName("subTabWidget");
            for (const auto &pair : activeTabs) {
                tabs->addTab(pair.first, pair.second);
            }
            connect(tabs, &QTabWidget::currentChanged, this, &MainWindow::refreshActiveTab);
            l->addWidget(tabs);
        } else {
            l->addWidget(activeTabs.first().first);
        }
        return w;
    };

    m_dashboard->setObjectName("仪表盘");
    m_reportsTab->setObjectName("统计图表");
    m_empTab->setObjectName("员工管理");
    m_profileTab->setObjectName("信息变更");
    m_attTaxTab->setObjectName("打卡补卡");
    m_leaveTab->setObjectName("请假审批");
    m_payrollTab->setObjectName("工资条");
    m_perfTab->setObjectName("绩效评分");
    m_rbacTab->setObjectName("权限管理");

    TaxConfigPanel *taxPanel = nullptr;

    // 1. Home Page Wrapper
    QList<QPair<QWidget*, QString>> homeTabs;
    if (SessionManager::instance()->hasPermission("view_dashboard"))
        homeTabs.append({m_dashboard, "仪表盘"});
    if (SessionManager::instance()->hasPermission("view_reports"))
        homeTabs.append({m_reportsTab, "统计图表"});
    QWidget *homePage = makeDynamicWrapper(homeTabs);

    // 2. Employee Page Wrapper
    QList<QPair<QWidget*, QString>> empTabs;
    if (SessionManager::instance()->hasPermission("manage_employees"))
        empTabs.append({m_empTab, "员工管理"});
    if (SessionManager::instance()->hasPermission("request_profile_change") || SessionManager::instance()->hasPermission("approve_profile_change"))
        empTabs.append({m_profileTab, "信息变更"});
    QWidget *empPage = makeDynamicWrapper(empTabs);

    // 3. Attendance Page Wrapper
    QList<QPair<QWidget*, QString>> attTabs;
    if (SessionManager::instance()->hasPermission("apply_leave_makeup") || SessionManager::instance()->hasPermission("approve_makeup"))
        attTabs.append({m_attTaxTab, "打卡补卡"});
    if (SessionManager::instance()->hasPermission("apply_leave") || SessionManager::instance()->hasPermission("approve_leave"))
        attTabs.append({m_leaveTab, "请假审批"});
    QWidget *attPage = makeDynamicWrapper(attTabs);

    // 4. Compensation Page Wrapper
    QList<QPair<QWidget*, QString>> payTabs;
    if (SessionManager::instance()->hasPermission("view_personal_payroll") || SessionManager::instance()->hasPermission("calculate_payroll"))
        payTabs.append({m_payrollTab, "工资条"});
    if (SessionManager::instance()->hasPermission("view_personal_performance") || SessionManager::instance()->hasPermission("evaluate_performance"))
        payTabs.append({m_perfTab, "绩效评分"});
    if (SessionManager::instance()->hasPermission("manage_tax_config")) {
        taxPanel = new TaxConfigPanel(logFn);
        taxPanel->setObjectName("社保配置");
        payTabs.append({taxPanel, "社保配置"});
    }
    QWidget *payPage = makeDynamicWrapper(payTabs);

    // 5. Org
    QWidget *orgPage = SessionManager::instance()->hasPermission("manage_org") ? m_orgTab : nullptr;

    // 6. Audit
    QWidget *auditPage = SessionManager::instance()->hasPermission("view_audit_logs") ? m_auditTab : nullptr;

    // 7. Rbac
    QWidget *rbacPage = SessionManager::instance()->hasPermission("manage_rbac") ? m_rbacTab : nullptr;

    ui->sidebar->setIconSize(QSize(20, 20));
    addNavItem("🏠", "首页",    homePage,    homePage != nullptr);
    addNavItem("👥", "员工",    empPage,     empPage != nullptr);
    addNavItem("⏰", "考勤",    attPage,     attPage != nullptr);
    addNavItem("💰", "薪酬",    payPage,     payPage != nullptr);
    addNavItem("🏢", "组织",    orgPage,     orgPage != nullptr);
    addNavItem("📜", "日志",    auditPage,   auditPage != nullptr);
    addNavItem("🔑", "权限",    rbacPage,    rbacPage != nullptr);

    connect(ui->sidebar, &QListWidget::currentRowChanged, this, [this](int r) {
        if (r >= 0 && r < ui->stackedWidget->count()) {
            ui->stackedWidget->setCurrentIndex(r);
            refreshActiveTab();
        }
        updateStatusBar();
    });
    ui->sidebar->setCurrentRow(0);

    // 通知铃铛
    m_bellBtn = new QPushButton;
    m_bellBtn->setFlat(true);
    m_bellBtn->setStyleSheet("QPushButton{font-size:16px;border:none;padding:2px 8px;} QPushButton:hover{background:#2a3344;}");
    ui->menubar->setCornerWidget(m_bellBtn, Qt::TopRightCorner);
    connect(m_bellBtn, &QPushButton::clicked, this, &MainWindow::showNotifications);

    m_bellTimer = new QTimer(this);
    m_bellTimer->setInterval(500);
    connect(m_bellTimer, &QTimer::timeout, this, [this]() {
        m_bellFlashState = !m_bellFlashState;
        if (m_bellFlashState) {
            m_bellBtn->setStyleSheet("QPushButton{font-size:16px;border:none;padding:2px 8px;background-color:#ef4444;color:white;border-radius:4px;} QPushButton:hover{background:#dc2626;}");
        } else {
            m_bellBtn->setStyleSheet("QPushButton{font-size:16px;border:none;padding:2px 8px;background-color:transparent;color:#ef4444;border-radius:4px;} QPushButton:hover{background:#2a3344;}");
        }
    });

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(5000); // 5 seconds polling
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::refreshBell);
    m_pollTimer->start();

    refreshBell();

    QMenu *sm = ui->menubar->addMenu("系统");
    sm->addAction("修改密码", this, &MainWindow::on_actionChangePassword_triggered);
    sm->addSeparator();
    sm->addAction("退出登录", this, &MainWindow::on_actionLogout_triggered);

    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_dashboard, &DashboardTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_empTab, &EmployeeTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_orgTab, &OrgTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_reportsTab, &ReportsTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_payrollTab, &PayrollTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, this, []() {
        SessionManager::instance()->reloadPermissions();
    });
    connect(GlobalEvents::instance(), &GlobalEvents::auditRefresh, m_auditTab, &AuditTab::refresh);

    updateStatusBar();
    logAction("用户登录");
    refreshActiveTab();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::addNavItem(const QString &icon, const QString &label, QWidget *page, bool visible)
{
    if (!page || !visible) return;
    auto *item = new QListWidgetItem(icon + "\n" + label);
    item->setTextAlignment(Qt::AlignCenter);
    item->setSizeHint(QSize(110, 60));
    ui->sidebar->addItem(item);
    ui->stackedWidget->addWidget(page);
}

void MainWindow::updateStatusBar()
{
    QString friendlyRole = (m_role == "admin") ? "系统管理员" : ((m_role == "user") ? "普通员工" : m_role);
    ui->statusbar->showMessage(QString("当前用户: %1 | 角色: %2").arg(m_empName, friendlyRole));
}

void MainWindow::refreshActiveTab()
{
    QWidget *currentWidget = ui->stackedWidget->currentWidget();
    if (!currentWidget) return;

    QTabWidget *tabWidget = currentWidget->findChild<QTabWidget*>("subTabWidget");
    QWidget *activePage = currentWidget;
    if (tabWidget) {
        activePage = tabWidget->currentWidget();
    } else {
        // Fallback for single-tab wrapper widgets (no subTabWidget present)
        if (currentWidget->findChild<DashboardTab*>()) activePage = m_dashboard;
        else if (currentWidget->findChild<EmployeeTab*>()) activePage = m_empTab;
        else if (currentWidget->findChild<OrgTab*>()) activePage = m_orgTab;
        else if (currentWidget->findChild<ReportsTab*>()) activePage = m_reportsTab;
        else if (currentWidget->findChild<PayrollTab*>()) activePage = m_payrollTab;
        else if (currentWidget->findChild<AuditTab*>()) activePage = m_auditTab;
        else if (currentWidget->findChild<ProfileChangeTab*>()) activePage = m_profileTab;
        else if (currentWidget->findChild<AttendTaxTab*>()) activePage = m_attTaxTab;
        else if (currentWidget->findChild<LeaveTab*>()) activePage = m_leaveTab;
        else if (currentWidget->findChild<PerformanceTab*>()) activePage = m_perfTab;
        else if (currentWidget->findChild<RbacTab*>()) activePage = m_rbacTab;
    }

    if (!activePage) return;

    if (activePage == m_dashboard) m_dashboard->refresh();
    else if (activePage == m_empTab) m_empTab->refresh();
    else if (activePage == m_orgTab) m_orgTab->refresh();
    else if (activePage == m_reportsTab) m_reportsTab->refresh();
    else if (activePage == m_payrollTab) m_payrollTab->refresh();
    else if (activePage == m_auditTab) m_auditTab->refresh();
    else if (activePage == m_profileTab) m_profileTab->refresh();
    else if (activePage == m_attTaxTab) m_attTaxTab->refresh();
    else if (activePage == m_leaveTab) m_leaveTab->refresh();
    else if (activePage == m_perfTab) m_perfTab->refresh();
}

void MainWindow::logAction(const QString &action, const QString &target)
{
    QSqlQuery q; q.prepare("INSERT INTO audit_logs(emp_id,emp_name,action,target,log_time) VALUES(?,?,?,?,NOW())");
    q.addBindValue(m_empId); q.addBindValue(m_empName); q.addBindValue(action); q.addBindValue(target); q.exec();
    emit GlobalEvents::instance()->auditRefresh();
}

void MainWindow::notifyUser(int empId, const QString &title, const QString &content)
{
    if (empId == 0) {
        notifyAdmins(title, content);
        return;
    }
    QSqlQuery q; q.prepare("INSERT INTO notifications(emp_id,title,content) VALUES(?,?,?)");
    q.addBindValue(empId); q.addBindValue(title); q.addBindValue(content); q.exec();
    if (empId == m_empId) refreshBell();
}

void MainWindow::notifyAdmins(const QString &title, const QString &content)
{
    QSqlQuery q("SELECT emp_id FROM employees WHERE role='admin'");
    while (q.next()) notifyUser(q.value(0).toInt(), title, content);
}

void MainWindow::refreshBell()
{
    QSqlQuery q; q.prepare("SELECT COUNT(*) FROM notifications WHERE emp_id=? AND is_read=0");
    q.addBindValue(m_empId); q.exec();
    int count = (q.next() ? q.value(0).toInt() : 0);
    if (count > 0) {
        m_bellBtn->setText(QString("🔔 %1").arg(count));
        if (!m_bellTimer->isActive()) {
            m_bellFlashState = false;
            m_bellTimer->start();
        }
    } else {
        m_bellBtn->setText("🔕");
        if (m_bellTimer->isActive()) {
            m_bellTimer->stop();
        }
        m_bellBtn->setStyleSheet("QPushButton{font-size:16px;border:none;padding:2px 8px;} QPushButton:hover{background:#2a3344;}");
    }
}

void MainWindow::showNotifications()
{
    QMenu menu;
    QSqlQuery q;
    q.prepare("SELECT notif_id, title, content, created_at, is_read FROM notifications WHERE emp_id=? ORDER BY created_at DESC LIMIT 10");
    q.addBindValue(m_empId);
    q.exec();
    
    QList<QAction*> notifActions;
    while (q.next()) {
        int notifId = q.value(0).toInt();
        QString title = q.value(1).toString();
        QString content = q.value(2).toString();
        QString timeStr = q.value(3).toDateTime().toString("MM-dd HH:mm");
        int isRead = q.value(4).toInt();
        
        QString text = QString("[%1] %2: %3").arg(timeStr, title, content);
        if (isRead) {
            text = "✓ " + text;
        } else {
            text = "✉ " + text;
        }
        
        QAction *act = menu.addAction(text);
        act->setData(notifId);
        notifActions.append(act);
    }
    
    if (notifActions.isEmpty()) {
        QAction *act = menu.addAction("暂无通知");
        act->setEnabled(false);
    }
    
    menu.addSeparator();
    QAction *markAllRead = menu.addAction("全部标记已读");
    QAction *clearRead = menu.addAction("清除已读通知");
    
    QAction *triggered = menu.exec(QCursor::pos());
    if (!triggered) return;
    
    if (triggered == markAllRead) {
        QSqlQuery u;
        u.prepare("UPDATE notifications SET is_read=1 WHERE emp_id=?");
        u.addBindValue(m_empId);
        u.exec();
        refreshBell();
    } else if (triggered == clearRead) {
        QSqlQuery u;
        u.prepare("DELETE FROM notifications WHERE emp_id=? AND is_read=1");
        u.addBindValue(m_empId);
        u.exec();
        refreshBell();
    } else if (notifActions.contains(triggered)) {
        int notifId = triggered->data().toInt();
        QSqlQuery u;
        u.prepare("UPDATE notifications SET is_read=1 WHERE notif_id=?");
        u.addBindValue(notifId);
        u.exec();
        refreshBell();
        
        // Show details in QMessageBox
        QMessageBox::information(this, "通知详情", triggered->text());
    }
}

void MainWindow::on_actionChangePassword_triggered()
{
    ChangePasswordDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QSqlQuery q; q.prepare("SELECT password_hash FROM employees WHERE emp_id=?"); q.addBindValue(m_empId); q.exec();
    if (q.next()) {
        QString oh = QString(QCryptographicHash::hash(dlg.oldPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
        if (q.value("password_hash").toString() != oh) { QMessageBox::warning(this,"失败","旧密码错误！"); return; }
    }
    QString nh = QString(QCryptographicHash::hash(dlg.newPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
    q.prepare("UPDATE employees SET password_hash=? WHERE emp_id=?"); q.addBindValue(nh); q.addBindValue(m_empId);
    if (q.exec()) { QMessageBox::information(this,"成功","密码修改成功！"); logAction("修改密码"); }
    else QMessageBox::critical(this,"失败","密码更新失败: "+q.lastError().text());
}

void MainWindow::on_actionLogout_triggered()
{
    logAction("退出登录");

    // Disable auto-login on logout
    QString exeDir = QCoreApplication::applicationDirPath();
    QStringList paths = {
        exeDir + "/config.ini",
        exeDir + "/../../config.ini",
        exeDir + "/../config.ini",
        "config.ini"
    };
    QString configPath = paths.first();
    for (const auto &p : paths) {
        if (QFileInfo::exists(p)) {
            configPath = p;
            break;
        }
    }
    QSettings settings(configPath, QSettings::IniFormat);
    settings.setValue("AutoLogin/Enable", false);

    auto *w = new LoginWindow;
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    deleteLater();
}
