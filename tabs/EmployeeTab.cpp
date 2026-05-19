#include "EmployeeTab.h"
#include "../widgets/PaginationBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QCryptographicHash>
#include <QTextStream>

EmployeeTab::EmployeeTab(std::function<void(const QString&, const QString&)> logFn,
                         QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 扩展字段迁移
    QSqlQuery q;
    for (auto &col : {"education", "marital_status", "position"}) {
        q.exec(QString("SHOW COLUMNS FROM employees LIKE '%1'").arg(col));
        if (q.size() == 0)
            q.exec(QString("ALTER TABLE employees ADD COLUMN %1 VARCHAR(50) DEFAULT '' AFTER status").arg(col));
    }
    q.exec("SHOW COLUMNS FROM employees LIKE 'contract_end_date'");
    if (q.size() == 0)
        q.exec("ALTER TABLE employees ADD COLUMN contract_end_date DATE DEFAULT NULL AFTER hire_date");

    // 部门表
    q.exec("CREATE TABLE IF NOT EXISTS departments (dept_id INT PRIMARY KEY AUTO_INCREMENT, dept_name VARCHAR(50) NOT NULL UNIQUE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("INSERT IGNORE INTO departments (dept_name) SELECT DISTINCT department FROM employees WHERE department IS NOT NULL AND department!=''");

    m_model = new QSqlTableModel(this);
    m_model->setTable("employees");
    m_model->setHeaderData(0, Qt::Horizontal, "员工编号");
    m_model->setHeaderData(1, Qt::Horizontal, "姓名");
    m_model->setHeaderData(2, Qt::Horizontal, "性别");
    m_model->setHeaderData(3, Qt::Horizontal, "联系电话");
    m_model->setHeaderData(4, Qt::Horizontal, "所属部门");
    m_model->setHeaderData(5, Qt::Horizontal, "系统角色");
    m_model->setHeaderData(7, Qt::Horizontal, "基础薪资");
    m_model->setHeaderData(8, Qt::Horizontal, "入职日期");
    m_model->setHeaderData(13, Qt::Horizontal, "合同到期");
    m_model->setHeaderData(9, Qt::Horizontal, "在职状态");
    m_model->setHeaderData(10, Qt::Horizontal, "学历");
    m_model->setHeaderData(11, Qt::Horizontal, "婚姻状况");
    m_model->setHeaderData(12, Qt::Horizontal, "岗位");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->hideColumn(6);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 筛选栏
    auto *filter = new QHBoxLayout;
    m_deptCombo = new QComboBox;
    m_deptCombo->addItem("全部部门");
    filter->addWidget(new QLabel("部门:"));
    filter->addWidget(m_deptCombo);
    m_statusCombo = new QComboBox;
    m_statusCombo->addItems({"全部状态", "在职", "离职"});
    filter->addWidget(new QLabel("状态:"));
    filter->addWidget(m_statusCombo);
    m_nameSearch = new QLineEdit;
    m_nameSearch->setPlaceholderText("输入姓名搜索...");
    filter->addWidget(new QLabel("姓名:"));
    filter->addWidget(m_nameSearch);
    auto *btnSearch = new QPushButton("查询");
    auto *btnReset = new QPushButton("重置");
    filter->addWidget(btnSearch);
    filter->addWidget(btnReset);
    layout->addLayout(filter);

    layout->addWidget(m_table, 1);

    m_pagination = new PaginationBar(20, this);
    layout->addWidget(m_pagination);
    connect(m_pagination, &PaginationBar::pageChanged, this, [this](int page) {
        Q_UNUSED(page);
        m_table->scrollToTop();
    });

    // 操作按钮行
    auto *btnRow = new QGridLayout;
    auto *btnAdd = new QPushButton("添加员工");
    auto *btnDel = new QPushButton("删除选中行");
    auto *btnRevert = new QPushButton("撤销修改");
    auto *btnSave = new QPushButton("保存修改");
    m_btnToggleStatus = new QPushButton("标记离职");
    m_btnToggleStatus->setEnabled(false);
    auto *btnCSV = new QPushButton("导出CSV");
    btnRow->addWidget(btnAdd, 0, 0);
    btnRow->addWidget(btnDel, 0, 1);
    btnRow->addWidget(btnRevert, 0, 2);
    btnRow->addWidget(btnSave, 0, 3);
    btnRow->addWidget(m_btnToggleStatus, 0, 4);
    btnRow->addWidget(btnCSV, 0, 5);
    layout->addLayout(btnRow);

    // 填充部门下拉
    QSqlQuery dq("SELECT dept_name FROM departments ORDER BY dept_id");
    while (dq.next()) m_deptCombo->addItem(dq.value(0).toString());

    // 选择变化更新按钮文字
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &cur, const QModelIndex &) {
        if (!cur.isValid()) { m_btnToggleStatus->setText("标记离职"); m_btnToggleStatus->setEnabled(false); return; }
        m_btnToggleStatus->setEnabled(true);
        m_btnToggleStatus->setText(m_model->data(m_model->index(cur.row(), 9)).toString()=="在职" ? "标记离职" : "标记在职");
    });

    connect(btnAdd, &QPushButton::clicked, this, &EmployeeTab::add);
    connect(btnDel, &QPushButton::clicked, this, &EmployeeTab::remove);
    connect(btnSave, &QPushButton::clicked, this, &EmployeeTab::save);
    connect(btnRevert, &QPushButton::clicked, this, &EmployeeTab::revert);
    connect(m_btnToggleStatus, &QPushButton::clicked, this, &EmployeeTab::toggleStatus);
    connect(btnSearch, &QPushButton::clicked, this, &EmployeeTab::search);
    connect(btnReset, &QPushButton::clicked, this, &EmployeeTab::resetFilter);
    connect(btnCSV, &QPushButton::clicked, this, &EmployeeTab::exportCSV);

    m_model->select();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::add()
{
    int rc = m_model->rowCount();
    m_model->insertRow(rc);
    m_table->setCurrentIndex(m_model->index(rc, 1));
    m_log("新增员工记录", "");
}

void EmployeeTab::remove()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请先在表格中选中要删除的员工！"); return; }
    QString name = m_model->data(m_model->index(row, 1)).toString();
    m_model->removeRow(row);
    m_log("删除员工记录", name);
}

void EmployeeTab::save()
{
    // 新员工无密码时自动注入默认密码 123456 的 SHA-256 哈希
    static const QString defaultPwdHash = QString(QCryptographicHash::hash("123456", QCryptographicHash::Sha256).toHex());
    for (int r = 0; r < m_model->rowCount(); r++) {
        if (m_model->data(m_model->index(r, 6)).toString().isEmpty())
            m_model->setData(m_model->index(r, 6), defaultPwdHash);
    }
    if (m_model->submitAll()) { m_log("保存员工信息修改", ""); QMessageBox::information(this, "成功", "所有数据修改已成功"); }
    else QMessageBox::critical(this, "失败", "保存失败: " + m_model->lastError().text());
}

void EmployeeTab::revert() { m_model->revertAll(); }

void EmployeeTab::toggleStatus()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请先在表格中选中员工！"); return; }
    QString s = m_model->data(m_model->index(row, 9)).toString();
    QString ns = (s == "在职") ? "离职" : "在职";
    m_model->setData(m_model->index(row, 9), ns);
    QString name = m_model->data(m_model->index(row, 1)).toString();
    m_log("变更员工状态", name + " → " + ns);
    m_btnToggleStatus->setText(ns == "在职" ? "标记离职" : "标记在职");
}

void EmployeeTab::search()
{
    QStringList cond;
    if (m_deptCombo->currentIndex() > 1) cond << QString("department='%1'").arg(m_deptCombo->currentText());
    if (m_statusCombo->currentIndex() > 0) cond << QString("status='%1'").arg(m_statusCombo->currentText());
    if (!m_nameSearch->text().isEmpty()) {
        QString escaped = m_nameSearch->text();
        escaped.replace("'", "''"); // 防 SQL 注入：转义单引号
        cond << QString("name LIKE '%%1%'").arg(escaped);
    }
    m_model->setFilter(cond.isEmpty() ? "" : cond.join(" AND "));
    m_model->select();
    m_pagination->refresh();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::resetFilter()
{
    m_deptCombo->setCurrentIndex(0);
    m_statusCombo->setCurrentIndex(0);
    m_nameSearch->clear();
    m_model->setFilter("");
    m_model->select();
    m_pagination->refresh();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出员工表", "员工信息.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";
    for (int c = 0; c < m_model->columnCount(); c++) {
        if (c == 6) continue;
        out << "\"" << m_model->headerData(c, Qt::Horizontal).toString() << "\"" << (c < m_model->columnCount()-1 ? "," : "\n");
    }
    for (int r = 0; r < m_model->rowCount(); r++) {
        for (int c = 0; c < m_model->columnCount(); c++) {
            if (c == 6) continue;
            out << "\"" << m_model->data(m_model->index(r, c)).toString().replace("\"", "\"\"") << "\"" << (c < m_model->columnCount()-1 ? "," : "\n");
        }
    }
    f.close();
    QMessageBox::information(this, "导出成功", "员工信息已导出至:\n" + path);
}
