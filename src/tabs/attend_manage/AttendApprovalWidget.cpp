#include "AttendApprovalWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../services/ApprovalService.h"
#include "../../utils/Toast.h"
#include "../../utils/DbUtils.h"
#include "../../utils/UiStyles.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QStringList>
#include <QTextEdit>

namespace ApprovalDialogCode {
constexpr int Approved = QDialog::Accepted;
constexpr int Rejected = 2;
}

// Modal for Leave Approval
class LeaveApprovalDialog : public QDialog
{
public:
    LeaveApprovalDialog(const QString &applicant, const QString &startDate, const QString &endDate, const QString &reason, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("请假申请单审批");
        setFixedSize(420, 330);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(12);

        QFormLayout *form = new QFormLayout;
        form->setSpacing(8);

        QLabel *lblApp = new QLabel(applicant, this);
        lblApp->setStyleSheet(UiStyles::strongText());
        form->addRow("申请人员:", lblApp);

        QLabel *lblDate = new QLabel(QString("%1 至 %2").arg(startDate, endDate), this);
        lblDate->setStyleSheet(UiStyles::strongText());
        form->addRow("请假日期:", lblDate);

        QLabel *lblReason = new QLabel(reason, this);
        lblReason->setWordWrap(true);
        lblReason->setStyleSheet(UiStyles::strongText());
        form->addRow("请假原因:", lblReason);

        l->addLayout(form);

        m_commentEdit = new QTextEdit(this);
        m_commentEdit->setPlaceholderText("审批意见；拒绝时必填");
        m_commentEdit->setFixedHeight(70);
        l->addWidget(m_commentEdit);

        auto *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        btnRow->addStretch();

        QPushButton *btnApprove = new QPushButton("同意", this);
        btnApprove->setStyleSheet(UiStyles::successButton());
        QPushButton *btnReject = new QPushButton("拒绝", this);
        btnReject->setStyleSheet(UiStyles::dangerButton());
        QPushButton *btnCancel = new QPushButton("取消", this);
        btnCancel->setStyleSheet(UiStyles::secondaryButton());

        btnRow->addWidget(btnApprove);
        btnRow->addWidget(btnReject);
        btnRow->addWidget(btnCancel);
        l->addLayout(btnRow);

        connect(btnApprove, &QPushButton::clicked, this, [this]() { done(ApprovalDialogCode::Approved); });
        connect(btnReject, &QPushButton::clicked, this, [this]() { done(ApprovalDialogCode::Rejected); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { reject(); });
    }

    QString comment() const { return m_commentEdit ? m_commentEdit->toPlainText().trimmed() : QString(); }

private:
    QTextEdit *m_commentEdit = nullptr;
};

// Modal for Makeup Approval
class MakeupApprovalDialog : public QDialog
{
public:
    MakeupApprovalDialog(const QString &applicant, const QString &date, const QString &type, const QString &time, const QString &reason, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("补卡申请单审批");
        setFixedSize(420, 350);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(12);

        QFormLayout *form = new QFormLayout;
        form->setSpacing(8);

        QLabel *lblApp = new QLabel(applicant, this);
        lblApp->setStyleSheet(UiStyles::strongText());
        form->addRow("申请人员:", lblApp);

        QLabel *lblDate = new QLabel(date, this);
        lblDate->setStyleSheet(UiStyles::strongText());
        form->addRow("补卡日期:", lblDate);

        QLabel *lblType = new QLabel(type, this);
        lblType->setStyleSheet(UiStyles::strongText());
        form->addRow("补卡类型:", lblType);

        QLabel *lblTime = new QLabel(time, this);
        lblTime->setStyleSheet(UiStyles::strongText());
        form->addRow("补卡时间:", lblTime);

        QLabel *lblReason = new QLabel(reason, this);
        lblReason->setWordWrap(true);
        lblReason->setStyleSheet(UiStyles::strongText());
        form->addRow("补卡原因:", lblReason);

        l->addLayout(form);

        m_commentEdit = new QTextEdit(this);
        m_commentEdit->setPlaceholderText("审批意见；拒绝时必填");
        m_commentEdit->setFixedHeight(70);
        l->addWidget(m_commentEdit);

        auto *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        btnRow->addStretch();

        QPushButton *btnApprove = new QPushButton("同意", this);
        btnApprove->setStyleSheet(UiStyles::successButton());
        QPushButton *btnReject = new QPushButton("拒绝", this);
        btnReject->setStyleSheet(UiStyles::dangerButton());
        QPushButton *btnCancel = new QPushButton("取消", this);
        btnCancel->setStyleSheet(UiStyles::secondaryButton());

        btnRow->addWidget(btnApprove);
        btnRow->addWidget(btnReject);
        btnRow->addWidget(btnCancel);
        l->addLayout(btnRow);

        connect(btnApprove, &QPushButton::clicked, this, [this]() { done(ApprovalDialogCode::Approved); });
        connect(btnReject, &QPushButton::clicked, this, [this]() { done(ApprovalDialogCode::Rejected); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { reject(); });
    }

    QString comment() const { return m_commentEdit ? m_commentEdit->toPlainText().trimmed() : QString(); }

private:
    QTextEdit *m_commentEdit = nullptr;
};

AttendApprovalWidget::AttendApprovalWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);

    m_approvalTabs = new QTabWidget(this);
    m_approvalTabs->setStyleSheet(UiStyles::tabWidget());

    m_approvalTabs->addTab(createLeaveApprovalPage(), "💬 请假审批");
    m_approvalTabs->addTab(createMakeupApprovalPage(), "✉ 补卡审批");
    l->addWidget(m_approvalTabs);
}

QWidget *AttendApprovalWidget::createLeaveApprovalPage()
{
    auto *leavePage = new QWidget(m_approvalTabs);
    auto *l1 = new QVBoxLayout(leavePage);
    l1->setContentsMargins(10, 10, 10, 10);
    l1->setSpacing(10);

    m_leaveModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("leave_approval_model"));
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
    m_leaveTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_leaveTable->setShowGrid(false);
    m_leaveTable->verticalHeader()->setVisible(false);
    m_leaveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_leaveTable->horizontalHeader()->setStyleSheet(UiStyles::tableHeader());
    m_leaveTable->setStyleSheet(UiStyles::tableView());
    m_leaveTable->setItemDelegateForColumn(5, new RequestStatusDelegate(m_leaveTable));
    l1->addWidget(m_leaveTable, 1);

    auto *btnRowLeave = new QHBoxLayout;
    m_btnApproveLeave = new QPushButton("办理审批", leavePage);
    m_btnApproveLeave->setStyleSheet(UiStyles::actionButton());
    m_btnApproveLeave->setEnabled(false);
    m_btnApproveLeaves = new QPushButton("批量同意", leavePage);
    m_btnApproveLeaves->setStyleSheet(UiStyles::successButton());
    m_btnApproveLeaves->setEnabled(false);
    m_btnRejectLeaves = new QPushButton("批量拒绝", leavePage);
    m_btnRejectLeaves->setStyleSheet(UiStyles::dangerButton());
    m_btnRejectLeaves->setEnabled(false);
    btnRowLeave->addStretch();
    btnRowLeave->addWidget(m_btnApproveLeave);
    btnRowLeave->addWidget(m_btnApproveLeaves);
    btnRowLeave->addWidget(m_btnRejectLeaves);
    l1->addLayout(btnRowLeave);

    connect(m_leaveTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AttendApprovalWidget::updateLeaveApprovalButton);
    connect(m_btnApproveLeave, &QPushButton::clicked, this, &AttendApprovalWidget::openLeaveApproval);
    connect(m_btnApproveLeaves, &QPushButton::clicked, this, &AttendApprovalWidget::approveSelectedLeaves);
    connect(m_btnRejectLeaves, &QPushButton::clicked, this, &AttendApprovalWidget::rejectSelectedLeaves);
    connect(m_leaveTable, &QTableView::doubleClicked, this, &AttendApprovalWidget::openLeaveApproval);

    return leavePage;
}

QWidget *AttendApprovalWidget::createMakeupApprovalPage()
{
    auto *makeupPage = new QWidget(m_approvalTabs);
    auto *l2 = new QVBoxLayout(makeupPage);
    l2->setContentsMargins(10, 10, 10, 10);
    l2->setSpacing(10);

    m_makeupModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("makeup_approval_model"));
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
    m_makeupTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_makeupTable->setShowGrid(false);
    m_makeupTable->verticalHeader()->setVisible(false);
    m_makeupTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_makeupTable->horizontalHeader()->setStyleSheet(UiStyles::tableHeader());
    m_makeupTable->setStyleSheet(UiStyles::tableView());
    m_makeupTable->setItemDelegateForColumn(3, new MakeupTypeDelegate(m_makeupTable));
    m_makeupTable->setItemDelegateForColumn(6, new RequestStatusDelegate(m_makeupTable));
    l2->addWidget(m_makeupTable, 1);

    auto *btnRowMakeup = new QHBoxLayout;
    m_btnApproveMakeup = new QPushButton("办理审批", makeupPage);
    m_btnApproveMakeup->setStyleSheet(UiStyles::actionButton());
    m_btnApproveMakeup->setEnabled(false);
    m_btnApproveMakeups = new QPushButton("批量同意", makeupPage);
    m_btnApproveMakeups->setStyleSheet(UiStyles::successButton());
    m_btnApproveMakeups->setEnabled(false);
    m_btnRejectMakeups = new QPushButton("批量拒绝", makeupPage);
    m_btnRejectMakeups->setStyleSheet(UiStyles::dangerButton());
    m_btnRejectMakeups->setEnabled(false);
    btnRowMakeup->addStretch();
    btnRowMakeup->addWidget(m_btnApproveMakeup);
    btnRowMakeup->addWidget(m_btnApproveMakeups);
    btnRowMakeup->addWidget(m_btnRejectMakeups);
    l2->addLayout(btnRowMakeup);

    connect(m_makeupTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AttendApprovalWidget::updateMakeupApprovalButton);
    connect(m_btnApproveMakeup, &QPushButton::clicked, this, &AttendApprovalWidget::openMakeupApproval);
    connect(m_btnApproveMakeups, &QPushButton::clicked, this, &AttendApprovalWidget::approveSelectedMakeups);
    connect(m_btnRejectMakeups, &QPushButton::clicked, this, &AttendApprovalWidget::rejectSelectedMakeups);
    connect(m_makeupTable, &QTableView::doubleClicked, this, &AttendApprovalWidget::openMakeupApproval);

    return makeupPage;
}

void AttendApprovalWidget::updateLeaveApprovalButton()
{
    auto selected = m_leaveTable->selectionModel() ? m_leaveTable->selectionModel()->selectedRows() : QModelIndexList();
    m_btnApproveLeave->setEnabled(selected.size() == 1);
    m_btnApproveLeaves->setEnabled(!selected.isEmpty());
    m_btnRejectLeaves->setEnabled(!selected.isEmpty());
}

void AttendApprovalWidget::updateMakeupApprovalButton()
{
    auto selected = m_makeupTable->selectionModel() ? m_makeupTable->selectionModel()->selectedRows() : QModelIndexList();
    m_btnApproveMakeup->setEnabled(selected.size() == 1);
    m_btnApproveMakeups->setEnabled(!selected.isEmpty());
    m_btnRejectMakeups->setEnabled(!selected.isEmpty());
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
    m_btnApproveLeave->setEnabled(false);

    int row = m_leaveTable->currentIndex().row();
    if (row < 0) {
        updateLeaveApprovalButton();
        return;
    }

    int id = m_leaveModel->data(m_leaveModel->index(row, 0)).toInt();
    QString applicant = m_leaveModel->data(m_leaveModel->index(row, 1)).toString();
    QString startDate = m_leaveModel->data(m_leaveModel->index(row, 2)).toString();
    QString endDate = m_leaveModel->data(m_leaveModel->index(row, 3)).toString();
    QString reason = m_leaveModel->data(m_leaveModel->index(row, 4)).toString();

    LeaveApprovalDialog dlg(applicant, startDate, endDate, reason, this);
    int res = dlg.exec();

    if (res != ApprovalDialogCode::Approved && res != ApprovalDialogCode::Rejected) {
        updateLeaveApprovalButton();
        return;
    }

    const bool approved = (res == ApprovalDialogCode::Approved);
    const ApprovalService::Result result =
        ApprovalService().reviewLeaveRequest(id, approved, dlg.comment(), SessionManager::instance()->empId);
    if (!result.success) {
        QMessageBox::critical(this, "审批失败", result.errorMessage);
        updateLeaveApprovalButton();
        return;
    }

    emit logRequested(result.logAction, result.logDetails);
    emit notificationRequested(result.employeeId, result.notificationTitle, result.notificationContent);
    Toast::show(this, approved ? "审批通过" : "已拒绝该申请", approved ? Toast::Success : Toast::Info);

    refresh();
    updateLeaveApprovalButton();
    GlobalEvents::instance()->dataChanged();
}

void AttendApprovalWidget::openMakeupApproval()
{
    m_btnApproveMakeup->setEnabled(false);

    int row = m_makeupTable->currentIndex().row();
    if (row < 0) {
        updateMakeupApprovalButton();
        return;
    }

    int mid = m_makeupModel->data(m_makeupModel->index(row, 0)).toInt();
    QString applicant = m_makeupModel->data(m_makeupModel->index(row, 1)).toString();
    QString date = m_makeupModel->data(m_makeupModel->index(row, 2)).toString();
    QString typeRaw = m_makeupModel->data(m_makeupModel->index(row, 3)).toString();
    QString type = (typeRaw == "clock_in") ? "上班签到" : "下班签退";
    QString time = m_makeupModel->data(m_makeupModel->index(row, 4)).toString();
    QString reason = m_makeupModel->data(m_makeupModel->index(row, 5)).toString();

    MakeupApprovalDialog dlg(applicant, date, type, time, reason, this);
    int res = dlg.exec();

    if (res != ApprovalDialogCode::Approved && res != ApprovalDialogCode::Rejected) {
        updateMakeupApprovalButton();
        return;
    }

    const bool approved = (res == ApprovalDialogCode::Approved);
    const ApprovalService::Result result =
        ApprovalService().reviewMakeupRequest(mid, approved, dlg.comment(), SessionManager::instance()->empId);
    if (!result.success) {
        QMessageBox::critical(this, "审批失败", result.errorMessage);
        updateMakeupApprovalButton();
        return;
    }

    emit logRequested(result.logAction, result.logDetails);
    emit notificationRequested(result.employeeId, result.notificationTitle, result.notificationContent);
    Toast::show(this, approved ? "审批通过，考勤数据已更新" : "已拒绝该申请",
                approved ? Toast::Success : Toast::Info);

    refresh();
    updateMakeupApprovalButton();
    GlobalEvents::instance()->dataChanged();
}

void AttendApprovalWidget::approveSelectedLeaves()
{
    reviewSelectedLeaves(true);
}

void AttendApprovalWidget::rejectSelectedLeaves()
{
    reviewSelectedLeaves(false);
}

void AttendApprovalWidget::approveSelectedMakeups()
{
    reviewSelectedMakeups(true);
}

void AttendApprovalWidget::rejectSelectedMakeups()
{
    reviewSelectedMakeups(false);
}

void AttendApprovalWidget::reviewSelectedLeaves(bool approved)
{
    const auto selected = m_leaveTable->selectionModel() ? m_leaveTable->selectionModel()->selectedRows() : QModelIndexList();
    if (selected.isEmpty()) return;

    bool ok = true;
    QString comment = QInputDialog::getMultiLineText(this,
        approved ? "批量同意请假" : "批量拒绝请假",
        approved ? "审批意见（可选）:" : "拒绝原因（必填）:",
        approved ? "已核验通过" : QString(), &ok).trimmed();
    if (!ok) return;
    if (!approved && comment.isEmpty()) {
        QMessageBox::warning(this, "请填写原因", "批量拒绝时必须填写拒绝原因，员工端会展示该原因。");
        return;
    }

    int successCount = 0;
    QStringList failures;
    for (const QModelIndex &idx : selected) {
        const int id = m_leaveModel->data(m_leaveModel->index(idx.row(), 0)).toInt();
        const ApprovalService::Result result =
            ApprovalService().reviewLeaveRequest(id, approved, comment, SessionManager::instance()->empId);
        if (result.success) {
            ++successCount;
            emit logRequested(result.logAction, result.logDetails);
            emit notificationRequested(result.employeeId, result.notificationTitle, result.notificationContent);
        } else {
            failures << QString("#%1：%2").arg(id).arg(result.errorMessage);
        }
    }

    refresh();
    updateLeaveApprovalButton();
    GlobalEvents::instance()->dataChanged();
    Toast::show(this, QString("已处理 %1 条请假申请").arg(successCount),
                failures.isEmpty() ? Toast::Success : Toast::Warning);
    if (!failures.isEmpty()) {
        QMessageBox::warning(this, "部分申请处理失败", failures.join("\n"));
    }
}

void AttendApprovalWidget::reviewSelectedMakeups(bool approved)
{
    const auto selected = m_makeupTable->selectionModel() ? m_makeupTable->selectionModel()->selectedRows() : QModelIndexList();
    if (selected.isEmpty()) return;

    bool ok = true;
    QString comment = QInputDialog::getMultiLineText(this,
        approved ? "批量同意补卡" : "批量拒绝补卡",
        approved ? "审批意见（可选）:" : "拒绝原因（必填）:",
        approved ? "已核验通过" : QString(), &ok).trimmed();
    if (!ok) return;
    if (!approved && comment.isEmpty()) {
        QMessageBox::warning(this, "请填写原因", "批量拒绝时必须填写拒绝原因，员工端会展示该原因。");
        return;
    }

    int successCount = 0;
    QStringList failures;
    for (const QModelIndex &idx : selected) {
        const int id = m_makeupModel->data(m_makeupModel->index(idx.row(), 0)).toInt();
        const ApprovalService::Result result =
            ApprovalService().reviewMakeupRequest(id, approved, comment, SessionManager::instance()->empId);
        if (result.success) {
            ++successCount;
            emit logRequested(result.logAction, result.logDetails);
            emit notificationRequested(result.employeeId, result.notificationTitle, result.notificationContent);
        } else {
            failures << QString("#%1：%2").arg(id).arg(result.errorMessage);
        }
    }

    refresh();
    updateMakeupApprovalButton();
    GlobalEvents::instance()->dataChanged();
    Toast::show(this, QString("已处理 %1 条补卡申请").arg(successCount),
                failures.isEmpty() ? Toast::Success : Toast::Warning);
    if (!failures.isEmpty()) {
        QMessageBox::warning(this, "部分申请处理失败", failures.join("\n"));
    }
}
