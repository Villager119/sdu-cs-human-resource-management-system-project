#include "AttendManageTab.h"
#include "../utils/Toast.h"
#include "../utils/CsvExport.h"
#include "../core/Constants.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QSplitter>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTime>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDialog>
#include <QFileDialog>
#include <QTimeEdit>

// Status badges
class ManageAttendanceStatusDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString status = index.data(Qt::DisplayRole).toString();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#f1f5f9"));
            opt.state &= ~QStyle::State_Selected;
        }

        QColor bgColor, textColor;
        if (status == "正常") {
            bgColor = QColor("#dcfce7"); // Light Green
            textColor = QColor("#15803d");
        } else if (status == "迟到") {
            bgColor = QColor("#fef3c7"); // Light Yellow
            textColor = QColor("#b45309");
        } else if (status == "早退") {
            bgColor = QColor("#fee2e2"); // Light Red
            textColor = QColor("#b91c1c");
        } else if (status == "缺卡") {
            bgColor = QColor("#f1f5f9"); // Light Gray
            textColor = QColor("#475569");
        } else if (status == "请假") {
            bgColor = QColor("#dbeafe"); // Light Blue
            textColor = QColor("#1d4ed8");
        } else {
            bgColor = QColor("#f1f5f9");
            textColor = QColor("#475569");
        }

        int paddingX = 8;
        int paddingY = 3;
        QFont font = opt.font;
        font.setPointSize(9);
        font.setBold(true);
        painter->setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(status);
        int textHeight = fm.height();

        int pillWidth = textWidth + paddingX * 2;
        int pillHeight = textHeight + paddingY * 2;

        int pillX = opt.rect.x() + (opt.rect.width() - pillWidth) / 2;
        int pillY = opt.rect.y() + (opt.rect.height() - pillHeight) / 2;

        QRect pillRect(pillX, pillY, pillWidth, pillHeight);
        painter->setBrush(bgColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(pillRect, 4, 4);

        painter->setPen(textColor);
        painter->drawText(pillRect, Qt::AlignCenter, status);

        painter->restore();
    }
};

class ManageRequestStatusDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString status = index.data(Qt::DisplayRole).toString();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#f1f5f9"));
            opt.state &= ~QStyle::State_Selected;
        }

        QColor bgColor, textColor;
        if (status == "待审批" || status == "审批中") {
            bgColor = QColor("#fef3c7"); // Light Yellow
            textColor = QColor("#b45309");
            status = "待审批";
        } else if (status == "已同意" || status == "已通过" || status == "已同意补卡") {
            bgColor = QColor("#dcfce7"); // Light Green
            textColor = QColor("#15803d");
            status = "已同意";
        } else if (status == "已拒绝" || status == "已驳回") {
            bgColor = QColor("#fee2e2"); // Light Red
            textColor = QColor("#b91c1c");
            status = "已拒绝";
        } else {
            bgColor = QColor("#f1f5f9");
            textColor = QColor("#475569");
        }

        int paddingX = 8;
        int paddingY = 3;
        QFont font = opt.font;
        font.setPointSize(9);
        font.setBold(true);
        painter->setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(status);
        int textHeight = fm.height();

        int pillWidth = textWidth + paddingX * 2;
        int pillHeight = textHeight + paddingY * 2;

        int pillX = opt.rect.x() + (opt.rect.width() - pillWidth) / 2;
        int pillY = opt.rect.y() + (opt.rect.height() - pillHeight) / 2;

        QRect pillRect(pillX, pillY, pillWidth, pillHeight);
        painter->setBrush(bgColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(pillRect, 4, 4);

        painter->setPen(textColor);
        painter->drawText(pillRect, Qt::AlignCenter, status);

        painter->restore();
    }
};

class ManageMakeupTypeDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString type = index.data(Qt::DisplayRole).toString();
        if (type == "clock_in") opt.text = "上班签到";
        else if (type == "clock_out") opt.text = "下班签退";

        QStyledItemDelegate::paint(painter, opt, index);
    }
};

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

AttendManageTab::AttendManageTab(int empId, const QString &role,
                                 std::function<void(const QString&, const QString&)> logFn,
                                 std::function<void(int, const QString&, const QString&)> notifyFn,
                                 QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QTabWidget *topTabs = new QTabWidget(this);
    topTabs->setObjectName("attendManageTabs");
    topTabs->setStyleSheet(
        "QTabWidget::panel {"
        "  border: 1px solid #e2e8f0;"
        "  background-color: #ffffff;"
        "  border-radius: 8px;"
        "}"
    );

    m_shiftStartEdit = nullptr;
    m_shiftEndEdit = nullptr;

    topTabs->addTab(createBoardWidget(), "📊 考勤看板");
    topTabs->addTab(createApprovalCenterWidget(), "✍ 审批中心");
    topTabs->addTab(createShiftSettingsWidget(), "⚙ 班次设置");

    mainLayout->addWidget(topTabs);

    refresh();
}

QWidget *AttendManageTab::createBoardWidget()
{
    QWidget *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(15, 15, 15, 15);
    l->setSpacing(12);

    // Filters Bar
    auto *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("开始日期:", w));
    m_startDateFilter = new QDateEdit(w);
    m_startDateFilter->setCalendarPopup(true);
    m_startDateFilter->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
    filterLayout->addWidget(m_startDateFilter);

    filterLayout->addWidget(new QLabel("结束日期:", w));
    m_endDateFilter = new QDateEdit(w);
    m_endDateFilter->setCalendarPopup(true);
    m_endDateFilter->setDate(QDate::currentDate());
    filterLayout->addWidget(m_endDateFilter);

    filterLayout->addWidget(new QLabel("姓名:", w));
    m_nameFilter = new QLineEdit(w);
    m_nameFilter->setPlaceholderText("搜索姓名...");
    m_nameFilter->setFixedWidth(100);
    filterLayout->addWidget(m_nameFilter);

    filterLayout->addWidget(new QLabel("状态:", w));
    m_statusFilter = new QComboBox(w);
    m_statusFilter->addItems({"全部状态", "正常", "迟到", "早退", "缺卡", "请假"});
    filterLayout->addWidget(m_statusFilter);

    QPushButton *btnQuery = new QPushButton("查询", w);
    btnQuery->setStyleSheet("QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #1d4ed8; }");
    filterLayout->addWidget(btnQuery);

    QPushButton *btnReset = new QPushButton("重置", w);
    btnReset->setStyleSheet("QPushButton { background-color: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #e2e8f0; }");
    filterLayout->addWidget(btnReset);

    filterLayout->addStretch();

    QPushButton *btnExport = new QPushButton("导出报表", w);
    btnExport->setStyleSheet("QPushButton { background-color: #0284c7; color: white; border: none; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #0ea5e9; }");
    filterLayout->addWidget(btnExport);

    l->addLayout(filterLayout);

    // Records Table
    m_attModel = new QSqlRelationalTableModel(this);
    m_attModel->setTable("attendances");
    m_attModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_attModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_attModel->setHeaderData(1, Qt::Horizontal, "员工");
    m_attModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_attModel->setHeaderData(3, Qt::Horizontal, "签到时间");
    m_attModel->setHeaderData(4, Qt::Horizontal, "签退时间");
    m_attModel->setHeaderData(5, Qt::Horizontal, "状态");
    m_attModel->setHeaderData(6, Qt::Horizontal, "备注");

    m_attTable = new QTableView(w);
    m_attTable->setModel(m_attModel);
    m_attTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_attTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_attTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_attTable->setShowGrid(false);
    m_attTable->verticalHeader()->setVisible(false);
    m_attTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_attTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  padding: 8px;"
        "  border: none;"
        "  font-weight: bold;"
        "  border-bottom: 2px solid #e2e8f0;"
        "}"
    );
    m_attTable->setStyleSheet(
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
    m_attTable->setItemDelegateForColumn(5, new ManageAttendanceStatusDelegate(m_attTable));
    l->addWidget(m_attTable, 1);

    connect(btnQuery, &QPushButton::clicked, this, &AttendManageTab::filterAttendance);
    connect(btnReset, &QPushButton::clicked, this, &AttendManageTab::resetFilters);
    connect(btnExport, &QPushButton::clicked, this, &AttendManageTab::exportCsv);

    return w;
}

QWidget *AttendManageTab::createApprovalCenterWidget()
{
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
    m_leaveTable->setItemDelegateForColumn(5, new ManageRequestStatusDelegate(m_leaveTable));
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
    connect(m_btnApproveLeave, &QPushButton::clicked, this, &AttendManageTab::openLeaveApproval);
    connect(m_leaveTable, &QTableView::doubleClicked, this, &AttendManageTab::openLeaveApproval);

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
    m_makeupTable->setItemDelegateForColumn(3, new ManageMakeupTypeDelegate(m_makeupTable));
    m_makeupTable->setItemDelegateForColumn(6, new ManageRequestStatusDelegate(m_makeupTable));
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
    connect(m_btnApproveMakeup, &QPushButton::clicked, this, &AttendManageTab::openMakeupApproval);
    connect(m_makeupTable, &QTableView::doubleClicked, this, &AttendManageTab::openMakeupApproval);

    m_approvalTabs->addTab(makeupPage, "✉ 补卡审批");

    return m_approvalTabs;
}

QWidget *AttendManageTab::createShiftSettingsWidget()
{
    QWidget *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(15, 15, 15, 15);
    l->setSpacing(15);

    QFrame *panel = new QFrame(w);
    panel->setObjectName("shiftSettingsPanel");
    panel->setStyleSheet(
        "QFrame#shiftSettingsPanel {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}"
    );
    panel->setFixedWidth(400);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 20, 20, 20);
    panelLayout->setSpacing(15);

    QLabel *title = new QLabel("标准班次工作时间设置", panel);
    title->setStyleSheet("font-size: 15px; font-weight: bold; color: #1d4ed8; border: none; background: transparent;");
    panelLayout->addWidget(title);

    panelLayout->addWidget(new QLabel("上班打卡时间:", panel));
    m_shiftStartEdit = new QTimeEdit(panel);
    m_shiftStartEdit->setDisplayFormat("HH:mm");
    m_shiftStartEdit->setStyleSheet(
        "QTimeEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "}"
        "QTimeEdit:focus { border: 1px solid #3b82f6; }"
    );
    panelLayout->addWidget(m_shiftStartEdit);

    panelLayout->addWidget(new QLabel("下班打卡时间:", panel));
    m_shiftEndEdit = new QTimeEdit(panel);
    m_shiftEndEdit->setDisplayFormat("HH:mm");
    m_shiftEndEdit->setStyleSheet(
        "QTimeEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "}"
        "QTimeEdit:focus { border: 1px solid #3b82f6; }"
    );
    panelLayout->addWidget(m_shiftEndEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton *btnSave = new QPushButton("保存设置", panel);
    btnSave->setStyleSheet(
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
    btnRow->addWidget(btnSave);
    panelLayout->addLayout(btnRow);

    connect(btnSave, &QPushButton::clicked, this, &AttendManageTab::saveShiftSettings);

    l->addWidget(panel);
    l->addStretch(1);

    return w;
}

void AttendManageTab::refresh()
{
    filterAttendance();

    m_leaveModel->setFilter("leave_requests.status = '待审批'");
    m_leaveModel->select();

    m_makeupModel->setFilter("makeup_requests.status = '待审批'");
    m_makeupModel->select();

    // Load shift times for the shift configuration tab
    QSqlQuery sq("SELECT start_time, end_time FROM shifts WHERE shift_id = 1");
    if (sq.exec() && sq.next()) {
        QTime start = QTime::fromString(sq.value(0).toString(), "HH:mm:ss");
        QTime end = QTime::fromString(sq.value(1).toString(), "HH:mm:ss");
        if (m_shiftStartEdit) m_shiftStartEdit->setTime(start);
        if (m_shiftEndEdit) m_shiftEndEdit->setTime(end);
    }
}

void AttendManageTab::filterAttendance()
{
    QStringList conds;

    QString start = m_startDateFilter->date().toString("yyyy-MM-dd");
    QString end = m_endDateFilter->date().toString("yyyy-MM-dd");
    conds << QString("attendances.att_date BETWEEN '%1' AND '%2'").arg(start, end);

    if (!m_nameFilter->text().trimmed().isEmpty()) {
        QString namePattern = m_nameFilter->text().trimmed();
        namePattern.replace("'", "''");
        conds << QString("attendances.emp_id IN (SELECT emp_id FROM employees WHERE name LIKE '%%1%')").arg(namePattern);
    }

    if (m_statusFilter->currentIndex() > 0) {
        QString status = m_statusFilter->currentText();
        status.replace("'", "''");
        conds << QString("attendances.status = '%1'").arg(status);
    }

    m_attModel->setFilter(conds.join(" AND "));
    m_attModel->select();
}

void AttendManageTab::resetFilters()
{
    m_startDateFilter->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
    m_endDateFilter->setDate(QDate::currentDate());
    m_nameFilter->clear();
    m_statusFilter->setCurrentIndex(0);
    filterAttendance();
}

void AttendManageTab::exportCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "导出考勤报表", "考勤报表.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;

    exportModelToCSV(m_attModel, path);
    Toast::show(this, "导出成功", Toast::Success);
}

void AttendManageTab::openLeaveApproval()
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

    QSqlQuery q;
    if (res == QDialog::Accepted) {
        q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::APPROVED));
        q.addBindValue(id);
        if (q.exec()) {
            m_log("同意请假", "单号: " + QString::number(id));
            m_notify(eid, "请假已批准", "你的请假申请已通过审批");
            Toast::show(this, "审批通过", Toast::Success);
        }
    } else if (res == QDialog::Rejected) {
        q.prepare(QString("UPDATE leave_requests SET status='%1' WHERE request_id=?").arg(HR::LeaveStatus::REJECTED));
        q.addBindValue(id);
        if (q.exec()) {
            m_log("拒绝请假", "单号: " + QString::number(id));
            m_notify(eid, "请假已拒绝", "你的请假申请被拒绝");
            Toast::show(this, "已拒绝该申请", Toast::Info);
        }
    }

    refresh();
    GlobalEvents::instance()->dataChanged();
}

void AttendManageTab::openMakeupApproval()
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
        QSqlQuery eq; eq.prepare("SELECT emp_id FROM makeup_requests WHERE makeup_id=?");
        eq.addBindValue(mid); eq.exec();
        if (eq.next()) eid = eq.value(0).toInt();
    }

    QSqlQuery q;
    if (res == QDialog::Accepted) {
        q.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
        q.addBindValue(eid); q.addBindValue(date); q.exec();
        if (q.next()) {
            q.prepare(QString("UPDATE attendances SET %1=?, status='正常' WHERE emp_id=? AND att_date=?").arg(typeRaw));
            q.addBindValue(time); q.addBindValue(eid); q.addBindValue(date);
        } else {
            q.prepare(QString("INSERT INTO attendances(emp_id,att_date,%1,status) VALUES(?,?,?,'正常')").arg(typeRaw));
            q.addBindValue(eid); q.addBindValue(date); q.addBindValue(time);
        }
        q.exec();

        q.prepare("UPDATE makeup_requests SET status='已同意' WHERE makeup_id=?");
        q.addBindValue(mid); q.exec();

        m_log("同意补卡", date);
        m_notify(eid, "补卡已批准", QString("你%1的补卡申请已通过").arg(date));
        Toast::show(this, "审批通过，考勤已补签", Toast::Success);
    } else if (res == QDialog::Rejected) {
        q.prepare("UPDATE makeup_requests SET status='已拒绝' WHERE makeup_id=?");
        q.addBindValue(mid); q.exec();

        m_log("拒绝补卡", "单号: " + QString::number(mid));
        m_notify(eid, "补卡已拒绝", "你的补卡申请未通过审批");
        Toast::show(this, "已拒绝该申请", Toast::Info);
    }

    refresh();
    GlobalEvents::instance()->dataChanged();
}

void AttendManageTab::saveShiftSettings()
{
    QTime start = m_shiftStartEdit->time();
    QTime end = m_shiftEndEdit->time();

    if (start >= end) {
        Toast::show(this, "下班时间不能早于或等于上班时间", Toast::Warning);
        return;
    }

    QSqlQuery q;
    q.prepare("UPDATE shifts SET start_time = ?, end_time = ? WHERE shift_id = 1");
    q.addBindValue(start.toString("HH:mm:ss"));
    q.addBindValue(end.toString("HH:mm:ss"));

    if (q.exec()) {
        m_log("修改工作时间", QString("上班: %1, 下班: %2").arg(start.toString("HH:mm"), end.toString("HH:mm")));
        Toast::show(this, "工作时间设置已更新", Toast::Success);
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "错误", "更新工作时间失败: " + q.lastError().text());
    }
}
