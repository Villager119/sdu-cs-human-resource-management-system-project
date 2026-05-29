#include "AppCenterWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../services/AttendanceService.h"
#include "../../utils/Toast.h"
#include "../../utils/DbUtils.h"
#include "../../utils/UiStyles.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QHeaderView>
#include <QTabWidget>
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
    tabs->setStyleSheet(UiStyles::tabWidget());

    tabs->addTab(createLeavePage(tabs), "💬 请假申请");
    tabs->addTab(createMakeupPage(tabs), "✉ 补卡申请");
    mainLayout->addWidget(tabs);
}

QWidget *AppCenterWidget::createLeavePage(QTabWidget *tabs)
{
    QWidget *leavePage = new QWidget(tabs);
    auto *leaveLayout = new QHBoxLayout(leavePage);
    leaveLayout->setContentsMargins(10, 10, 10, 10);
    leaveLayout->setSpacing(15);

    QFrame *leaveLeft = new QFrame(leavePage);
    leaveLeft->setObjectName("leaveLeft");
    leaveLeft->setStyleSheet(UiStyles::panelFrame("leaveLeft"));
    leaveLeft->setFixedWidth(320);
    auto *ll = new QVBoxLayout(leaveLeft);
    ll->setContentsMargins(15, 15, 15, 15);
    ll->setSpacing(12);

    QLabel *lblLeaveTitle = new QLabel("新建请假申请", leaveLeft);
    lblLeaveTitle->setStyleSheet(UiStyles::sectionTitle(true));
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
    m_leaveReason->setStyleSheet(UiStyles::textEdit());
    ll->addWidget(m_leaveReason, 1);

    auto *btnLeaveRow = new QHBoxLayout;
    btnLeaveRow->addStretch();
    QPushButton *btnSubmitLeave = new QPushButton("提交申请", leaveLeft);
    btnSubmitLeave->setStyleSheet(UiStyles::primaryButton());
    btnLeaveRow->addWidget(btnSubmitLeave);
    ll->addLayout(btnLeaveRow);
    connect(btnSubmitLeave, &QPushButton::clicked, this, &AppCenterWidget::submitLeave);

    leaveLayout->addWidget(leaveLeft);

    QFrame *leaveRight = new QFrame(leavePage);
    leaveRight->setObjectName("leaveRight");
    leaveRight->setStyleSheet(UiStyles::panelFrame("leaveRight"));
    auto *lr = new QVBoxLayout(leaveRight);
    lr->setContentsMargins(15, 15, 15, 15);
    lr->setSpacing(10);

    QLabel *lblLeaveHistTitle = new QLabel("我的请假历史", leaveRight);
    lblLeaveHistTitle->setStyleSheet(UiStyles::sectionTitle());
    lr->addWidget(lblLeaveHistTitle);

    m_leaveModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("leave_request_model"));
    m_leaveModel->setTable("leave_requests");
    m_leaveModel->setHeaderData(2, Qt::Horizontal, "开始日期");
    m_leaveModel->setHeaderData(3, Qt::Horizontal, "结束日期");
    m_leaveModel->setHeaderData(4, Qt::Horizontal, "请假理由");
    m_leaveModel->setHeaderData(5, Qt::Horizontal, "状态");
    m_leaveModel->setHeaderData(6, Qt::Horizontal, "审批人ID");
    m_leaveModel->setHeaderData(7, Qt::Horizontal, "审批时间");
    m_leaveModel->setHeaderData(8, Qt::Horizontal, "审批意见");

    m_leaveTable = new QTableView(leaveRight);
    m_leaveTable->setModel(m_leaveModel);
    m_leaveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_leaveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_leaveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_leaveTable->setShowGrid(false);
    m_leaveTable->verticalHeader()->setVisible(false);
    m_leaveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_leaveTable->horizontalHeader()->setStyleSheet(UiStyles::tableHeader());
    m_leaveTable->setStyleSheet(UiStyles::tableView());
    m_leaveTable->setItemDelegateForColumn(5, new RequestStatusDelegate(m_leaveTable));
    lr->addWidget(m_leaveTable, 1);

    leaveLayout->addWidget(leaveRight, 1);

    if (!SessionManager::instance()->hasPermission("apply_leave_makeup")) {
        leaveLeft->setVisible(false);
    }

    return leavePage;
}

QWidget *AppCenterWidget::createMakeupPage(QTabWidget *tabs)
{
    QWidget *makeupPage = new QWidget(tabs);
    auto *makeupLayout = new QHBoxLayout(makeupPage);
    makeupLayout->setContentsMargins(10, 10, 10, 10);
    makeupLayout->setSpacing(15);

    QFrame *makeupLeft = new QFrame(makeupPage);
    makeupLeft->setObjectName("makeupLeft");
    makeupLeft->setStyleSheet(UiStyles::panelFrame("makeupLeft"));
    makeupLeft->setFixedWidth(320);
    auto *ml = new QVBoxLayout(makeupLeft);
    ml->setContentsMargins(15, 15, 15, 15);
    ml->setSpacing(12);

    QLabel *lblMakeupTitle = new QLabel("新建补卡申请", makeupLeft);
    lblMakeupTitle->setStyleSheet(UiStyles::sectionTitle(true));
    ml->addWidget(lblMakeupTitle);

    ml->addWidget(new QLabel("补卡日期:", makeupLeft));
    m_makeupDate = new QDateEdit(QDate::currentDate(), makeupLeft);
    m_makeupDate->setCalendarPopup(true);
    ml->addWidget(m_makeupDate);

    ml->addWidget(new QLabel("补卡类型:", makeupLeft));
    m_makeupType = new QComboBox(makeupLeft);
    m_makeupType->addItem("上班签到", "clock_in");
    m_makeupType->addItem("下班签退", "clock_out");
    m_makeupType->setStyleSheet(UiStyles::comboBox());
    ml->addWidget(m_makeupType);

    ml->addWidget(new QLabel("补卡时间:", makeupLeft));
    m_makeupTime = new QTimeEdit(QTime::currentTime(), makeupLeft);
    ml->addWidget(m_makeupTime);

    ml->addWidget(new QLabel("补卡理由:", makeupLeft));
    m_makeupReason = new QTextEdit(makeupLeft);
    m_makeupReason->setPlaceholderText("请输入补卡事由 (必填，最少输入5个字)");
    m_makeupReason->setStyleSheet(UiStyles::textEdit());
    ml->addWidget(m_makeupReason, 1);

    auto *btnMakeupRow = new QHBoxLayout;
    btnMakeupRow->addStretch();
    QPushButton *btnSubmitMakeup = new QPushButton("提交申请", makeupLeft);
    btnSubmitMakeup->setStyleSheet(UiStyles::primaryButton());
    btnMakeupRow->addWidget(btnSubmitMakeup);
    ml->addLayout(btnMakeupRow);
    connect(btnSubmitMakeup, &QPushButton::clicked, this, &AppCenterWidget::submitMakeup);

    makeupLayout->addWidget(makeupLeft);

    QFrame *makeupRight = new QFrame(makeupPage);
    makeupRight->setObjectName("makeupRight");
    makeupRight->setStyleSheet(UiStyles::panelFrame("makeupRight"));
    auto *mr = new QVBoxLayout(makeupRight);
    mr->setContentsMargins(15, 15, 15, 15);
    mr->setSpacing(10);

    QLabel *lblMakeupHistTitle = new QLabel("我的补卡历史", makeupRight);
    lblMakeupHistTitle->setStyleSheet(UiStyles::sectionTitle());
    mr->addWidget(lblMakeupHistTitle);

    m_makeupModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("makeup_request_model"));
    m_makeupModel->setTable("makeup_requests");
    m_makeupModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_makeupModel->setHeaderData(3, Qt::Horizontal, "类型");
    m_makeupModel->setHeaderData(4, Qt::Horizontal, "时间");
    m_makeupModel->setHeaderData(5, Qt::Horizontal, "理由");
    m_makeupModel->setHeaderData(6, Qt::Horizontal, "状态");
    m_makeupModel->setHeaderData(7, Qt::Horizontal, "审批人ID");
    m_makeupModel->setHeaderData(8, Qt::Horizontal, "审批时间");
    m_makeupModel->setHeaderData(9, Qt::Horizontal, "审批意见");

    m_makeupTable = new QTableView(makeupRight);
    m_makeupTable->setModel(m_makeupModel);
    m_makeupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_makeupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_makeupTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_makeupTable->setShowGrid(false);
    m_makeupTable->verticalHeader()->setVisible(false);
    m_makeupTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_makeupTable->horizontalHeader()->setStyleSheet(UiStyles::tableHeader());
    m_makeupTable->setStyleSheet(UiStyles::tableView());
    m_makeupTable->setItemDelegateForColumn(3, new MakeupTypeDelegate(m_makeupTable));
    m_makeupTable->setItemDelegateForColumn(6, new RequestStatusDelegate(m_makeupTable));
    mr->addWidget(m_makeupTable, 1);

    makeupLayout->addWidget(makeupRight, 1);

    if (!SessionManager::instance()->hasPermission("apply_leave_makeup")) {
        makeupLeft->setVisible(false);
    }

    return makeupPage;
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

    const AttendanceService::Result result = AttendanceService().submitLeaveRequest(m_empId, s, e, r);
    if (!result.success) {
        Toast::show(this, result.errorMessage, Toast::Warning);
        return;
    }

    Toast::show(this, result.message, Toast::Success);
    emit logRequested(result.logAction, result.logDetails);
    emit notificationRequested(0, result.notificationTitle, result.notificationContent);
    
    m_leaveModel->select();
    m_leaveReason->clear();
    m_leaveStart->setDate(QDate::currentDate());
    m_leaveEnd->setDate(QDate::currentDate());
    GlobalEvents::instance()->dataChanged();
}

void AppCenterWidget::submitMakeup()
{
    QDate date = m_makeupDate->date();
    QString type = m_makeupType->currentData().toString();
    QTime time = m_makeupTime->time();
    QString reason = m_makeupReason->toPlainText().trimmed();

    const AttendanceService::Result result =
        AttendanceService().submitMakeupRequest(m_empId, date, type, time, reason);
    if (!result.success) {
        Toast::show(this, result.errorMessage, Toast::Warning);
        return;
    }

    Toast::show(this, result.message, Toast::Success);
    emit logRequested(result.logAction, result.logDetails);
    emit notificationRequested(0, result.notificationTitle, result.notificationContent);
    
    m_makeupModel->select();
    m_makeupReason->clear();
    m_makeupDate->setDate(QDate::currentDate());
    m_makeupTime->setTime(QTime::currentTime());
    GlobalEvents::instance()->dataChanged();
}
