#include "AuditTab.h"
#include "../widgets/PaginationBar.h"
#include "../utils/DbUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

AuditTab::AuditTab(QWidget *parent)
    : QWidget(parent)
{
    m_model = new QSqlTableModel(this, createClonedDatabaseConnection("audit_model"));
    m_model->setTable("audit_logs");
    m_model->setHeaderData(0, Qt::Horizontal, "日志编号");
    m_model->setHeaderData(1, Qt::Horizontal, "操作人ID");
    m_model->setHeaderData(2, Qt::Horizontal, "操作人姓名");
    m_model->setHeaderData(3, Qt::Horizontal, "操作动作");
    m_model->setHeaderData(4, Qt::Horizontal, "操作对象");
    m_model->setHeaderData(5, Qt::Horizontal, "操作时间");
    m_model->setSort(5, Qt::DescendingOrder);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->hideColumn(1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *filterLayout = new QHBoxLayout;

    m_keywordEdit = new QLineEdit(this);
    m_keywordEdit->setPlaceholderText("操作人/动作/对象/员工ID");
    m_keywordEdit->setFixedWidth(180);
    filterLayout->addWidget(new QLabel("关键字:", this));
    filterLayout->addWidget(m_keywordEdit);

    const QDate minDate(2000, 1, 1);
    m_startDateEdit = new QDateEdit(minDate, this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setMinimumDate(minDate);
    m_startDateEdit->setSpecialValueText("不限");
    filterLayout->addWidget(new QLabel("开始:", this));
    filterLayout->addWidget(m_startDateEdit);

    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setMinimumDate(minDate);
    filterLayout->addWidget(new QLabel("结束:", this));
    filterLayout->addWidget(m_endDateEdit);

    auto *btnSearch = new QPushButton("查询", this);
    auto *btnReset = new QPushButton("重置", this);
    filterLayout->addWidget(btnSearch);
    filterLayout->addWidget(btnReset);
    filterLayout->addStretch();

    layout->addLayout(filterLayout);
    layout->addWidget(m_table);

    m_pagination = new PaginationBar(20, this);
    layout->addWidget(m_pagination);

    connect(btnSearch, &QPushButton::clicked, this, [this]() {
        loadAuditPage(true);
    });
    connect(btnReset, &QPushButton::clicked, this, [this, minDate]() {
        m_keywordEdit->clear();
        m_startDateEdit->setDate(minDate);
        m_endDateEdit->setDate(QDate::currentDate());
        loadAuditPage(true);
    });
    connect(m_keywordEdit, &QLineEdit::returnPressed, this, [this]() {
        loadAuditPage(true);
    });
    connect(m_pagination, &PaginationBar::pageChanged, this, [this](int) {
        loadAuditPage(false);
    });

    loadAuditPage(true);
}

void AuditTab::refresh()
{
    loadAuditPage(false);
}

QString AuditTab::auditBaseFilter() const
{
    QStringList conds;
    const QDate minDate(2000, 1, 1);

    const QString keyword = m_keywordEdit ? m_keywordEdit->text().trimmed() : QString();
    if (!keyword.isEmpty()) {
        QString escaped = keyword;
        escaped.replace("'", "''");
        conds << QString("(emp_name LIKE '%%1%' OR action LIKE '%%1%' OR target LIKE '%%1%' "
                         "OR CAST(emp_id AS CHAR) LIKE '%%1%')")
                     .arg(escaped);
    }

    if (m_startDateEdit && m_endDateEdit && m_startDateEdit->date() > m_endDateEdit->date()) {
        conds << "1=0";
    } else {
        if (m_startDateEdit && m_startDateEdit->date() > minDate) {
            conds << QString("log_time >= '%1 00:00:00'")
                         .arg(m_startDateEdit->date().toString("yyyy-MM-dd"));
        }
        if (m_endDateEdit && m_endDateEdit->date() >= minDate) {
            conds << QString("log_time <= '%1 23:59:59'")
                         .arg(m_endDateEdit->date().toString("yyyy-MM-dd"));
        }
    }

    return conds.join(" AND ");
}

QString AuditTab::auditPagedFilter(QString *errorText) const
{
    if (errorText) errorText->clear();
    QString filter = auditBaseFilter();
    if (filter.isEmpty()) filter = "1=1";

    const int pageSize = 20;
    const int offset = m_pagination ? (m_pagination->currentPage() - 1) * pageSize : 0;

    QSqlQuery query;
    query.prepare(QString("SELECT log_id FROM audit_logs WHERE %1 "
                          "ORDER BY log_time DESC, log_id DESC LIMIT %2 OFFSET %3")
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
    return QString("log_id IN (%1)").arg(ids.join(","));
}

int AuditTab::filteredAuditCount(QString *errorText) const
{
    if (errorText) errorText->clear();
    QString filter = auditBaseFilter();
    if (filter.isEmpty()) filter = "1=1";

    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM audit_logs WHERE " + filter);

    int total = 0;
    if (query.exec() && query.next()) {
        total = query.value(0).toInt();
    } else if (errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return total;
}

void AuditTab::loadAuditPage(bool resetPage)
{
    if (!m_model || !m_pagination) return;
    if (resetPage) {
        m_pagination->refresh();
    }

    QString errorText;
    const int total = filteredAuditCount(&errorText);
    if (!errorText.isEmpty()) {
        QMessageBox::warning(this, "分页失败", "统计日志数量失败：" + errorText);
        return;
    }

    m_pagination->setTotalRecords(total);
    const QString filter = auditPagedFilter(&errorText);
    if (!errorText.isEmpty()) {
        QMessageBox::warning(this, "分页失败", "读取日志分页数据失败：" + errorText);
        return;
    }
    m_model->setFilter(filter);
    m_model->setSort(5, Qt::DescendingOrder);
    if (!m_model->select()) {
        QMessageBox::warning(this, "分页失败", "加载日志列表失败：" + m_model->lastError().text());
    }
}
