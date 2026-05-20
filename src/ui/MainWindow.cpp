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
#include "../core/GlobalEvents.h"
#include <QMenu>
#include <QTabWidget>
#include <QMessageBox>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>

MainWindow::MainWindow(int empId, QString role, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_empId(empId), m_role(role)
{
    ui->setupUi(this);
    setWindowTitle("HRMS");

    { QSqlQuery q; q.prepare("SELECT name FROM employees WHERE emp_id=?"); q.addBindValue(m_empId); if(q.exec()&&q.next()) m_empName=q.value(0).toString(); }

    auto logFn = [this](const QString &a, const QString &t) { logAction(a, t); };
    auto notifyFn = [this](int e, const QString &t, const QString &c) { notifyUser(e, t, c); };

    // 创建所有原生页面
    m_dashboard = new DashboardTab;
    m_empTab    = new EmployeeTab(logFn);
    m_leaveTab  = new LeaveTab(m_empId, m_role, logFn, notifyFn);
    m_payrollTab= new PayrollTab(m_empId, m_role, logFn);
    m_auditTab  = new AuditTab;
    m_orgTab    = new OrgTab(logFn);
    m_perfTab   = new PerformanceTab(m_empId, m_role, logFn);
    m_reportsTab= new ReportsTab;
    m_profileTab= new ProfileChangeTab(m_empId, m_role, logFn, notifyFn);
    m_attTaxTab = new AttendTaxTab(m_empId, m_role, logFn, notifyFn);

    auto makeWrapper = [](QWidget *a, QWidget *b = nullptr, QWidget *c = nullptr) -> QWidget * {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        if (b) {
            auto *tabs = new QTabWidget;
            tabs->addTab(a, a->objectName().isEmpty() ? "Tab1" : a->objectName());
            tabs->addTab(b, b->objectName().isEmpty() ? "Tab2" : b->objectName());
            if (c) tabs->addTab(c, c->objectName().isEmpty() ? "Tab3" : c->objectName());
            l->addWidget(tabs);
        } else {
            l->addWidget(a);
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

    QWidget *homePage    = makeWrapper(m_dashboard, m_reportsTab);
    QWidget *empPage     = makeWrapper(m_empTab, m_profileTab);
    QWidget *attPage     = makeWrapper(m_attTaxTab, m_leaveTab);
    QWidget *payPage     = makeWrapper(m_payrollTab, m_perfTab);

    bool adm = (m_role == "admin");
    ui->sidebar->setIconSize(QSize(20, 20));
    addNavItem("🏠", "首页",    homePage,    adm);
    addNavItem("👥", "员工",    empPage,     adm);
    addNavItem("⏰", "考勤",    attPage,     true);
    addNavItem("💰", "薪酬",    payPage,     true);
    addNavItem("🏢", "组织",    m_orgTab,   adm);
    addNavItem("📜", "日志",    m_auditTab, adm);

    connect(ui->sidebar, &QListWidget::currentRowChanged, this, [this](int r) {
        ui->stackedWidget->setCurrentIndex(r);
        if (r == 0) m_dashboard->refresh();
        updateStatusBar();
    });
    ui->sidebar->setCurrentRow(0);

    // 通知铃铛
    { QSqlQuery q; q.exec("CREATE TABLE IF NOT EXISTS notifications (notif_id INT PRIMARY KEY AUTO_INCREMENT,emp_id INT NOT NULL,title VARCHAR(100) NOT NULL,content VARCHAR(200),is_read TINYINT DEFAULT 0,created_at DATETIME DEFAULT NOW(),FOREIGN KEY(emp_id) REFERENCES employees(emp_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"); }
    m_bellBtn = new QPushButton;
    m_bellBtn->setFlat(true);
    m_bellBtn->setStyleSheet("QPushButton{font-size:16px;border:none;padding:2px 8px;} QPushButton:hover{background:#2a3344;}");
    ui->menubar->setCornerWidget(m_bellBtn, Qt::TopRightCorner);
    connect(m_bellBtn, &QPushButton::clicked, this, &MainWindow::showNotifications);
    refreshBell();

    QMenu *sm = ui->menubar->addMenu("系统");
    sm->addAction("修改密码", this, &MainWindow::on_actionChangePassword_triggered);
    sm->addSeparator();
    sm->addAction("退出登录", this, &MainWindow::on_actionLogout_triggered);

    connect(GlobalEvents::instance(), &GlobalEvents::dataChanged, m_dashboard, &DashboardTab::refresh);
    connect(GlobalEvents::instance(), &GlobalEvents::auditRefresh, m_auditTab, &AuditTab::refresh);

    updateStatusBar();
    logAction("用户登录");
    m_dashboard->refresh();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::addNavItem(const QString &icon, const QString &label, QWidget *page, bool visible)
{
    auto *item = new QListWidgetItem(icon + "\n" + label);
    item->setTextAlignment(Qt::AlignCenter);
    item->setSizeHint(QSize(68, 48));
    if (!visible) item->setHidden(true);
    ui->sidebar->addItem(item);
    ui->stackedWidget->addWidget(page);
}

void MainWindow::updateStatusBar()
{
    ui->statusbar->showMessage(QString("当前用户: %1 | %2").arg(m_empName, (m_role=="admin")?"管理员":"普通员工"));
}

void MainWindow::logAction(const QString &action, const QString &target)
{
    QSqlQuery q; q.prepare("INSERT INTO audit_logs(emp_id,emp_name,action,target,log_time) VALUES(?,?,?,?,NOW())");
    q.addBindValue(m_empId); q.addBindValue(m_empName); q.addBindValue(action); q.addBindValue(target); q.exec();
}

void MainWindow::notifyUser(int empId, const QString &title, const QString &content)
{
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
    m_bellBtn->setText(q.next() && q.value(0).toInt() > 0 ? QString("🔔 %1").arg(q.value(0).toInt()) : "🔕");
}

void MainWindow::showNotifications()
{
    QMenu menu; QSqlQuery q;
    q.prepare("SELECT title,content,created_at FROM notifications WHERE emp_id=? ORDER BY created_at DESC LIMIT 5");
    q.addBindValue(m_empId); q.exec(); bool has=false;
    while (q.next()) { menu.addAction("["+q.value(2).toString()+"] "+q.value(0).toString()); has=true; }
    if (!has) menu.addAction("暂无通知");
    menu.addSeparator();
    QAction *mark = menu.addAction("全部标记已读");
    menu.exec(QCursor::pos());
    if (menu.activeAction() == mark) { QSqlQuery u; u.prepare("UPDATE notifications SET is_read=1 WHERE emp_id=?"); u.addBindValue(m_empId); u.exec(); refreshBell(); }
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
    auto *w = new LoginWindow; w->show(); close();
}
