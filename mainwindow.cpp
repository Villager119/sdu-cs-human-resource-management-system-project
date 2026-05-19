#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "changepassworddialog.h"
#include "loginwindow.h"
#include "tabs/EmployeeTab.h"
#include "tabs/LeaveTab.h"
#include "tabs/PayrollTab.h"
#include "tabs/AuditTab.h"
#include "tabs/ReportsTab.h"
#include "tabs/DashboardTab.h"
#include "tabs/PerformanceTab.h"
#include "tabs/OrgTab.h"
#include "tabs/ProfileChangeTab.h"
#include "tabs/AttendTaxTab.h"
#include <QMenu>
#include <QMessageBox>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>

MainWindow::MainWindow(int empId, QString role, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_empId(empId)
    , m_role(role)
{
    ui->setupUi(this);
    setWindowTitle("HRMS-员工信息管理");

    // 查询用户姓名
    {
        QSqlQuery q;
        q.prepare("SELECT name FROM employees WHERE emp_id=?");
        q.addBindValue(m_empId);
        if (q.exec() && q.next()) m_empName = q.value(0).toString();
    }

    auto logFn = [this](const QString &action, const QString &target) { logAction(action, target); };
    auto notifyFn = [this](int toEmpId, const QString &title, const QString &content) {
        if (toEmpId == 0) notifyAdmins(title, content);
        else notifyUser(toEmpId, title, content);
    };

    // 创建所有 Tab
    m_dashboard = new DashboardTab;
    m_empTab = new EmployeeTab(logFn);
    m_leaveTab = new LeaveTab(m_empId, m_role, logFn, notifyFn);
    m_payrollTab = new PayrollTab(m_empId, m_role, logFn);
    m_auditTab = new AuditTab;
    m_reportsTab = new ReportsTab;
    m_perfTab = new PerformanceTab(m_empId, m_role, logFn);
    m_orgTab = new OrgTab(logFn);
    m_profileTab = new ProfileChangeTab(m_empId, m_role, logFn, notifyFn);
    m_attTaxTab = new AttendTaxTab(m_empId, m_role, logFn, notifyFn);

    ui->tabWidget->addTab(m_dashboard, "首页");
    ui->tabWidget->addTab(m_empTab, "员工信息管理");
    ui->tabWidget->addTab(m_leaveTab, "考勤与请假审批");
    ui->tabWidget->addTab(m_payrollTab, "薪酬管理");
    ui->tabWidget->addTab(m_auditTab, "操作日志");
    ui->tabWidget->addTab(m_profileTab, "信息变更");
    ui->tabWidget->addTab(m_attTaxTab, "考勤排班");
    ui->tabWidget->addTab(m_orgTab, "组织架构");
    ui->tabWidget->addTab(m_perfTab, "绩效管理");
    ui->tabWidget->addTab(m_reportsTab, "统计报表");

    // RBAC 权限
    if (m_role == "user") {
        ui->tabWidget->setTabVisible(0, false);  // 仪表盘
        ui->tabWidget->setTabVisible(1, false);  // 员工管理
        ui->tabWidget->setTabVisible(4, false);  // 操作日志
        ui->tabWidget->setTabVisible(7, false);  // 组织架构
        ui->tabWidget->setTabVisible(8, false);  // 绩效管理
        ui->tabWidget->setTabVisible(9, false);  // 统计报表
    }

    // 通知铃铛
    {
        QSqlQuery q;
        q.exec("CREATE TABLE IF NOT EXISTS notifications ("
               "notif_id INT PRIMARY KEY AUTO_INCREMENT,"
               "emp_id INT NOT NULL,"
               "title VARCHAR(100) NOT NULL,"
               "content VARCHAR(200),"
               "is_read TINYINT DEFAULT 0,"
               "created_at DATETIME DEFAULT NOW(),"
               "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    }
    m_bellBtn = new QPushButton;
    m_bellBtn->setFlat(true);
    m_bellBtn->setStyleSheet("QPushButton { font-size: 16px; border: none; } QPushButton:hover { background: #f0f0f0; }");
    ui->menubar->setCornerWidget(m_bellBtn, Qt::TopRightCorner);
    connect(m_bellBtn, &QPushButton::clicked, this, &MainWindow::showNotifications);
    refreshBell();

    // 系统菜单
    QMenu *sysMenu = ui->menubar->addMenu("系统");
    sysMenu->addAction("修改密码", this, &MainWindow::on_actionChangePassword_triggered);
    sysMenu->addSeparator();
    sysMenu->addAction("退出登录", this, &MainWindow::on_actionLogout_triggered);

    // 状态栏
    QString roleText = (m_role == "admin") ? "管理员" : "普通员工";
    ui->statusbar->showMessage(QString("当前用户: %1 | %2").arg(m_empName, roleText));
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 0) m_dashboard->refresh();
        QString tab = ui->tabWidget->tabText(idx);
        QString info;
        if (idx == 1) info = QString(" | 共 %1 条").arg(m_empTab->findChild<QTableView *>()->model()->rowCount());
        ui->statusbar->showMessage(QString("当前用户: %1 | %2 | %3%4")
            .arg(m_empName, (m_role == "admin") ? "管理员" : "普通员工", tab, info));
    });

    // 登录日志
    logAction("用户登录");
    m_dashboard->refresh();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::logAction(const QString &action, const QString &target)
{
    QSqlQuery q;
    q.prepare("INSERT INTO audit_logs(emp_id, emp_name, action, target, log_time) VALUES(?,?,?,?,NOW())");
    q.addBindValue(m_empId);
    q.addBindValue(m_empName);
    q.addBindValue(action);
    q.addBindValue(target);
    q.exec();
}

void MainWindow::on_actionChangePassword_triggered()
{
    ChangePasswordDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QSqlQuery q;
    q.prepare("SELECT password_hash FROM employees WHERE emp_id=?");
    q.addBindValue(m_empId);
    q.exec();
    if (q.next()) {
        QString oldHash = QString(QCryptographicHash::hash(dlg.oldPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
        if (q.value("password_hash").toString() != oldHash) {
            QMessageBox::warning(this, "修改失败", "旧密码错误！");
            return;
        }
    }
    QString newHash = QString(QCryptographicHash::hash(dlg.newPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
    q.prepare("UPDATE employees SET password_hash=? WHERE emp_id=?");
    q.addBindValue(newHash); q.addBindValue(m_empId);
    if (q.exec()) {
        QMessageBox::information(this, "成功", "密码修改成功！");
        logAction("修改密码");
    } else {
        QMessageBox::critical(this, "失败", "密码更新失败: " + q.lastError().text());
    }
}

void MainWindow::on_actionLogout_triggered()
{
    logAction("退出登录");
    auto *w = new LoginWindow;
    w->show();
    close();
}

void MainWindow::notifyUser(int empId, const QString &title, const QString &content)
{
    QSqlQuery q;
    q.prepare("INSERT INTO notifications(emp_id,title,content) VALUES(?,?,?)");
    q.addBindValue(empId); q.addBindValue(title); q.addBindValue(content); q.exec();
    if (empId == m_empId) refreshBell();
}

void MainWindow::notifyAdmins(const QString &title, const QString &content)
{
    QSqlQuery q("SELECT emp_id FROM employees WHERE role='admin'");
    while (q.next())
        notifyUser(q.value(0).toInt(), title, content);
}

void MainWindow::refreshBell()
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM notifications WHERE emp_id=? AND is_read=0");
    q.addBindValue(m_empId); q.exec();
    int n = q.next() ? q.value(0).toInt() : 0;
    m_bellBtn->setText(n > 0 ? QString("🔔 %1").arg(n) : "🔕");
}

void MainWindow::showNotifications()
{
    QMenu menu;
    QSqlQuery q;
    q.prepare("SELECT title, content, created_at FROM notifications WHERE emp_id=? ORDER BY created_at DESC LIMIT 5");
    q.addBindValue(m_empId); q.exec();
    bool has = false;
    while (q.next()) {
        menu.addAction(QString("[%1] %2").arg(q.value(2).toString(), q.value(0).toString()));
        has = true;
    }
    if (!has) menu.addAction("暂无通知");
    menu.addSeparator();
    QAction *mark = menu.addAction("全部标记已读");
    menu.exec(QCursor::pos());

    if (menu.activeAction() == mark) {
        QSqlQuery uq;
        uq.prepare("UPDATE notifications SET is_read=1 WHERE emp_id=?");
        uq.addBindValue(m_empId); uq.exec();
        refreshBell();
    }
}
