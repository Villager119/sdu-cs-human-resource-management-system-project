#include "DailyClockWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../widgets/PaginationBar.h"
#include "../../services/AttendanceService.h"
#include "../../utils/Toast.h"
#include "../../utils/DbUtils.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

DailyClockWidget::DailyClockWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(15);
    l->addWidget(createClockCard());
    l->addWidget(createAttendanceTable(), 1);

    setupTimerAndConnections();
    applyPermissionVisibility();
}

QWidget *DailyClockWidget::createClockCard()
{
    auto *clockCard = new QFrame(this);
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

    return clockCard;
}

QWidget *DailyClockWidget::createAttendanceTable()
{
    auto *box = new QGroupBox("本月考勤记录", this);
    box->setStyleSheet("QGroupBox { font-size: 14px; font-weight: bold; color: #1e3a8a; border: 1px solid #cbd5e1; border-radius: 8px; margin-top: 10px; padding-top: 10px; }");
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(10, 10, 10, 10);

    m_attModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("daily_attendance_model"));
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

    m_pagination = new PaginationBar(10, box);
    boxLayout->addWidget(m_pagination);
    connect(m_pagination, &PaginationBar::pageChanged, this, [this](int) {
        loadAttendancePage(false);
    });

    return box;
}

void DailyClockWidget::setupTimerAndConnections()
{
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &DailyClockWidget::updateClock);
    m_clockTimer->start(1000);
    updateClock();

    connect(m_btnClockIn, &QPushButton::clicked, this, &DailyClockWidget::clockIn);
    connect(m_btnClockOut, &QPushButton::clicked, this, &DailyClockWidget::clockOut);
}

void DailyClockWidget::applyPermissionVisibility()
{
    bool hasClockIn = SessionManager::instance()->hasPermission("apply_leave_makeup");
    if (!hasClockIn) {
        m_btnClockIn->setVisible(false);
        m_btnClockOut->setVisible(false);
    }
}

void DailyClockWidget::updateClock()
{
    m_lblDate->setText(QDate::currentDate().toString("yyyy年MM月dd日 dddd"));
    m_lblTime->setText(QTime::currentTime().toString("HH:mm:ss"));
}

void DailyClockWidget::refresh()
{
    const QDate monthStart(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    QString backfillError;
    AttendanceService().backfillAttendanceRange(monthStart, QDate::currentDate(), &backfillError, m_empId);

    const AttendanceService::TodayStatus today = AttendanceService().todayStatus(m_empId);
    bool hasClockInPerm = SessionManager::instance()->hasPermission("apply_leave_makeup");

    if (today.hasRecord) {
        if (!today.clockIn.isEmpty()) {
            m_btnClockIn->setEnabled(false);
            m_btnClockIn->setText("已打上班卡\n" + today.clockIn.left(5));
        } else {
            m_btnClockIn->setEnabled(hasClockInPerm);
            m_btnClockIn->setText("上班打卡");
        }

        if (!today.clockOut.isEmpty()) {
            m_btnClockOut->setEnabled(false);
            m_btnClockOut->setText("已打下班卡\n" + today.clockOut.left(5));
        } else {
            m_btnClockOut->setEnabled(hasClockInPerm);
            m_btnClockOut->setText("下班打卡");
        }
        
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: %3 (上班: %4 | 下班: %5)")
            .arg(today.shiftStart, today.shiftEnd, today.status,
                 today.clockIn.isEmpty() ? "未打卡" : today.clockIn.left(5),
                 today.clockOut.isEmpty() ? "未打卡" : today.clockOut.left(5)));
    } else {
        m_btnClockIn->setEnabled(hasClockInPerm);
        m_btnClockIn->setText("上班打卡");
        m_btnClockOut->setEnabled(hasClockInPerm);
        m_btnClockOut->setText("下班打卡");
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: 未打卡")
            .arg(today.shiftStart, today.shiftEnd));
    }

    loadAttendancePage(true);
}

void DailyClockWidget::loadAttendancePage(bool resetPage)
{
    if (!m_attModel || !m_pagination) return;
    if (resetPage) {
        m_pagination->refresh();
    }

    QString errorText;
    const int total = currentMonthAttendanceCount(&errorText);
    if (!errorText.isEmpty()) {
        Toast::show(this, "统计考勤记录失败：" + errorText, Toast::Warning);
        return;
    }
    m_pagination->setTotalRecords(total);

    const QString filter = currentMonthPagedFilter(&errorText);
    if (!errorText.isEmpty()) {
        Toast::show(this, "读取考勤分页数据失败：" + errorText, Toast::Warning);
        return;
    }

    m_attModel->setFilter(filter);
    m_attModel->setSort(2, Qt::DescendingOrder);
    if (!m_attModel->select()) {
        Toast::show(this, "加载考勤记录失败：" + m_attModel->lastError().text(), Toast::Warning);
        return;
    }
    m_attTable->hideColumn(0);
    m_attTable->hideColumn(1);
}

QString DailyClockWidget::currentMonthBaseFilter() const
{
    return QString("attendances.emp_id=%1 AND attendances.att_date LIKE '%2%'")
        .arg(m_empId)
        .arg(QDate::currentDate().toString("yyyy-MM"));
}

QString DailyClockWidget::currentMonthPagedFilter(QString *errorText) const
{
    if (errorText) errorText->clear();
    const int pageSize = 10;
    const int offset = m_pagination ? (m_pagination->currentPage() - 1) * pageSize : 0;

    QSqlQuery query;
    query.prepare(QString("SELECT attendances.att_id FROM attendances WHERE %1 "
                          "ORDER BY attendances.att_date DESC, attendances.att_id DESC "
                          "LIMIT %2 OFFSET %3")
                      .arg(currentMonthBaseFilter())
                      .arg(pageSize)
                      .arg(offset));

    QStringList ids;
    if (query.exec()) {
        while (query.next()) {
            ids << query.value(0).toString();
        }
    } else if (errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();

    if (ids.isEmpty()) {
        return "1=0";
    }
    return QString("attendances.att_id IN (%1)").arg(ids.join(","));
}

int DailyClockWidget::currentMonthAttendanceCount(QString *errorText) const
{
    if (errorText) errorText->clear();
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM attendances WHERE emp_id=? AND att_date LIKE ?");
    query.addBindValue(m_empId);
    query.addBindValue(QDate::currentDate().toString("yyyy-MM") + "%");

    int total = 0;
    if (query.exec() && query.next()) {
        total = query.value(0).toInt();
    } else if (errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return total;
}

void DailyClockWidget::clockIn()
{
    const AttendanceService::Result result =
        AttendanceService().clockIn(m_empId, QDate::currentDate(), QTime::currentTime());
    if (!result.success) {
        Toast::show(this, result.errorMessage, Toast::Warning);
        return;
    }

    Toast::show(this, result.message, Toast::Success);
    emit logRequested(result.logAction, result.logDetails);
    refresh();
    GlobalEvents::instance()->dataChanged();
}

void DailyClockWidget::clockOut()
{
    const AttendanceService::Result result =
        AttendanceService().clockOut(m_empId, QDate::currentDate(), QTime::currentTime());
    if (!result.success) {
        Toast::show(this, result.errorMessage, Toast::Warning);
        return;
    }

    Toast::show(this, result.message, Toast::Success);
    emit logRequested(result.logAction, result.logDetails);
    refresh();
    GlobalEvents::instance()->dataChanged();
}
