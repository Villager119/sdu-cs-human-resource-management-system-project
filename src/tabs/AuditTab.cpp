#include "AuditTab.h"

AuditTab::AuditTab(QWidget *parent)
    : QWidget(parent)
{
    // 建表
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS audit_logs ("
           "log_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL,"
           "emp_name VARCHAR(50) NOT NULL,"
           "action VARCHAR(100) NOT NULL,"
           "target VARCHAR(200) DEFAULT '',"
           "log_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

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
