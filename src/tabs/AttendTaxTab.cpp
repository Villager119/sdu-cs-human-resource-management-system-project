#include "AttendTaxTab.h"
#include "../core/GlobalEvents.h"
#include "../core/Constants.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QDateEdit>
#include <QTimeEdit>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

AttendTaxTab::AttendTaxTab(int empId, const QString &role,
                           std::function<void(const QString&, const QString&)> logFn,
                           std::function<void(int, const QString&, const QString&)> notifyFn,
                           QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    initTables();

    m_viewTabs = new QTabWidget;
    m_viewTabs->addTab(createAttendancePanel(), "打卡考勤");
    m_viewTabs->addTab(createMakeupPanel(), "补卡申请");
    connect(m_viewTabs, &QTabWidget::currentChanged, this, &AttendTaxTab::switchView);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_viewTabs);
}

void AttendTaxTab::initTables()
{
    QSqlQuery q;
    // 排班表
    q.exec("CREATE TABLE IF NOT EXISTS shifts ("
           "shift_id INT PRIMARY KEY AUTO_INCREMENT,"
           "shift_name VARCHAR(30) NOT NULL,"
           "start_time TIME NOT NULL,"
           "end_time TIME NOT NULL"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("INSERT IGNORE INTO shifts(shift_name,start_time,end_time) VALUES('标准班','09:00:00','18:00:00')");

    // 考勤表
    q.exec("CREATE TABLE IF NOT EXISTS attendances ("
           "att_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL,"
           "att_date DATE NOT NULL,"
           "clock_in TIME,"
           "clock_out TIME,"
           "status VARCHAR(20) DEFAULT '正常',"
           "remark TEXT,"
           "FOREIGN KEY (emp_id) REFERENCES employees(emp_id),"
           "UNIQUE KEY uk_emp_date (emp_id, att_date)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    // 补卡表
    q.exec("CREATE TABLE IF NOT EXISTS makeup_requests ("
           "makeup_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL,"
           "att_date DATE NOT NULL,"
           "request_type VARCHAR(10) NOT NULL,"
           "request_time TIME NOT NULL,"
           "reason TEXT NOT NULL,"
           "status VARCHAR(20) DEFAULT '待审批',"
           "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    // 社保配置表
    q.exec("CREATE TABLE IF NOT EXISTS salary_config ("
           "config_id INT PRIMARY KEY AUTO_INCREMENT,"
           "item_name VARCHAR(50) NOT NULL UNIQUE,"
           "rate_personal DECIMAL(5,4) NOT NULL DEFAULT 0.0000,"
           "enabled TINYINT DEFAULT 1"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("INSERT IGNORE INTO salary_config(item_name,rate_personal) VALUES "
           "('养老保险',0.0800),('医疗保险',0.0200),('失业保险',0.0050),"
           "('工伤保险',0.0000),('生育保险',0.0000),('住房公积金',0.1200)");
}

QWidget *AttendTaxTab::createAttendancePanel()
{
    m_attModel = new QSqlRelationalTableModel(this);
    m_attModel->setTable("attendances");
    m_attModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_attModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_attModel->setHeaderData(1, Qt::Horizontal, "员工");
    m_attModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_attModel->setHeaderData(3, Qt::Horizontal, "签到");
    m_attModel->setHeaderData(4, Qt::Horizontal, "签退");
    m_attModel->setHeaderData(5, Qt::Horizontal, "状态");
    m_attModel->setHeaderData(6, Qt::Horizontal, "备注");
    if (!SessionManager::instance()->hasPermission("approve_makeup"))
        m_attModel->setFilter(QString("attendances.emp_id=%1").arg(m_empId));

    m_attTable = new QTableView;
    m_attTable->setModel(m_attModel);
    m_attTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 筛选布局
    auto *filterLayout = new QHBoxLayout;
    m_chkDateFilter = new QCheckBox("启用日期筛选", this);
    m_attDateFilter = new QDateEdit(QDate::currentDate(), this);
    m_attDateFilter->setCalendarPopup(true);
    m_attDateFilter->setEnabled(false);
    connect(m_chkDateFilter, &QCheckBox::toggled, m_attDateFilter, &QDateEdit::setEnabled);

    filterLayout->addWidget(m_chkDateFilter);
    filterLayout->addWidget(m_attDateFilter);

    if (SessionManager::instance()->hasPermission("approve_makeup")) {
        m_attNameFilter = new QLineEdit(this);
        m_attNameFilter->setPlaceholderText("搜索员工姓名...");
        m_attNameFilter->setFixedWidth(120);
        filterLayout->addWidget(new QLabel("姓名:", this));
        filterLayout->addWidget(m_attNameFilter);
    } else {
        m_attNameFilter = nullptr;
    }

    m_attStatusCombo = new QComboBox(this);
    m_attStatusCombo->addItems({
        "全部状态",
        HR::AttStatus::NORMAL,
        HR::AttStatus::LATE,
        HR::AttStatus::EARLY,
        HR::AttStatus::MISSING,
        HR::AttStatus::LEAVE
    });
    filterLayout->addWidget(new QLabel("状态:", this));
    filterLayout->addWidget(m_attStatusCombo);

    m_btnSearchAtt = new QPushButton("查询", this);
    m_btnResetAtt = new QPushButton("重置", this);
    filterLayout->addWidget(m_btnSearchAtt);
    filterLayout->addWidget(m_btnResetAtt);
    filterLayout->addStretch();

    connect(m_btnSearchAtt, &QPushButton::clicked, this, &AttendTaxTab::filterAttendance);
    connect(m_btnResetAtt, &QPushButton::clicked, this, [this]() {
        m_chkDateFilter->setChecked(false);
        m_attDateFilter->setDate(QDate::currentDate());
        if (m_attNameFilter) m_attNameFilter->clear();
        m_attStatusCombo->setCurrentIndex(0);
        filterAttendance();
    });

    auto *w = new QWidget;
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addLayout(filterLayout);
    l->addWidget(m_attTable, 1);

    auto *btnWidget = new QWidget;
    auto *row = new QHBoxLayout(btnWidget);
    row->setContentsMargins(0, 0, 0, 0);
    row->addStretch();
    auto *btnIn = new QPushButton("上班打卡");
    auto *btnOut = new QPushButton("下班打卡");
    row->addWidget(btnIn);
    row->addWidget(btnOut);
    row->addStretch();
    l->addWidget(btnWidget);

    if (!SessionManager::instance()->hasPermission("apply_leave_makeup")) {
        btnWidget->setVisible(false);
    }

    connect(btnIn, &QPushButton::clicked, this, &AttendTaxTab::clockIn);
    connect(btnOut, &QPushButton::clicked, this, &AttendTaxTab::clockOut);

    m_attModel->select();
    return w;
}

QWidget *AttendTaxTab::createMakeupPanel()
{
    m_makeupModel = new QSqlRelationalTableModel(this);
    m_makeupModel->setTable("makeup_requests");
    m_makeupModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_makeupModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_makeupModel->setHeaderData(1, Qt::Horizontal, "员工");
    m_makeupModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_makeupModel->setHeaderData(3, Qt::Horizontal, "类型");
    m_makeupModel->setHeaderData(4, Qt::Horizontal, "时间");
    m_makeupModel->setHeaderData(5, Qt::Horizontal, "理由");
    m_makeupModel->setHeaderData(6, Qt::Horizontal, "状态");
    if (!SessionManager::instance()->hasPermission("approve_makeup"))
        m_makeupModel->setFilter(QString("emp_id=%1").arg(m_empId));

    m_makeupTable = new QTableView;
    m_makeupTable->setModel(m_makeupModel);
    m_makeupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_makeupTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *w = new QWidget;
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_makeupTable, 1);

    auto *panel = new QGroupBox("补卡申请");
    auto *form = new QFormLayout(panel);
    m_makeupDate = new QDateEdit(QDate::currentDate());
    form->addRow("日期:", m_makeupDate);
    m_makeupType = new QComboBox;
    m_makeupType->addItem("上班签到", "clock_in");
    m_makeupType->addItem("下班签退", "clock_out");
    form->addRow("类型:", m_makeupType);
    m_makeupTime = new QTimeEdit(QTime::currentTime());
    form->addRow("时间:", m_makeupTime);
    m_makeupReason = new QLineEdit;
    m_makeupReason->setPlaceholderText("补卡事由");
    form->addRow("理由:", m_makeupReason);
    auto *btn = new QPushButton("提交补卡");
    btn->setStyleSheet("QPushButton { background: #1e3a5f; color: #fff; } QPushButton:hover { background: #2a5080; }");
    form->addRow(btn);
    connect(btn, &QPushButton::clicked, this, &AttendTaxTab::submitMakeup);

    auto *approveRow = new QHBoxLayout;
    m_btnApproveMakeup = new QPushButton("同意补卡");
    m_btnRejectMakeup = new QPushButton("拒绝");
    approveRow->addStretch();
    approveRow->addWidget(m_btnApproveMakeup);
    approveRow->addWidget(m_btnRejectMakeup);
    if (!SessionManager::instance()->hasPermission("approve_makeup")) {
        m_btnApproveMakeup->setVisible(false);
        m_btnRejectMakeup->setVisible(false);
    }

    m_btnApproveMakeup->setEnabled(false);
    m_btnRejectMakeup->setEnabled(false);

    connect(m_makeupTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto selected = m_makeupTable->selectionModel()->selectedRows();
        if (selected.size() == 1) {
            int row = selected.first().row();
            QString status = m_makeupModel->data(m_makeupModel->index(row, 6)).toString();
            bool isPending = (status == "待审批");
            m_btnApproveMakeup->setEnabled(isPending);
            m_btnRejectMakeup->setEnabled(isPending);
        } else {
            m_btnApproveMakeup->setEnabled(false);
            m_btnRejectMakeup->setEnabled(false);
        }
    });

    l->addWidget(panel);
    l->addLayout(approveRow);

    if (!SessionManager::instance()->hasPermission("apply_leave_makeup")) {
        panel->setVisible(false);
    }
    connect(m_btnApproveMakeup, &QPushButton::clicked, this, &AttendTaxTab::approveMakeup);
    connect(m_btnRejectMakeup, &QPushButton::clicked, this, &AttendTaxTab::rejectMakeup);

    m_makeupModel->select();
    return w;
}

void AttendTaxTab::switchView(int)
{
    if (m_attModel) m_attModel->select();
    if (m_makeupModel) m_makeupModel->select();
}

// 打卡
void AttendTaxTab::clockIn()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    QSqlQuery q;
    q.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
    q.addBindValue(m_empId); q.addBindValue(today); q.exec();
    if (q.next()) {
        QMessageBox::warning(this, "提示", "今天已打卡，请勿重复");
        return;
    }

    // 检查迟到
    QSqlQuery sq("SELECT start_time FROM shifts WHERE shift_id=1");
    sq.exec();
    QString start = sq.next() ? sq.value(0).toString() : "09:00:00";
    QString status = (now > start) ? "迟到" : "正常";

    q.prepare("INSERT INTO attendances(emp_id,att_date,clock_in,status) VALUES(?,?,?,?)");
    q.addBindValue(m_empId); q.addBindValue(today); q.addBindValue(now); q.addBindValue(status);
    if (q.exec()) {
        QMessageBox::information(this, "打卡成功", QString("签到 %1  %2").arg(today, now));
        m_log("上班打卡", today);
        m_attModel->select();
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
}

void AttendTaxTab::clockOut()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    QSqlQuery q;
    q.prepare("SELECT att_id, status FROM attendances WHERE emp_id=? AND att_date=?");
    q.addBindValue(m_empId); q.addBindValue(today); q.exec();
    if (!q.next()) { QMessageBox::warning(this, "提示", "请先打上班卡"); return; }

    QSqlQuery sq("SELECT end_time FROM shifts WHERE shift_id=1");
    sq.exec();
    QString end = sq.next() ? sq.value(0).toString() : "18:00:00";
    QString status = (now < end) ? "早退" : "正常";
    if (q.value(1).toString() == "迟到") status = "迟到";
    if (status == "正常" && q.value(1).toString() == "正常") status = "正常";
    else if (status == "早退") status = "早退";

    q.prepare("UPDATE attendances SET clock_out=?, status=? WHERE emp_id=? AND att_date=?");
    q.addBindValue(now); q.addBindValue(status); q.addBindValue(m_empId); q.addBindValue(today);
    if (q.exec()) {
        QMessageBox::information(this, "打卡成功", QString("签退 %1  %2  [%3]").arg(today, now, status));
        m_log("下班打卡", today);
        m_attModel->select();
    }
}

// 补卡
void AttendTaxTab::submitMakeup()
{
    QString date = m_makeupDate->date().toString("yyyy-MM-dd");
    QString type = m_makeupType->currentData().toString();
    QString time = m_makeupTime->time().toString("HH:mm:ss");
    QString reason = m_makeupReason->text().trimmed();
    if (reason.isEmpty()) { QMessageBox::warning(this, "提示", "请填写理由"); return; }

    QSqlQuery q;
    q.prepare("INSERT INTO makeup_requests(emp_id,att_date,request_type,request_time,reason) VALUES(?,?,?,?,?)");
    q.addBindValue(m_empId); q.addBindValue(date); q.addBindValue(type); q.addBindValue(time); q.addBindValue(reason);
    if (q.exec()) {
        QMessageBox::information(this, "成功", "补卡申请已提交");
        m_log("补卡申请", date);
        m_notify(0, "补卡申请", QString("员工提交了补卡申请 %1").arg(date));
        m_makeupModel->select();
        m_makeupReason->clear();
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
}

void AttendTaxTab::approveMakeup()
{
    int row = m_makeupTable->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请选中一条补卡申请"); return; }
    int mid = m_makeupModel->data(m_makeupModel->index(row, 0)).toInt();
    int eid = 0;
    {
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM makeup_requests WHERE makeup_id=?");
        eq.addBindValue(mid); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }
    QString date = m_makeupModel->data(m_makeupModel->index(row, 2)).toString();
    QString type = m_makeupModel->data(m_makeupModel->index(row, 3)).toString();
    QString time = m_makeupModel->data(m_makeupModel->index(row, 4)).toString();

    if (type != "clock_in" && type != "clock_out") {
        QMessageBox::warning(this, "错误", "无效的补卡类型");
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
    q.addBindValue(eid); q.addBindValue(date); q.exec();
    if (q.next()) {
        q.prepare(QString("UPDATE attendances SET %1=?, status='正常' WHERE emp_id=? AND att_date=?").arg(type));
        q.addBindValue(time); q.addBindValue(eid); q.addBindValue(date);
    } else {
        q.prepare(QString("INSERT INTO attendances(emp_id,att_date,%1,status) VALUES(?,?,?,'正常')").arg(type));
        q.addBindValue(eid); q.addBindValue(date); q.addBindValue(time);
    }
    q.exec();
    q.prepare("UPDATE makeup_requests SET status='已同意' WHERE makeup_id=?");
    q.addBindValue(mid); q.exec();
    m_log("同意补卡", date);
    m_notify(eid, "补卡已批准", QString("你%1的补卡申请已通过").arg(date));
    m_makeupModel->select();
    m_attModel->select();
    GlobalEvents::instance()->dataChanged();
    QMessageBox::information(this, "成功", "补卡已批准");
}

void AttendTaxTab::rejectMakeup()
{
    int row = m_makeupTable->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请选中一条补卡申请"); return; }
    int mid = m_makeupModel->data(m_makeupModel->index(row, 0)).toInt();
    QSqlQuery q;
    q.prepare("UPDATE makeup_requests SET status='已拒绝' WHERE makeup_id=?");
    q.addBindValue(mid);
    if (q.exec()) {
        int eid = 0;
        {
            QSqlQuery eq; eq.prepare("SELECT emp_id FROM makeup_requests WHERE makeup_id=?");
            eq.addBindValue(mid); eq.exec();
            if (eq.next()) eid = eq.value(0).toInt();
        }
        m_log("拒绝补卡", QString::number(mid));
        m_notify(eid, "补卡已拒绝", "你的补卡申请未通过审批");
        m_makeupModel->select();
        GlobalEvents::instance()->dataChanged();
        QMessageBox::information(this, "成功", "已拒绝");
    }
}

void AttendTaxTab::filterAttendance()
{
    QStringList conds;

    // 角色基础过滤
    if (!SessionManager::instance()->hasPermission("approve_makeup")) {
        conds << QString("attendances.emp_id = %1").arg(m_empId);
    } else {
        if (m_attNameFilter && !m_attNameFilter->text().trimmed().isEmpty()) {
            QString namePattern = m_attNameFilter->text().trimmed();
            namePattern.replace("'", "''");
            conds << QString("attendances.emp_id IN (SELECT emp_id FROM employees WHERE name LIKE '%%1%')").arg(namePattern);
        }
    }

    // 日期过滤
    if (m_chkDateFilter->isChecked()) {
        QString dateStr = m_attDateFilter->date().toString("yyyy-MM-dd");
        conds << QString("attendances.att_date = '%1'").arg(dateStr);
    }

    // 状态过滤
    if (m_attStatusCombo->currentIndex() > 0) {
        QString status = m_attStatusCombo->currentText();
        status.replace("'", "''");
        conds << QString("attendances.status = '%1'").arg(status);
    }

    m_attModel->setFilter(conds.isEmpty() ? "" : conds.join(" AND "));
    m_attModel->select();
}

void AttendTaxTab::refresh()
{
    m_attModel->select();
    m_makeupModel->select();
}



