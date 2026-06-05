#include "AttendBoardWidget.h"
#include "../../widgets/CommonDelegates.h"
#include "../../widgets/PaginationBar.h"
#include "../../utils/Toast.h"
#include "../../utils/DbUtils.h"
#include "../../utils/CsvExport.h"
#include "../../services/AttendanceService.h"
#include "../../core/Constants.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

AttendBoardWidget::AttendBoardWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(15, 15, 15, 15);
    l->setSpacing(12);

    // Filters Bar
    auto *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("开始日期:", this));
    m_startDateFilter = new QDateEdit(this);
    m_startDateFilter->setCalendarPopup(true);
    m_startDateFilter->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
    filterLayout->addWidget(m_startDateFilter);

    filterLayout->addWidget(new QLabel("结束日期:", this));
    m_endDateFilter = new QDateEdit(this);
    m_endDateFilter->setCalendarPopup(true);
    m_endDateFilter->setDate(QDate::currentDate());
    filterLayout->addWidget(m_endDateFilter);

    filterLayout->addWidget(new QLabel("范围:", this));
    m_scopeFilter = new QComboBox(this);
    m_scopeFilter->addItem("我的记录", "mine");
    m_scopeFilter->addItem("全部员工", "all");
    filterLayout->addWidget(m_scopeFilter);

    filterLayout->addWidget(new QLabel("姓名:", this));
    m_nameFilter = new QLineEdit(this);
    m_nameFilter->setPlaceholderText("搜索姓名...");
    m_nameFilter->setFixedWidth(100);
    filterLayout->addWidget(m_nameFilter);

    filterLayout->addWidget(new QLabel("状态:", this));
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItems({"全部状态", "正常", "迟到", "早退", "迟到/早退", "缺卡", "请假"});
    filterLayout->addWidget(m_statusFilter);

    QPushButton *btnQuery = new QPushButton("查询", this);
    btnQuery->setStyleSheet("QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #1d4ed8; }");
    filterLayout->addWidget(btnQuery);

    QPushButton *btnReset = new QPushButton("重置", this);
    btnReset->setStyleSheet("QPushButton { background-color: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #e2e8f0; }");
    filterLayout->addWidget(btnReset);

    filterLayout->addStretch();

    QPushButton *btnExport = new QPushButton("导出报表", this);
    btnExport->setStyleSheet("QPushButton { background-color: #0284c7; color: white; border: none; border-radius: 6px; padding: 6px 12px; } QPushButton:hover { background-color: #0ea5e9; }");
    filterLayout->addWidget(btnExport);

    l->addLayout(filterLayout);

    // Records Table
    m_attModel = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("attendance_board_model"));
    m_attModel->setTable("attendances");
    m_attModel->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_attModel->setHeaderData(0, Qt::Horizontal, "编号");
    m_attModel->setHeaderData(1, Qt::Horizontal, "员工");
    m_attModel->setHeaderData(2, Qt::Horizontal, "日期");
    m_attModel->setHeaderData(3, Qt::Horizontal, "签到时间");
    m_attModel->setHeaderData(4, Qt::Horizontal, "签退时间");
    m_attModel->setHeaderData(5, Qt::Horizontal, "状态");
    m_attModel->setHeaderData(6, Qt::Horizontal, "备注");

    m_attTable = new QTableView(this);
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
    l->addWidget(m_attTable, 1);

    m_pagination = new PaginationBar(20, this);
    l->addWidget(m_pagination);

    connect(btnQuery, &QPushButton::clicked, this, &AttendBoardWidget::filterAttendance);
    connect(btnReset, &QPushButton::clicked, this, &AttendBoardWidget::resetFilters);
    connect(btnExport, &QPushButton::clicked, this, &AttendBoardWidget::exportCsv);
    connect(m_scopeFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        filterAttendance();
    });
    connect(m_pagination, &PaginationBar::pageChanged, this, [this](int) {
        QString pageError;
        const QString filter = pagedFilter(&pageError);
        if (!pageError.isEmpty()) {
            Toast::show(this, "读取考勤分页数据失败：" + pageError, Toast::Warning);
            return;
        }
        m_attModel->setFilter(filter);
        m_attModel->setSort(2, Qt::DescendingOrder);
        if (!m_attModel->select()) {
            Toast::show(this, "加载考勤列表失败：" + m_attModel->lastError().text(), Toast::Warning);
        }
    });
}

void AttendBoardWidget::refresh()
{
    filterAttendance();
}

void AttendBoardWidget::filterAttendance()
{
    QString backfillError;
    const bool ownScope = m_scopeFilter && m_scopeFilter->currentData().toString() == "mine";
    const int backfillEmpId = ownScope ? m_empId : 0;
    if (!AttendanceService().backfillAttendanceRange(m_startDateFilter->date(), m_endDateFilter->date(),
                                                     &backfillError, backfillEmpId)) {
        Toast::show(this, "考勤状态回填失败：" + backfillError, Toast::Warning);
    }

    if (m_pagination) {
        QString countError;
        m_pagination->refresh();
        const int total = filteredAttendanceCount(&countError);
        if (!countError.isEmpty()) {
            Toast::show(this, "统计考勤记录失败：" + countError, Toast::Warning);
            return;
        }
        m_pagination->setTotalRecords(total);
    }
    QString pageError;
    const QString filter = pagedFilter(&pageError);
    if (!pageError.isEmpty()) {
        Toast::show(this, "读取考勤分页数据失败：" + pageError, Toast::Warning);
        return;
    }
    m_attModel->setFilter(filter);
    m_attModel->setSort(2, Qt::DescendingOrder);
    if (!m_attModel->select()) {
        Toast::show(this, "加载考勤列表失败：" + m_attModel->lastError().text(), Toast::Warning);
    }
}

void AttendBoardWidget::resetFilters()
{
    m_startDateFilter->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
    m_endDateFilter->setDate(QDate::currentDate());
    m_scopeFilter->setCurrentIndex(0);
    m_nameFilter->clear();
    m_statusFilter->setCurrentIndex(0);
    filterAttendance();
}

void AttendBoardWidget::exportCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "导出考勤报表", "考勤报表.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;

    QSqlQuery query;
    query.prepare(QString("SELECT employees.name, attendances.att_date, attendances.clock_in, "
                          "attendances.clock_out, attendances.status, attendances.remark "
                          "FROM attendances "
                          "JOIN employees ON employees.emp_id=attendances.emp_id "
                          "WHERE %1 "
                          "ORDER BY attendances.att_date DESC, attendances.att_id DESC")
                      .arg(baseFilter()));
    if (!query.exec()) {
        Toast::show(this, "导出考勤报表失败：" + query.lastError().text(), Toast::Error);
        query.finish();
        return;
    }

    QList<QStringList> rows;
    while (query.next()) {
        rows << QStringList{
            query.value(0).toString(),
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toString(),
            query.value(4).toString(),
            query.value(5).toString()
        };
    }
    query.finish();

    exportRowsToCSVAsync({"员工", "日期", "签到时间", "签退时间", "状态", "备注"}, rows, path, this);
    emit logRequested("导出考勤报表", path);
}

QString AttendBoardWidget::baseFilter() const
{
    QStringList conds;

    const QString start = m_startDateFilter->date().toString("yyyy-MM-dd");
    const QString end = m_endDateFilter->date().toString("yyyy-MM-dd");
    conds << QString("attendances.att_date BETWEEN '%1' AND '%2'").arg(start, end);

    if (m_scopeFilter && m_scopeFilter->currentData().toString() == "mine") {
        conds << QString("attendances.emp_id=%1").arg(m_empId);
    }

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
    return conds.join(" AND ");
}

QString AttendBoardWidget::pagedFilter(QString *errorText) const
{
    if (errorText) errorText->clear();
    const QString filter = baseFilter();
    const int pageSize = 20;
    const int offset = m_pagination ? (m_pagination->currentPage() - 1) * pageSize : 0;

    QSqlQuery query;
    query.prepare(QString("SELECT attendances.att_id FROM attendances WHERE %1 "
                          "ORDER BY attendances.att_date DESC, attendances.att_id DESC "
                          "LIMIT %2 OFFSET %3")
                      .arg(filter)
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
    return QString("%1 AND attendances.att_id IN (%2)").arg(filter, ids.join(","));
}

int AttendBoardWidget::filteredAttendanceCount(QString *errorText) const
{
    if (errorText) errorText->clear();
    QString filter = baseFilter();
    QSqlQuery query;
    if (filter.isEmpty()) {
        query.prepare("SELECT COUNT(*) FROM attendances");
    } else {
        query.prepare("SELECT COUNT(*) FROM attendances WHERE " + filter);
    }

    int total = 0;
    if (query.exec() && query.next()) {
        total = query.value(0).toInt();
    } else if (errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return total;
}
