#include "AuditTab.h"
#include <QVBoxLayout>
#include <QSqlQuery>

AuditTab::AuditTab(QWidget *parent)
    : QWidget(parent)
{
    m_model = new QSqlTableModel(this);
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
    layout->addWidget(m_table);

    m_model->select();
}

void AuditTab::refresh()
{
    m_model->select();
}
