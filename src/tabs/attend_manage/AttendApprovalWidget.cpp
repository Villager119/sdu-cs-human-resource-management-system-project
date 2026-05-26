#include "AttendApprovalWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../utils/Toast.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialog>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTime>
#include <QMessageBox>

// Modal for Leave Approval
class LeaveApprovalDialog : public QDialog
{
public:
    LeaveApprovalDialog(const QString &applicant, const QString &startDate, const QString &endDate, const QString &reason, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("请假申请单审批");
        setFixedSize(380, 240);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(12);

        QFormLayout *form = new QFormLayout;
        form->setSpacing(8);

        QLabel *lblApp = new QLabel(applicant, this);
        lblApp->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("申请人员:", lblApp);

        QLabel *lblDate = new QLabel(QString("%1 至 %2").arg(startDate, endDate), this);
        lblDate->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("请假日期:", lblDate);

        QLabel *lblReason = new QLabel(reason, this);
        lblReason->setWordWrap(true);
        lblReason->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("请假原因:", lblReason);

        l->addLayout(form);

        auto *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        btnRow->addStretch();

        QPushButton *btnApprove = new QPushButton("同意", this);
        btnApprove->setStyleSheet("QPushButton { background-color: #22c55e; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background-color: #16a34a; }");
        QPushButton *btnReject = new QPushButton("拒绝", this);
        btnReject->setStyleSheet("QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background-color: #dc2626; }");
        QPushButton *btnCancel = new QPushButton("取消", this);
        btnCancel->setStyleSheet("QPushButton { background-color: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 16px; } QPushButton:hover { background-color: #e2e8f0; }");

        btnRow->addWidget(btnApprove);
        btnRow->addWidget(btnReject);
        btnRow->addWidget(btnCancel);
        l->addLayout(btnRow);

        connect(btnApprove, &QPushButton::clicked, this, [this]() { done(QDialog::Accepted); });
        connect(btnReject, &QPushButton::clicked, this, [this]() { done(QDialog::Rejected); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { reject(); });
    }
};

// Modal for Makeup Approval
class MakeupApprovalDialog : public QDialog
{
public:
    MakeupApprovalDialog(const QString &applicant, const QString &date, const QString &type, const QString &time, const QString &reason, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("补卡申请单审批");
        setFixedSize(380, 260);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(12);

        QFormLayout *form = new QFormLayout;
        form->setSpacing(8);

        QLabel *lblApp = new QLabel(applicant, this);
        lblApp->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("申请人员:", lblApp);

        QLabel *lblDate = new QLabel(date, this);
        lblDate->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("补卡日期:", lblDate);

        QLabel *lblType = new QLabel(type, this);
        lblType->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("补卡类型:", lblType);

        QLabel *lblTime = new QLabel(time, this);
        lblTime->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("补卡时间:", lblTime);

        QLabel *lblReason = new QLabel(reason, this);
        lblReason->setWordWrap(true);
        lblReason->setStyleSheet("font-weight: bold; color: #1e293b;");
        form->addRow("补卡原因:", lblReason);

        l->addLayout(form);

        auto *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        btnRow->addStretch();

        QPushButton *btnApprove = new QPushButton("同意", this);
        btnApprove->setStyleSheet("QPushButton { background-color: #22c55e; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background-color: #16a34a; }");
        QPushButton *btnReject = new QPushButton("拒绝", this);
        btnReject->setStyleSheet("QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background-color: #dc2626; }");
        QPushButton *btnCancel = new QPushButton("取消", this);
        btnCancel->setStyleSheet("QPushButton { background-color: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 16px; } QPushButton:hover { background-color: #e2e8f0; }");

        btnRow->addWidget(btnApprove);
        btnRow->addWidget(btnReject);
        btnRow->addWidget(btnCancel);
        l->addLayout(btnRow);

        connect(btnApprove, &QPushButton::clicked, this, [this]() { done(QDialog::Accepted); });
        connect(btnReject, &QPushButton::clicked, this, [this]() { done(QDialog::Rejected); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { reject(); });
    }
};

AttendApprovalWidget::AttendApprovalWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);

    m_approvalTabs = new QTabWidget(this);
    m_approvalTabs->setStyleSheet(
        "QTabWidget::panel { border: none; background-color: #ffffff; }"
        "QTabBar::tab { font-size: 13px; padding: 6px 14px; }"
    );

    // Sub-tab 1: Leave Approval
    QWidget *leavePage = new QWidget(m_approvalTabs);
    auto *l1 = new QVBoxLayout(leavePage);
    l1->setContentsMargins(10, 10, 10, 10);
    l1->setSpacing(10);

    m_leaveModel = new QSqlRelationalTableModel(this);
    m_leaveModel->setTable("leave_requests");
    m_leaveModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_leaveModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_leaveModel->setHeaderData(1, Qt::Horizontal, "申请人");
    m_leaveModel->setHeaderData(2, Qt::Horizontal, "开始日期");
    m_leaveModel->setHeaderData(3, Qt::Horizontal, "结束日期");
    m_leaveModel->setHeaderData(4, Qt::Horizontal, "请假事由");
    m_leaveModel->setHeaderData(5, Qt::Horizontal, "状态");

    m_leaveTable = new QTableView(leavePage);
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
    l1->addWidget(m_leaveTable, 1);

    auto *btnRowLeave = new QHBoxLayout;
    m_btnApproveLeave = new QPushButton("办理审批", leavePage);
    m_btnApproveLeave->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }"
    );
    m_btnApproveLeave->setEnabled(false);
    btnRowLeave->addStretch();
    btnRowLeave->addWidget(m_btnApproveLeave);
    l1->addLayout(btnRowLeave);

    connect(m_leaveTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto selected = m_leaveTable->selectionModel()->selectedRows();
        m_btnApproveLeave->setEnabled(selected.size() == 1);
    });
    connect(m_btnApproveLeave, &QPushButton::clicked, this, &AttendApprovalWidget::openLeaveApproval);
    connect(m_leaveTable, &QTableView::doubleClicked, this, &AttendApprovalWidget::openLeaveApproval);

    m_approvalTabs->addTab(leavePage, "💬 请假审批");

    // Sub-tab 2: Makeup Approval
    QWidget *makeupPage = new QWidget(m_approvalTabs);
    auto *l2 = new QVBoxLayout(makeupPage);
    l2->setContentsMargins(10, 10, 10, 10);
    l2->setSpacing(10);

    m_makeupModel = new QSqlRelationalTableModel(this);
    m_makeupModel->setTable("makeup_requests");
    m_makeupModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_makeupModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_makeupModel->setHeaderData(1, Qt::Horizontal, "申请人");
    m_makeupModel->setHeaderData(2, Qt::Horizontal, "补卡日期");
    m_makeupModel->setHeaderData(3, Qt::Horizontal, "类型");
    m_makeupModel->setHeaderData(4, Qt::Horizontal, "补卡时间");
    m_makeupModel->setHeaderData(5, Qt::Horizontal, "原因");
    m_makeupModel->setHeaderData(6, Qt::Horizontal, "状态");

    m_makeupTable = new QTableView(makeupPage);
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
    l2->addWidget(m_makeupTable, 1);

    auto *btnRowMakeup = new QHBoxLayout;
    m_btnApproveMakeup = new QPushButton("办理审批", makeupPage);
    m_btnApproveMakeup->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }"
    );
    m_btnApproveMakeup->setEnabled(false);
    btnRowMakeup->addStretch();
    btnRowMakeup->addWidget(m_btnApproveMakeup);
    l2->addLayout(btnRowMakeup);

    connect(m_makeupTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto selected = m_makeupTable->selectionModel()->selectedRows();
        m_btnApproveMakeup->setEnabled(selected.size() == 1);
    });
    connect(m_btnApproveMakeup, &QPushButton::clicked, this, &AttendApprovalWidget::openMakeupApproval);
    connect(m_makeupTable, &QTableView::doubleClicked, this, &AttendApprovalWidget::openMakeupApproval);

    m_approvalTabs->addTab(makeupPage, "✉ 补卡审批");

    l->addWidget(m_approvalTabs);
}

void AttendApprovalWidget::refresh()
{
    m_leaveModel->setFilter("leave_requests.status = '待审批'");
    m_leaveModel->select();

    m_makeupModel->setFilter("makeup_requests.status = '待审批'");
    m_makeupModel->select();
}

void AttendApprovalWidget::openLeaveApproval()
{
    int row = m_leaveTable->currentIndex().row();
    if (row < 0) return;

    int id = m_leaveModel->data(m_leaveModel->index(row, 0)).toInt();
    QString applicant = m_leaveModel->data(m_leaveModel->index(row, 1)).toString();
    QString startDate = m_leaveModel->data(m_leaveModel->index(row, 2)).toString();
    QString endDate = m_leaveModel->data(m_leaveModel->index(row, 3)).toString();
    QString reason = m_leaveModel->data(m_leaveModel->index(row, 4)).toString();

    LeaveApprovalDialog dlg(applicant, startDate, endDate, reason, this);
    int res = dlg.exec();

    int eid = 0;
    {
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM leave_requests WHERE request_id=?");
        eq.addBindValue(id); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }

    if (res == QDialog::Accepted) {
        bool ok = false;
        {
            QSqlQuery q;
            q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::APPROVED));
            q.addBindValue(id);
            ok = q.exec();
        }
        if (ok) {
            emit logRequested("同意请假", "单号: " + QString::number(id));
            emit notificationRequested(eid, "请假已批准", "你的请假申请已通过审批");
            Toast::show(this, "审批通过", Toast::Success);
        }
    } else if (res == QDialog::Rejected) {
        bool ok = false;
        {
            QSqlQuery q;
            q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::REJECTED));
            q.addBindValue(id);
            ok = q.exec();
        }
        if (ok) {
            emit logRequested("拒绝请假", "单号: " + QString::number(id));
            emit notificationRequested(eid, "请假已拒绝", "你的请假申请被拒绝");
            Toast::show(this, "已拒绝该申请", Toast::Info);
        }
    }

    refresh();
    GlobalEvents::instance()->dataChanged();
}

void AttendApprovalWidget::openMakeupApproval()
{
    int row = m_makeupTable->currentIndex().row();
    if (row < 0) return;

    int mid = m_makeupModel->data(m_makeupModel->index(row, 0)).toInt();
    QString applicant = m_makeupModel->data(m_makeupModel->index(row, 1)).toString();
    QString date = m_makeupModel->data(m_makeupModel->index(row, 2)).toString();
    QString typeRaw = m_makeupModel->data(m_makeupModel->index(row, 3)).toString();
    QString type = (typeRaw == "clock_in") ? "上班签到" : "下班签退";
    QString time = m_makeupModel->data(m_makeupModel->index(row, 4)).toString();
    QString reason = m_makeupModel->data(m_makeupModel->index(row, 5)).toString();

    MakeupApprovalDialog dlg(applicant, date, type, time, reason, this);
    int res = dlg.exec();

    int eid = 0;
    {
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM makeup_requests WHERE request_id=?");
        eq.addBindValue(mid); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }

    if (res == QDialog::Accepted) {
        bool ok = false;
        {
            QSqlQuery q;
            q.prepare("UPDATE makeup_requests SET status='已同意' WHERE request_id=?");
            q.addBindValue(mid);
            ok = q.exec();
        }
        if (ok) {
            emit logRequested("同意补卡", "单号: " + QString::number(mid));
            emit notificationRequested(eid, "补卡申请已批准", "你的补卡申请已通过审批");

            // Apply modifications to the real attendance record
            int attId = -1;
            bool attExists = false;
            {
                QSqlQuery aq;
                aq.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
                aq.addBindValue(eid); aq.addBindValue(date);
                if (aq.exec() && aq.next()) {
                    attId = aq.value(0).toInt();
                    attExists = true;
                }
            }
            if (attExists) {
                {
                    QSqlQuery uq;
                    if (typeRaw == "clock_in") {
                        uq.prepare("UPDATE attendances SET clock_in=? WHERE att_id=?");
                        uq.addBindValue(time); uq.addBindValue(attId);
                        uq.exec();
                    } else {
                        uq.prepare("UPDATE attendances SET clock_out=? WHERE att_id=?");
                        uq.addBindValue(time); uq.addBindValue(attId);
                        uq.exec();
                    }
                }
                
                // Re-evaluate attendance status based on standard shift times
                QString start = "09:00:00";
                QString end = "18:00:00";
                bool shiftFound = false;
                {
                    QSqlQuery sq("SELECT start_time, end_time FROM shifts WHERE shift_id=1");
                    if (sq.exec() && sq.next()) {
                        start = sq.value(0).toString();
                        end = sq.value(1).toString();
                        shiftFound = true;
                    }
                }
                if (shiftFound) {
                    QString ci, co;
                    bool attFound = false;
                    {
                        QSqlQuery rq;
                        rq.prepare("SELECT clock_in, clock_out FROM attendances WHERE att_id=?");
                        rq.addBindValue(attId);
                        if (rq.exec() && rq.next()) {
                            ci = rq.value(0).toString();
                            co = rq.value(1).toString();
                            attFound = true;
                        }
                    }
                    if (attFound) {
                        QString status = "正常";
                        if (!ci.isEmpty() && ci > start) status = "迟到";
                        else if (!co.isEmpty() && co < end) status = "早退";
                        
                        {
                            QSqlQuery fq;
                            fq.prepare("UPDATE attendances SET status=? WHERE att_id=?");
                            fq.addBindValue(status); fq.addBindValue(attId);
                            fq.exec();
                        }
                    }
                }
            } else {
                // Create a new attendance record
                {
                    QSqlQuery iq;
                    if (typeRaw == "clock_in") {
                        iq.prepare("INSERT INTO attendances(emp_id, att_date, clock_in, status, comment) VALUES(?,?,?,'正常','补卡录入')");
                    } else {
                        iq.prepare("INSERT INTO attendances(emp_id, att_date, clock_out, status, comment) VALUES(?,?,?,'正常','补卡录入')");
                    }
                    iq.addBindValue(eid); iq.addBindValue(date); iq.addBindValue(time);
                    iq.exec();
                }
            }

            Toast::show(this, "审批通过，考勤数据已更新", Toast::Success);
        }
    } else if (res == QDialog::Rejected) {
        bool ok = false;
        {
            QSqlQuery q;
            q.prepare("UPDATE makeup_requests SET status='已拒绝' WHERE request_id=?");
            q.addBindValue(mid);
            ok = q.exec();
        }
        if (ok) {
            emit logRequested("拒绝补卡", "单号: " + QString::number(mid));
            emit notificationRequested(eid, "补卡申请已拒绝", "你的补卡申请被拒绝");
            Toast::show(this, "已拒绝该申请", Toast::Info);
        }
    }

    refresh();
    GlobalEvents::instance()->dataChanged();
}
