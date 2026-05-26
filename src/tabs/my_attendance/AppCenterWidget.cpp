#include "AppCenterWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../utils/Toast.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QHeaderView>
#include <QTabWidget>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QDateEdit>
#include <QTimeEdit>

AppCenterWidget::AppCenterWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->setStyleSheet(
        "QTabWidget::panel { border: none; background-color: #ffffff; }"
        "QTabBar::tab { font-size: 13px; padding: 6px 14px; }"
    );

    // Page 1: Leave App
    QWidget *leavePage = new QWidget(tabs);
    auto *leaveLayout = new QHBoxLayout(leavePage);
    leaveLayout->setContentsMargins(10, 10, 10, 10);
    leaveLayout->setSpacing(15);

    QFrame *leaveLeft = new QFrame(leavePage);
    leaveLeft->setObjectName("leaveLeft");
    leaveLeft->setStyleSheet("QFrame#leaveLeft { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }");
    leaveLeft->setFixedWidth(320);
    auto *ll = new QVBoxLayout(leaveLeft);
    ll->setContentsMargins(15, 15, 15, 15);
    ll->setSpacing(12);

    QLabel *lblLeaveTitle = new QLabel("新建请假申请", leaveLeft);
    lblLeaveTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1d4ed8; border: none; background: transparent;");
    ll->addWidget(lblLeaveTitle);

    ll->addWidget(new QLabel("开始日期:", leaveLeft));
    m_leaveStart = new QDateEdit(QDate::currentDate(), leaveLeft);
    m_leaveStart->setCalendarPopup(true);
    ll->addWidget(m_leaveStart);

    ll->addWidget(new QLabel("结束日期:", leaveLeft));
    m_leaveEnd = new QDateEdit(QDate::currentDate(), leaveLeft);
    m_leaveEnd->setCalendarPopup(true);
    ll->addWidget(m_leaveEnd);

    ll->addWidget(new QLabel("请假理由:", leaveLeft));
    m_leaveReason = new QTextEdit(leaveLeft);
    m_leaveReason->setPlaceholderText("请输入请假原因 (必填，最少输入5个字)");
    m_leaveReason->setStyleSheet(
        "QTextEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}"
        "QTextEdit:focus { border: 1px solid #3b82f6; }"
    );
    ll->addWidget(m_leaveReason, 1);

    auto *btnLeaveRow = new QHBoxLayout;
    btnLeaveRow->addStretch();
    QPushButton *btnSubmitLeave = new QPushButton("提交申请", leaveLeft);
    btnSubmitLeave->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    btnLeaveRow->addWidget(btnSubmitLeave);
    ll->addLayout(btnLeaveRow);
    connect(btnSubmitLeave, &QPushButton::clicked, this, &AppCenterWidget::submitLeave);

    leaveLayout->addWidget(leaveLeft);

    QFrame *leaveRight = new QFrame(leavePage);
    leaveRight->setObjectName("leaveRight");
    leaveRight->setStyleSheet("QFrame#leaveRight { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }");
    auto *lr = new QVBoxLayout(leaveRight);
    lr->setContentsMargins(15, 15, 15, 15);
    lr->setSpacing(10);

    QLabel *lblLeaveHistTitle = new QLabel("我的请假历史", leaveRight);
    lblLeaveHistTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    lr->addWidget(lblLeaveHistTitle);

    m_leaveModel = new QSqlRelationalTableModel(this);
    m_leaveModel->setTable("leave_requests");
    m_leaveModel->setHeaderData(2, Qt::Horizontal, "开始日期");
    m_leaveModel->setHeaderData(3, Qt::Horizontal, "结束日期");
    m_leaveModel->setHeaderData(4, Qt::Horizontal, "请假理由");
    m_leaveModel->setHeaderData(5, Qt::Horizontal, "状态");

    m_leaveTable = new QTableView(leaveRight);
    m_leaveTable->setModel(m_leaveModel);
    m_leaveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_leaveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_leaveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_leaveTable->setShowGrid(false);
    m_leaveTable->verticalHeader()->setVisible(false);
    m_leaveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_leaveTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  padding: 8px;"
        "  border: none;"
        "  font-weight: bold;"
        "  border-bottom: 2px solid #e2e8f0;"
        "}"
    );
    m_leaveTable->setStyleSheet(
        "QTableView {"
        "  border: none;"
        "  background-color: #ffffff;"
        "  gridline-color: #f1f5f9;"
        "  selection-background-color: #f1f5f9;"
        "  selection-color: #0f172a;"
        "}"
        "QTableView::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #f1f5f9;"
        "}"
    );
    m_leaveTable->setItemDelegateForColumn(5, new RequestStatusDelegate(m_leaveTable));
    lr->addWidget(m_leaveTable, 1);

    leaveLayout->addWidget(leaveRight, 1);

    tabs->addTab(leavePage, "💬 请假申请");

    // Page 2: Makeup App
    QWidget *makeupPage = new QWidget(tabs);
    auto *makeupLayout = new QHBoxLayout(makeupPage);
    makeupLayout->setContentsMargins(10, 10, 10, 10);
    makeupLayout->setSpacing(15);

    QFrame *makeupLeft = new QFrame(makeupPage);
    makeupLeft->setObjectName("makeupLeft");
    makeupLeft->setStyleSheet("QFrame#makeupLeft { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }");
    makeupLeft->setFixedWidth(320);
    auto *ml = new QVBoxLayout(makeupLeft);
    ml->setContentsMargins(15, 15, 15, 15);
    ml->setSpacing(12);

    QLabel *lblMakeupTitle = new QLabel("新建补卡申请", makeupLeft);
    lblMakeupTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1d4ed8; border: none; background: transparent;");
    ml->addWidget(lblMakeupTitle);

    ml->addWidget(new QLabel("补卡日期:", makeupLeft));
    m_makeupDate = new QDateEdit(QDate::currentDate(), makeupLeft);
    m_makeupDate->setCalendarPopup(true);
    ml->addWidget(m_makeupDate);

    ml->addWidget(new QLabel("补卡类型:", makeupLeft));
    m_makeupType = new QComboBox(makeupLeft);
    m_makeupType->addItem("上班签到", "clock_in");
    m_makeupType->addItem("下班签退", "clock_out");
    m_makeupType->setStyleSheet(
        "QComboBox {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "  background: #ffffff;"
        "}"
    );
    ml->addWidget(m_makeupType);

    ml->addWidget(new QLabel("补卡时间:", makeupLeft));
    m_makeupTime = new QTimeEdit(QTime::currentTime(), makeupLeft);
    ml->addWidget(m_makeupTime);

    ml->addWidget(new QLabel("补卡理由:", makeupLeft));
    m_makeupReason = new QTextEdit(makeupLeft);
    m_makeupReason->setPlaceholderText("请输入补卡事由 (必填，最少输入5个字)");
    m_makeupReason->setStyleSheet(
        "QTextEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}"
        "QTextEdit:focus { border: 1px solid #3b82f6; }"
    );
    ml->addWidget(m_makeupReason, 1);

    auto *btnMakeupRow = new QHBoxLayout;
    btnMakeupRow->addStretch();
    QPushButton *btnSubmitMakeup = new QPushButton("提交申请", makeupLeft);
    btnSubmitMakeup->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    btnMakeupRow->addWidget(btnSubmitMakeup);
    ml->addLayout(btnMakeupRow);
    connect(btnSubmitMakeup, &QPushButton::clicked, this, &AppCenterWidget::submitMakeup);

    makeupLayout->addWidget(makeupLeft);

    QFrame *makeupRight = new QFrame(makeupPage);
    makeupRight->setObjectName("makeupRight");
    makeupRight->setStyleSheet("QFrame#makeupRight { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }");
    auto *mr = new QVBoxLayout(makeupRight);
    mr->setContentsMargins(15, 15, 15, 15);
    mr->setSpacing(10);

    QLabel *lblMakeupHistTitle = new QLabel("我的补卡历史", makeupRight);
    lblMakeupHistTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    mr->addWidget(lblMakeupHistTitle);

    m_makeupModel = new QSqlRelationalTableModel(this);
    m_makeupModel->setTable("makeup_requests");
    m_makeupModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_makeupModel->setHeaderData(3, Qt::Horizontal, "类型");
    m_makeupModel->setHeaderData(4, Qt::Horizontal, "时间");
    m_makeupModel->setHeaderData(5, Qt::Horizontal, "理由");
    m_makeupModel->setHeaderData(6, Qt::Horizontal, "状态");

    m_makeupTable = new QTableView(makeupRight);
    m_makeupTable->setModel(m_makeupModel);
    m_makeupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_makeupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_makeupTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_makeupTable->setShowGrid(false);
    m_makeupTable->verticalHeader()->setVisible(false);
    m_makeupTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_makeupTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  padding: 8px;"
        "  border: none;"
        "  font-weight: bold;"
        "  border-bottom: 2px solid #e2e8f0;"
        "}"
    );
    m_makeupTable->setStyleSheet(
        "QTableView {"
        "  border: none;"
        "  background-color: #ffffff;"
        "  gridline-color: #f1f5f9;"
        "  selection-background-color: #f1f5f9;"
        "  selection-color: #0f172a;"
        "}"
        "QTableView::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #f1f5f9;"
        "}"
    );
    m_makeupTable->setItemDelegateForColumn(3, new MakeupTypeDelegate(m_makeupTable));
    m_makeupTable->setItemDelegateForColumn(6, new RequestStatusDelegate(m_makeupTable));
    mr->addWidget(m_makeupTable, 1);

    makeupLayout->addWidget(makeupRight, 1);

    tabs->addTab(makeupPage, "✉ 补卡申请");

    mainLayout->addWidget(tabs);

    // Apply UI dynamic hiding based on permission
    bool hasApplyPerm = SessionManager::instance()->hasPermission("apply_leave_makeup");
    if (!hasApplyPerm) {
        leaveLeft->setVisible(false);
        makeupLeft->setVisible(false);
    }
}

void AppCenterWidget::refresh()
{
    m_leaveModel->setFilter(QString("leave_requests.emp_id=%1").arg(m_empId));
    m_leaveModel->select();
    m_leaveTable->hideColumn(0);
    m_leaveTable->hideColumn(1);

    m_makeupModel->setFilter(QString("makeup_requests.emp_id=%1").arg(m_empId));
    m_makeupModel->select();
    m_makeupTable->hideColumn(0);
    m_makeupTable->hideColumn(1);
}

void AppCenterWidget::submitLeave()
{
    QDate s = m_leaveStart->date(), e = m_leaveEnd->date();
    QString r = m_leaveReason->toPlainText().trimmed();
    if (s > e) { Toast::show(this, "结束日期不能早于开始日期", Toast::Warning); return; }
    if (r.isEmpty()) { Toast::show(this, "请假事由不能为空", Toast::Warning); return; }
    if (r.length() < 5) { Toast::show(this, "请假事由字数不能少于5个字", Toast::Warning); return; }

    bool overlap = false;
    {
        QSqlQuery check;
        check.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status != ? AND NOT (end_date < ? OR start_date > ?)");
        check.addBindValue(m_empId);
        check.addBindValue(HR::LeaveStatus::REJECTED);
        check.addBindValue(s.toString("yyyy-MM-dd"));
        check.addBindValue(e.toString("yyyy-MM-dd"));
        if (check.exec() && check.next() && check.value(0).toInt() > 0) {
            overlap = true;
        }
    }
    if (overlap) {
        Toast::show(this, "在该日期段内已有尚未拒绝的请假申请", Toast::Warning);
        return;
    }

    bool ok = false;
    QString err;
    {
        QSqlQuery q;
        q.prepare(QString("INSERT INTO leave_requests(emp_id,start_date,end_date,reason,status) VALUES(?,?,?,?,'%1')").arg(HR::LeaveStatus::PENDING));
        q.addBindValue(m_empId);
        q.addBindValue(s.toString("yyyy-MM-dd"));
        q.addBindValue(e.toString("yyyy-MM-dd"));
        q.addBindValue(r);
        ok = q.exec();
        if (!ok) err = q.lastError().text();
    }
    if (ok) {
        Toast::show(this, "请假申请已提交，等待审批", Toast::Success);
        emit logRequested("提交请假申请", s.toString("MM-dd") + " ~ " + e.toString("MM-dd"));
        emit notificationRequested(0, "请假申请", QString("员工提交了请假申请 %1~%2").arg(s.toString("MM-dd"), e.toString("MM-dd")));
        
        m_leaveModel->select();
        m_leaveReason->clear();
        m_leaveStart->setDate(QDate::currentDate());
        m_leaveEnd->setDate(QDate::currentDate());
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "错误", err);
    }
}

void AppCenterWidget::submitMakeup()
{
    QString date = m_makeupDate->date().toString("yyyy-MM-dd");
    QString type = m_makeupType->currentData().toString();
    QString time = m_makeupTime->time().toString("HH:mm:ss");
    QString reason = m_makeupReason->toPlainText().trimmed();
    if (reason.isEmpty()) { Toast::show(this, "补卡事由不能为空", Toast::Warning); return; }
    if (reason.length() < 5) { Toast::show(this, "补卡事由字数不能少于5个字", Toast::Warning); return; }

    bool ok = false;
    QString err;
    {
        QSqlQuery q;
        q.prepare("INSERT INTO makeup_requests(emp_id,att_date,request_type,request_time,reason,status) VALUES(?,?,?,?,?,'待审批')");
        q.addBindValue(m_empId); q.addBindValue(date); q.addBindValue(type); q.addBindValue(time); q.addBindValue(reason);
        ok = q.exec();
        if (!ok) err = q.lastError().text();
    }
    if (ok) {
        Toast::show(this, "补卡申请已提交", Toast::Success);
        emit logRequested("补卡申请", date);
        emit notificationRequested(0, "补卡申请", QString("员工提交了补卡申请 日期：%1").arg(date));
        
        m_makeupModel->select();
        m_makeupReason->clear();
        m_makeupDate->setDate(QDate::currentDate());
        m_makeupTime->setTime(QTime::currentTime());
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "错误", err);
    }
}
