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

    // 全局样式 — 精炼企业蓝灰调
    setStyleSheet(R"(
        QMainWindow { background: #f0f2f5; }
        QTabWidget::pane { border: none; background: #fff; border-radius: 6px; margin: 2px 0 0 0; }
        QTabBar { alignment: left; }
        QTabBar::tab { padding: 9px 24px; margin-right: 0px; border: none; border-bottom: 3px solid transparent; background: transparent; font-size: 13px; color: #6b7280; }
        QTabBar::tab:selected { color: #1e3a5f; font-weight: 600; border-bottom: 3px solid #1e3a5f; }
        QTabBar::tab:hover { color: #1e3a5f; background: rgba(30,58,95,0.04); }
        QTableView { gridline-color: #f1f3f5; alternate-background-color: #f8f9fb; selection-background-color: #e8ecf1; selection-color: #1e3a5f; border: 1px solid #e2e5ea; border-radius: 4px; }
        QTableView::item { padding: 7px 10px; }
        QTableView::item:hover { background: #f1f4f8; }
        QHeaderView::section { background: #f5f6f8; border: none; border-bottom: 2px solid #e2e5ea; padding: 8px 10px; font-weight: 600; font-size: 12px; color: #5f6b7a; }
        QPushButton { padding: 6px 16px; border: 1px solid #d5d9df; border-radius: 5px; background: #fff; color: #374151; font-size: 12px; }
        QPushButton:hover { border-color: #1e3a5f; color: #1e3a5f; background: #f8f9fb; }
        QPushButton:pressed { background: #e8ecf1; }
        QComboBox { padding: 6px 10px; border: 1px solid #d5d9df; border-radius: 5px; background: #fff; color: #374151; }
        QComboBox:hover { border-color: #1e3a5f; }
        QComboBox::drop-down { border: none; }
        QLineEdit { padding: 6px 10px; border: 1px solid #d5d9df; border-radius: 5px; background: #fff; color: #374151; }
        QLineEdit:focus { border-color: #1e3a5f; }
        QDateEdit { padding: 6px 10px; border: 1px solid #d5d9df; border-radius: 5px; background: #fff; color: #374151; }
        QDateEdit:focus { border-color: #1e3a5f; }
        QMenuBar { background: #fff; border-bottom: 1px solid #e8eaed; padding: 2px 0; }
        QMenuBar::item { padding: 5px 14px; color: #4b5563; }
        QMenuBar::item:selected { background: #f1f4f8; color: #1e3a5f; border-radius: 4px; }
        QMenu { background: #fff; border: 1px solid #e2e5ea; padding: 4px; }
        QMenu::item { padding: 6px 28px; border-radius: 4px; }
        QMenu::item:selected { background: #f1f4f8; }
        QStatusBar { background: #fff; border-top: 1px solid #e8eaed; color: #8893a4; font-size: 12px; padding: 2px 8px; }
        QFrame#loginCard { background: #fff; border: 1px solid #e2e5ea; border-radius: 10px; }
    )");

    // 查询用户姓名
    {
        QSqlQuery q;
        q.prepare("SELECT name FROM employees WHERE emp_id=?");
        q.addBindValue(m_empId);
        if (q.exec() && q.next()) m_empName = q.value(0).toString();
    }

    auto logFn = [this](const QString &action, const QString &target) { logAction(action, target); };

    // 创建所有 Tab
    m_dashboard = new DashboardTab;
    m_empTab = new EmployeeTab(logFn);
    m_leaveTab = new LeaveTab(m_empId, m_role, logFn);
    m_payrollTab = new PayrollTab(m_empId, m_role, logFn);
    m_auditTab = new AuditTab;
    m_reportsTab = new ReportsTab;

    ui->tabWidget->addTab(m_dashboard, "首页");
    ui->tabWidget->addTab(m_empTab, "员工信息管理");
    ui->tabWidget->addTab(m_leaveTab, "考勤与请假审批");
    ui->tabWidget->addTab(m_payrollTab, "薪酬管理");
    ui->tabWidget->addTab(m_auditTab, "操作日志");
    ui->tabWidget->addTab(m_reportsTab, "统计报表");

    // RBAC 权限
    if (m_role == "user") {
        ui->tabWidget->setTabVisible(0, false);  // 仪表盘
        ui->tabWidget->setTabVisible(1, false);  // 员工管理
        ui->tabWidget->setTabVisible(4, false);  // 操作日志
        ui->tabWidget->setTabVisible(5, false);  // 统计报表
    }

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
