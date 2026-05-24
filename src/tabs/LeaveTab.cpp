#include "LeaveTab.h"
#include "../utils/Toast.h"
#include "../core/Constants.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QSqlError>

LeaveTab::LeaveTab(int empId, const QString &role,
                   std::function<void(const QString&, const QString&)> logFn,
                   std::function<void(int, const QString&, const QString&)> notifyFn,
                   QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    m_model = new QSqlRelationalTableModel(this);
    m_model->setTable("leave_requests");
    m_model->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_model->setHeaderData(0, Qt::Horizontal, "请假单号");
    m_model->setHeaderData(1, Qt::Horizontal, "申请人");
    m_model->setHeaderData(2, Qt::Horizontal, "开始日期");
    m_model->setHeaderData(3, Qt::Horizontal, "结束日期");
    m_model->setHeaderData(4, Qt::Horizontal, "请假事由");
    m_model->setHeaderData(5, Qt::Horizontal, "审批状态");

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table, 1);

    // 请假表单行
    auto *applyFormWidget = new QWidget;
    auto *form = new QHBoxLayout(applyFormWidget);
    form->setContentsMargins(0, 0, 0, 0);
    form->addWidget(new QLabel("开始日期:"));
    m_start = new QDateEdit(QDate::currentDate());
    m_start->setCalendarPopup(true);
    form->addWidget(m_start);
    form->addWidget(new QLabel("结束日期:"));
    m_end = new QDateEdit(QDate::currentDate());
    m_end->setCalendarPopup(true);
    form->addWidget(m_end);
    form->addWidget(new QLabel("事由:"));
    m_reason = new QLineEdit;
    form->addWidget(m_reason);
    auto *btnApply = new QPushButton("我要请假");
    form->addWidget(btnApply);
    layout->addWidget(applyFormWidget);

    if (!SessionManager::instance()->hasPermission("apply_leave")) {
        applyFormWidget->setVisible(false);
    }

    // 审批按钮行
    auto *approveWidget = new QWidget;
    auto *approveRow = new QHBoxLayout(approveWidget);
    approveRow->setContentsMargins(0, 0, 0, 0);
    approveRow->addStretch();
    m_btnApprove = new QPushButton("同意");
    m_btnReject = new QPushButton("拒绝");
    approveRow->addWidget(m_btnApprove);
    approveRow->addWidget(m_btnReject);
    layout->addWidget(approveWidget);

    if (!SessionManager::instance()->hasPermission("approve_leave")) {
        m_model->setFilter(QString("leave_requests.emp_id=%1").arg(m_empId));
        approveWidget->setVisible(false);
    }

    m_btnApprove->setEnabled(false);
    m_btnReject->setEnabled(false);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto selected = m_table->selectionModel()->selectedRows();
        if (selected.size() == 1) {
            int row = selected.first().row();
            QString status = m_model->data(m_model->index(row, 5)).toString();
            bool isPending = (status == HR::LeaveStatus::PENDING);
            m_btnApprove->setEnabled(isPending);
            m_btnReject->setEnabled(isPending);
        } else {
            m_btnApprove->setEnabled(false);
            m_btnReject->setEnabled(false);
        }
    });

    connect(btnApply, &QPushButton::clicked, this, &LeaveTab::applyLeave);
    connect(m_btnApprove, &QPushButton::clicked, this, &LeaveTab::approve);
    connect(m_btnReject, &QPushButton::clicked, this, &LeaveTab::reject);

    m_model->select();
}

void LeaveTab::applyLeave()
{
    QDate s = m_start->date(), e = m_end->date();
    QString r = m_reason->text().trimmed();
    if (s > e) { Toast::show(this, "结束日期不能早于开始日期", Toast::Warning); return; }
    if (r.isEmpty()) { Toast::show(this, "请假理由不能为空", Toast::Warning); return; }

    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status != ? AND NOT (end_date < ? OR start_date > ?)");
    check.addBindValue(m_empId);
    check.addBindValue(HR::LeaveStatus::REJECTED);
    check.addBindValue(s.toString("yyyy-MM-dd"));
    check.addBindValue(e.toString("yyyy-MM-dd"));
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        Toast::show(this, "在该日期段内已有尚未拒绝的请假申请", Toast::Warning);
        return;
    }

    QSqlQuery q;
    q.prepare(QString("INSERT INTO leave_requests(emp_id,start_date,end_date,reason,status) VALUES(:e,:s,:e2,:r,'%1')").arg(HR::LeaveStatus::PENDING));
    q.bindValue(":e", m_empId);
    q.bindValue(":s", s.toString("yyyy-MM-dd"));
    q.bindValue(":e2", e.toString("yyyy-MM-dd"));
    q.bindValue(":r", r);
    if (q.exec()) {
        Toast::show(this, "请假申请已提交，等待审批", Toast::Success);
        m_log("提交请假申请", s.toString("MM-dd") + " ~ " + e.toString("MM-dd"));
        m_notify(0, "请假申请", QString("员工提交了请假申请 %1~%2").arg(s.toString("MM-dd"), e.toString("MM-dd")));
        m_model->select();
        m_reason->clear();
        m_start->setDate(QDate::currentDate());
        m_end->setDate(QDate::currentDate());
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "数据库错误", "提交失败: " + q.lastError().text());
    }
}

void LeaveTab::approve()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { Toast::show(this, "请选中要审批的请假单", Toast::Warning); return; }
    int id = m_model->data(m_model->index(row, 0)).toInt();
    int eid = 0;
    {
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM leave_requests WHERE request_id=?");
        eq.addBindValue(id); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }
    QSqlQuery q;
    q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::APPROVED));
    q.addBindValue(id);
    if (q.exec()) {
        Toast::show(this, "已同意该请假申请", Toast::Success);
        m_log("同意请假", "请假单号: " + QString::number(id));
        m_notify(eid, "请假已批准", "你的请假申请已通过审批");
        m_model->select();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "审批失败", "审批失败: " + q.lastError().text());
    }
}

void LeaveTab::reject()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { Toast::show(this, "请选中要审批的请假单", Toast::Warning); return; }
    int id = m_model->data(m_model->index(row, 0)).toInt();
    int eid = 0;
    {
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM leave_requests WHERE request_id=?");
        eq.addBindValue(id); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }
    QSqlQuery q;
    q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::REJECTED));
    q.addBindValue(id);
    if (q.exec()) {
        Toast::show(this, "已拒绝该请假申请", Toast::Info);
        m_log("拒绝请假", "请假单号: " + QString::number(id));
        m_notify(eid, "请假已拒绝", "你的请假申请未通过审批");
        m_model->select();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "审批失败", "审批失败: " + q.lastError().text());
    }
}

void LeaveTab::refresh()
{
    m_model->select();
}

