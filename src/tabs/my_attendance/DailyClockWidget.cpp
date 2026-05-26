#include "DailyClockWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../utils/Toast.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTime>
#include <QMessageBox>

DailyClockWidget::DailyClockWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(15);

    // Today's Clock Card Frame
    QFrame *clockCard = new QFrame(this);
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
    QGroupBox *box = new QGroupBox("本月考勤记录", this);
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

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &DailyClockWidget::updateClock);
    m_clockTimer->start(1000);
    updateClock();

    connect(m_btnClockIn, &QPushButton::clicked, this, &DailyClockWidget::clockIn);
    connect(m_btnClockOut, &QPushButton::clicked, this, &DailyClockWidget::clockOut);

    // Apply UI dynamic hiding based on permission
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
    // Fetch standard shift start and end times dynamically
    QString shiftStart = "09:00";
    QString shiftEnd = "18:00";
    {
        QSqlQuery sq("SELECT start_time, end_time FROM shifts WHERE shift_id = 1");
        if (sq.exec() && sq.next()) {
            shiftStart = sq.value(0).toString().left(5);
            shiftEnd = sq.value(1).toString().left(5);
        }
    }

    // Today's Clock Status
    bool hasTodayAtt = false;
    QString clockInTime, clockOutTime, status;
    {
        QSqlQuery q;
        q.prepare("SELECT clock_in, clock_out, status FROM attendances WHERE emp_id = ? AND att_date = ?");
        q.addBindValue(m_empId);
        q.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
        if (q.exec() && q.next()) {
            clockInTime = q.value(0).toString();
            clockOutTime = q.value(1).toString();
            status = q.value(2).toString();
            hasTodayAtt = true;
        }
    }
    
    bool hasClockInPerm = SessionManager::instance()->hasPermission("apply_leave_makeup");

    if (hasTodayAtt) {
        if (!clockInTime.isEmpty()) {
            m_btnClockIn->setEnabled(false);
            m_btnClockIn->setText("已打上班卡\n" + clockInTime.left(5));
        } else {
            m_btnClockIn->setEnabled(hasClockInPerm);
            m_btnClockIn->setText("上班打卡");
        }

        if (!clockOutTime.isEmpty()) {
            m_btnClockOut->setEnabled(false);
            m_btnClockOut->setText("已打下班卡\n" + clockOutTime.left(5));
        } else {
            m_btnClockOut->setEnabled(hasClockInPerm);
            m_btnClockOut->setText("下班打卡");
        }
        
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: %3 (上班: %4 | 下班: %5)")
            .arg(shiftStart, shiftEnd, status, 
                 clockInTime.isEmpty() ? "未打卡" : clockInTime.left(5),
                 clockOutTime.isEmpty() ? "未打卡" : clockOutTime.left(5)));
    } else {
        m_btnClockIn->setEnabled(hasClockInPerm);
        m_btnClockIn->setText("上班打卡");
        m_btnClockOut->setEnabled(hasClockInPerm);
        m_btnClockOut->setText("下班打卡");
        m_lblStatus->setText(QString("今日排班: 标准班 (%1 - %2) | 考勤状态: 未打卡").arg(shiftStart, shiftEnd));
    }

    // Tables selection
    m_attModel->setFilter(QString("attendances.emp_id=%1 AND attendances.att_date LIKE '%2%'").arg(m_empId).arg(QDate::currentDate().toString("yyyy-MM")));
    m_attModel->select();
    m_attTable->hideColumn(0);
    m_attTable->hideColumn(1);
}

void DailyClockWidget::clockIn()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    bool alreadyClocked = false;
    {
        QSqlQuery q;
        q.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
        q.addBindValue(m_empId); q.addBindValue(today);
        if (q.exec() && q.next()) {
            alreadyClocked = true;
        }
    }
    if (alreadyClocked) {
        Toast::show(this, "今天已打卡，请勿重复", Toast::Warning);
        return;
    }

    QString start = "09:00:00";
    {
        QSqlQuery sq("SELECT start_time FROM shifts WHERE shift_id=1");
        if (sq.exec() && sq.next()) {
            start = sq.value(0).toString();
        }
    }
    QString status = (now > start) ? "迟到" : "正常";

    bool insertOk = false;
    QString err;
    {
        QSqlQuery q;
        q.prepare("INSERT INTO attendances(emp_id,att_date,clock_in,status) VALUES(?,?,?,?)");
        q.addBindValue(m_empId); q.addBindValue(today); q.addBindValue(now); q.addBindValue(status);
        insertOk = q.exec();
        if (!insertOk) err = q.lastError().text();
    }

    if (insertOk) {
        Toast::show(this, QString("上班打卡成功 (%1)").arg(now), Toast::Success);
        emit logRequested("上班打卡", today);
        refresh();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "失败", err);
    }
}

void DailyClockWidget::clockOut()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString now = QTime::currentTime().toString("HH:mm:ss");

    bool hasInRecord = false;
    QString currentStatus;
    {
        QSqlQuery q;
        q.prepare("SELECT att_id, status FROM attendances WHERE emp_id=? AND att_date=?");
        q.addBindValue(m_empId); q.addBindValue(today);
        if (q.exec() && q.next()) {
            currentStatus = q.value(1).toString();
            hasInRecord = true;
        }
    }
    if (!hasInRecord) { Toast::show(this, "请先打上班卡", Toast::Warning); return; }

    QString end = "18:00:00";
    {
        QSqlQuery sq("SELECT end_time FROM shifts WHERE shift_id=1");
        if (sq.exec() && sq.next()) {
            end = sq.value(0).toString();
        }
    }
    QString status = (now < end) ? "早退" : "正常";
    if (currentStatus == "迟到") status = "迟到";
    if (status == "正常" && currentStatus == "正常") status = "正常";

    bool updateOk = false;
    QString err;
    {
        QSqlQuery q;
        q.prepare("UPDATE attendances SET clock_out=?, status=? WHERE emp_id=? AND att_date=?");
        q.addBindValue(now); q.addBindValue(status); q.addBindValue(m_empId); q.addBindValue(today);
        updateOk = q.exec();
        if (!updateOk) err = q.lastError().text();
    }

    if (updateOk) {
        Toast::show(this, QString("下班打卡成功 (%1) [%2]").arg(now, status), Toast::Success);
        emit logRequested("下班打卡", today);
        refresh();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "失败", err);
    }
}
