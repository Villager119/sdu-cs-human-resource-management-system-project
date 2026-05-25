#include "MyAttendanceTab.h"
#include "../utils/Toast.h"
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
#include <QGroupBox>

// Delegate for Attendance Status badge in Table
class AttendanceStatusDelegate : public QStyledItemDelegate
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

// Delegate for Request Status (Leave & Makeup) badge in Table
class RequestStatusDelegate : public QStyledItemDelegate
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

// Delegate for translating makeup request type to Chinese
class MakeupTypeDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString type = index.data(Qt::DisplayRole).toString();
        if (type == "clock_in") {
            opt.text = "上班签到";
        } else if (type == "clock_out") {
            opt.text = "下班签退";
        }

        QStyledItemDelegate::paint(painter, opt, index);
    }
};

MyAttendanceTab::MyAttendanceTab(int empId, const QString &role,
                                 std::function<void(const QString&, const QString&)> logFn,
                                 std::function<void(int, const QString&, const QString&)> notifyFn,
                                 QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QTabWidget *topTabs = new QTabWidget(this);
    topTabs->setObjectName("myAttendTabs");
    topTabs->setStyleSheet(
        "QTabWidget::panel {"
        "  border: 1px solid #e2e8f0;"
        "  background-color: #ffffff;"
        "  border-radius: 8px;"
        "}"
    );

    topTabs->addTab(createDailyClockWidget(), "⏰ 每日打卡");
    topTabs->addTab(createAppCenterWidget(), "📂 申请中心");

    mainLayout->addWidget(topTabs);

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &MyAttendanceTab::updateClock);
    m_clockTimer->start(1000);
    updateClock();

    refresh();
}

void MyAttendanceTab::updateClock()
{
    m_lblDate->setText(QDate::currentDate().toString("yyyy年MM月dd日 dddd"));
    m_lblTime->setText(QTime::currentTime().toString("HH:mm:ss"));
}

QWidget *MyAttendanceTab::createDailyClockWidget()
{
    QWidget *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(15, 15, 15, 15);
    l->setSpacing(15);

    // Today's Clock Card Frame
    QFrame *clockCard = new QFrame(w);
    clockCard->setObjectName("clockCard");
    clockCard->setStyleSheet(
        "QFrame#clockCard {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}"
    );
    clockCard->setFixedHeight(180);

    auto *cardLayout = new QVBoxLayout(clockCard);
    cardLayout->setContentsMargins(20, 15, 20, 15);
    cardLayout->setSpacing(8);

    m_lblDate = new QLabel(clockCard);
    m_lblDate->setStyleSheet("font-size: 14px; font-weight: bold; color: #64748b; border: none; background: transparent;");
    m_lblDate->setAlignment(Qt::AlignCenter);

    m_lblTime = new QLabel(clockCard);
    m_lblTime->setStyleSheet("font-size: 32px; font-weight: 800; color: #1e293b; border: none; background: transparent; font-family: 'Courier New', monospace;");
    m_lblTime->setAlignment(Qt::AlignCenter);

    m_lblStatus = new QLabel("今日排班: 标准班 (09:00 - 18:00)", clockCard);
    m_lblStatus->setStyleSheet("font-size: 13px; color: #475569; border: none; background: transparent;");
    m_lblStatus->setAlignment(Qt::AlignCenter);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(20);
    btnRow->addStretch();

    m_btnClockIn = new QPushButton("上班打卡", clockCard);
    m_btnClockIn->setFixedSize(140, 48);
    m_btnClockIn->setStyleSheet(
        "QPushButton {"
        "  background-color: #10b981;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #059669; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }"
    );

    m_btnClockOut = new QPushButton("下班打卡", clockCard);
    m_btnClockOut->setFixedSize(140, 48);
    m_btnClockOut->setStyleSheet(
        "QPushButton {"
        "  background-color: #ef4444;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #dc2626; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }"
    );

    btnRow->addWidget(m_btnClockIn);
    btnRow->addWidget(m_btnClockOut);
    btnRow->addStretch();

    cardLayout->addWidget(m_lblDate);
    cardLayout->addWidget(m_lblTime);
    cardLayout->addWidget(m_lblStatus);
    cardLayout->addLayout(btnRow);

    l->addWidget(clockCard);

    // Records Table Frame
    QGroupBox *box = new QGroupBox("本月考勤记录", w);
    box->setStyleSheet("QGroupBox { font-size: 14px; font-weight: bold; color: #1e3a8a; border: 1px solid #cbd5e1; border-radius: 8px; margin-top: 10px; padding-top: 10px; }");
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(10, 10, 10, 10);

    m_attModel = new QSqlRelationalTableModel(this);
    m_attModel->setTable("attendances");
    m_attModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_attModel->setHeaderData(3, Qt::Horizontal, "签到时间");
    m_attModel->setHeaderData(4, Qt::Horizontal, "签退时间");
    m_attModel->setHeaderData(5, Qt::Horizontal, "状态");
    m_attModel->setHeaderData(6, Qt::Horizontal, "备注");

    m_attTable = new QTableView(box);
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
    m_attTable->setItemDelegateForColumn(5, new AttendanceStatusDelegate(m_attTable));
    boxLayout->addWidget(m_attTable);

    l->addWidget(box, 1);

    connect(m_btnClockIn, &QPushButton::clicked, this, &MyAttendanceTab::clockIn);
    connect(m_btnClockOut, &QPushButton::clicked, this, &MyAttendanceTab::clockOut);

    return w;
}

QWidget *MyAttendanceTab::createAppCenterWidget()
{
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
    connect(btnSubmitLeave, &QPushButton::clicked, this, &MyAttendanceTab::submitLeave);

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
    connect(btnSubmitMakeup, &QPushButton::clicked, this, &MyAttendanceTab::submitMakeup);

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

    return tabs;
}

void MyAttendanceTab::refresh()
{
    // Fetch standard shift start and end times dynamically
    QSqlQuery sq("SELECT start_time, end_time FROM shifts WHERE shift_id = 1");
    QString shiftStart = "09:00";
    QString shiftEnd = "18:00";
    if (sq.exec() && sq.next()) {
        shiftStart = sq.value(0).toString().left(5);
        shiftEnd = sq.value(1).toString().left(5);
    }

    // Today's Clock Status
    QSqlQuery q;
    q.prepare("SELECT clock_in, clock_out, status FROM attendances WHERE emp_id = ? AND att_date = ?");
    q.addBindValue(m_empId);
    q.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
    q.exec();
    if (q.next()) {
        QString clockInTime = q.value(0).toString();
        QString clockOutTime = q.value(1).toString();
        QString status = q.value(2).toString();

        if (!clockInTime.isEmpty()) {
            m_btnClockIn->setEnabled(false);
            m_btnClockIn->setText("已打上班卡\n" + clockInTime.left(5));
        } else {
            m_btnClockIn->setEnabled(true);
            m_btnClockIn->setText("上班打卡");
        }

        if (!clockOutTime.isEmpty()) {
            m_btnClockOut->setEnabled(false);
            m_btnClockOut->setText("已打下班卡\n" + clockOutTime.left(5));
        } else {
            m_btnClockOut->setEnabled(true);
            m_btnClockOut->setText("下班打卡");
        }
        
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: %3 (上班: %4 | 下班: %5)")
            .arg(shiftStart, shiftEnd, status, 
                 clockInTime.isEmpty() ? "未打卡" : clockInTime.left(5),
                 clockOutTime.isEmpty() ? "未打卡" : clockOutTime.left(5)));
    } else {
        m_btnClockIn->setEnabled(true);
        m_btnClockIn->setText("上班打卡");
        m_btnClockOut->setEnabled(true);
        m_btnClockOut->setText("下班打卡");
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: 未打卡").arg(shiftStart, shiftEnd));
    }

    // Tables selection
    m_attModel->setFilter(QString("attendances.emp_id=%1 AND attendances.att_date LIKE '%2%'").arg(m_empId).arg(QDate::currentDate().toString("yyyy-MM")));
    m_attModel->select();
    m_attTable->hideColumn(0);
    m_attTable->hideColumn(1);

    m_leaveModel->setFilter(QString("leave_requests.emp_id=%1").arg(m_empId));
    m_leaveModel->select();
    m_leaveTable->hideColumn(0);
    m_leaveTable->hideColumn(1);

    m_makeupModel->setFilter(QString("makeup_requests.emp_id=%1").arg(m_empId));
    m_makeupModel->select();
    m_makeupTable->hideColumn(0);
    m_makeupTable->hideColumn(1);
}

void MyAttendanceTab::clockIn()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    QSqlQuery q;
    q.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
    q.addBindValue(m_empId); q.addBindValue(today); q.exec();
    if (q.next()) {
        Toast::show(this, "今天已打卡，请勿重复", Toast::Warning);
        return;
    }

    QSqlQuery sq("SELECT start_time FROM shifts WHERE shift_id=1");
    sq.exec();
    QString start = sq.next() ? sq.value(0).toString() : "09:00:00";
    QString status = (now > start) ? "迟到" : "正常";

    q.prepare("INSERT INTO attendances(emp_id,att_date,clock_in,status) VALUES(?,?,?,?)");
    q.addBindValue(m_empId); q.addBindValue(today); q.addBindValue(now); q.addBindValue(status);
    if (q.exec()) {
        Toast::show(this, QString("上班打卡成功 (%1)").arg(now), Toast::Success);
        m_log("上班打卡", today);
        refresh();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
}

void MyAttendanceTab::clockOut()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    QSqlQuery q;
    q.prepare("SELECT att_id, status FROM attendances WHERE emp_id=? AND att_date=?");
    q.addBindValue(m_empId); q.addBindValue(today); q.exec();
    if (!q.next()) { Toast::show(this, "请先打上班卡", Toast::Warning); return; }

    QSqlQuery sq("SELECT end_time FROM shifts WHERE shift_id=1");
    sq.exec();
    QString end = sq.next() ? sq.value(0).toString() : "18:00:00";
    QString status = (now < end) ? "早退" : "正常";
    if (q.value(1).toString() == "迟到") status = "迟到";
    if (status == "正常" && q.value(1).toString() == "正常") status = "正常";

    q.prepare("UPDATE attendances SET clock_out=?, status=? WHERE emp_id=? AND att_date=?");
    q.addBindValue(now); q.addBindValue(status); q.addBindValue(m_empId); q.addBindValue(today);
    if (q.exec()) {
        Toast::show(this, QString("下班打卡成功 (%1) [%2]").arg(now, status), Toast::Success);
        m_log("下班打卡", today);
        refresh();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
}

void MyAttendanceTab::submitLeave()
{
    QDate s = m_leaveStart->date(), e = m_leaveEnd->date();
    QString r = m_leaveReason->toPlainText().trimmed();
    if (s > e) { Toast::show(this, "结束日期不能早于开始日期", Toast::Warning); return; }
    if (r.isEmpty()) { Toast::show(this, "请假事由不能为空", Toast::Warning); return; }
    if (r.length() < 5) { Toast::show(this, "请假事由字数不能少于5个字", Toast::Warning); return; }

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
    q.prepare(QString("INSERT INTO leave_requests(emp_id,start_date,end_date,reason,status) VALUES(?,?,?,?,'%1')").arg(HR::LeaveStatus::PENDING));
    q.addBindValue(m_empId);
    q.addBindValue(s.toString("yyyy-MM-dd"));
    q.addBindValue(e.toString("yyyy-MM-dd"));
    q.addBindValue(r);
    if (q.exec()) {
        Toast::show(this, "请假申请已提交，等待审批", Toast::Success);
        m_log("提交请假申请", s.toString("MM-dd") + " ~ " + e.toString("MM-dd"));
        m_notify(0, "请假申请", QString("员工提交了请假申请 %1~%2").arg(s.toString("MM-dd"), e.toString("MM-dd")));
        m_leaveModel->select();
        m_leaveReason->clear();
        m_leaveStart->setDate(QDate::currentDate());
        m_leaveEnd->setDate(QDate::currentDate());
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "错误", q.lastError().text());
    }
}

void MyAttendanceTab::submitMakeup()
{
    QString date = m_makeupDate->date().toString("yyyy-MM-dd");
    QString type = m_makeupType->currentData().toString();
    QString time = m_makeupTime->time().toString("HH:mm:ss");
    QString reason = m_makeupReason->toPlainText().trimmed();
    if (reason.isEmpty()) { Toast::show(this, "补卡事由不能为空", Toast::Warning); return; }
    if (reason.length() < 5) { Toast::show(this, "补卡事由字数不能少于5个字", Toast::Warning); return; }

    QSqlQuery q;
    q.prepare("INSERT INTO makeup_requests(emp_id,att_date,request_type,request_time,reason,status) VALUES(?,?,?,?,?,'待审批')");
    q.addBindValue(m_empId); q.addBindValue(date); q.addBindValue(type); q.addBindValue(time); q.addBindValue(reason);
    if (q.exec()) {
        Toast::show(this, "补卡申请已提交", Toast::Success);
        m_log("补卡申请", date);
        m_notify(0, "补卡申请", QString("员工提交了补卡申请 %1").arg(date));
        m_makeupModel->select();
        m_makeupReason->clear();
        m_makeupDate->setDate(QDate::currentDate());
        m_makeupTime->setTime(QTime::currentTime());
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "错误", q.lastError().text());
    }
}
