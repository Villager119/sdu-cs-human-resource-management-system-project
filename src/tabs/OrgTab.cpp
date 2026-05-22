#include "OrgTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

OrgTab::OrgTab(std::function<void(const QString&, const QString&)> logFn,
               QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    QSqlQuery q;
    for (auto &col : {"parent_id", "manager_id"}) {
        q.exec(QString("SHOW COLUMNS FROM departments LIKE '%1'").arg(col));
        if (!q.next())
            q.exec(QString("ALTER TABLE departments ADD COLUMN %1 INT DEFAULT NULL REFERENCES employees(emp_id)").arg(col));
    }

    // 右侧员工列表
    m_empModel = new QSqlTableModel(this);
    m_empModel->setHeaderData(1, Qt::Horizontal, "姓名");
    m_empModel->setHeaderData(3, Qt::Horizontal, "电话");
    m_empModel->setHeaderData(4, Qt::Horizontal, "部门");
    m_empModel->setHeaderData(7, Qt::Horizontal, "薪资");
    m_empModel->setHeaderData(10, Qt::Horizontal, "状态");
    m_empTable = new QTableView;
    m_empTable->setModel(m_empModel);
    m_empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_empTable->hideColumn(0); m_empTable->hideColumn(2); m_empTable->hideColumn(5);
    m_empTable->hideColumn(6); m_empTable->hideColumn(8); m_empTable->hideColumn(9);
    m_empTable->hideColumn(11); m_empTable->hideColumn(12); m_empTable->hideColumn(13);

    // 左侧部门树
    m_tree = new QTreeView;
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeModel = new QStandardItemModel(this);
    m_tree->setModel(m_treeModel);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(m_tree);
    m_splitter->addWidget(m_empTable);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 5);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter, 1);

    auto *panel = new QGroupBox("部门管理");
    auto *form = new QFormLayout(panel);
    m_nameEdit = new QLineEdit; m_nameEdit->setPlaceholderText("部门名称");
    form->addRow("名称:", m_nameEdit);
    m_parentCombo = new QComboBox; m_parentCombo->addItem("(无-顶级)", QVariant());
    form->addRow("上级:", m_parentCombo);
    m_managerCombo = new QComboBox; m_managerCombo->addItem("(无)", QVariant());
    form->addRow("负责人:", m_managerCombo);
    auto *btnRow = new QHBoxLayout;
    auto *btnSave = new QPushButton("保存部门");
    btnSave->setStyleSheet("QPushButton{background:#1a2233;color:#fff;} QPushButton:hover{background:#2a3a55;}");
    auto *btnDel = new QPushButton("删除");
    btnDel->setStyleSheet("QPushButton{color:#d14343;} QPushButton:hover{background:#d14343;color:#fff;}");
    btnRow->addWidget(btnSave); btnRow->addWidget(btnDel);
    form->addRow(btnRow);
    layout->addWidget(panel);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &OrgTab::onTreeSelectionChanged);
    connect(btnSave, &QPushButton::clicked, this, &OrgTab::saveDepartment);
    connect(btnDel, &QPushButton::clicked, this, &OrgTab::removeDepartment);

    refresh();
}

void OrgTab::refresh()
{
    m_treeModel->clear();
    m_parentCombo->clear(); m_parentCombo->addItem("(无-顶级)", QVariant());
    m_managerCombo->clear(); m_managerCombo->addItem("(无)", QVariant());

    QSqlQuery eq("SELECT emp_id, name FROM employees");
    while (eq.next()) m_managerCombo->addItem(eq.value(1).toString(), eq.value(0).toInt());

    // 一次查询获取所有部门的员工计数
    QMap<QString, int> empCountMap;
    {
        QSqlQuery eqc("SELECT department, COUNT(*) FROM employees WHERE department IS NOT NULL AND department!='' GROUP BY department");
        while (eqc.next())
            empCountMap[eqc.value(0).toString()] = eqc.value(1).toInt();
    }

    QMap<int, QStandardItem *> items;
    QMap<int, int> parentMap;
    QSqlQuery dq("SELECT dept_id, dept_name, parent_id FROM departments ORDER BY dept_id");
    while (dq.next()) {
        int id = dq.value(0).toInt();
        QString name = dq.value(1).toString();
        int pid = dq.value(2).isNull() ? -1 : dq.value(2).toInt();
        parentMap[id] = pid;
        int cnt = empCountMap.value(name, 0);
        auto *item = new QStandardItem(QString("%1  (%2人)").arg(name).arg(cnt));
        item->setData(id, Qt::UserRole);
        items[id] = item;
        m_parentCombo->addItem(name, id);
    }

    for (auto it = items.begin(); it != items.end(); ++it) {
        int pid = parentMap.value(it.key(), -1);
        if (pid == -1 || !items.contains(pid)) m_treeModel->appendRow(it.value());
        else items[pid]->appendRow(it.value());
    }
    m_tree->setModel(m_treeModel);
    m_tree->expandAll();
}

void OrgTab::onTreeSelectionChanged()
{
    auto *item = m_treeModel->itemFromIndex(m_tree->currentIndex());
    if (!item) { m_selectedDeptId = -1; m_empTable->setModel(nullptr); return; }
    m_selectedDeptId = item->data(Qt::UserRole).toInt();

    QSqlQuery q; q.prepare("SELECT dept_name, parent_id, manager_id FROM departments WHERE dept_id=?");
    q.addBindValue(m_selectedDeptId); q.exec();
    if (q.next()) {
        QString name = q.value(0).toString();
        m_nameEdit->setText(name);
        int pid = q.value(1).isNull() ? 0 : m_parentCombo->findData(q.value(1).toInt());
        m_parentCombo->setCurrentIndex(pid > 0 ? pid : 0);
        int mid = q.value(2).isNull() ? 0 : m_managerCombo->findData(q.value(2).toInt());
        m_managerCombo->setCurrentIndex(mid > 0 ? mid : 0);
        // 显示该部门员工
        m_empModel->setTable("employees");
        m_empModel->setFilter(QString("department='%1'").arg(name.replace("'", "''")));
        m_empModel->select();
        m_empTable->setModel(m_empModel);
    }
}

void OrgTab::saveDepartment()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) { QMessageBox::warning(this, "提示", "请输入部门名称"); return; }
    QVariant pid = m_parentCombo->currentData(), mid = m_managerCombo->currentData();
    QSqlQuery q;
    if (m_selectedDeptId > 0) {
        q.prepare("UPDATE departments SET dept_name=?,parent_id=?,manager_id=? WHERE dept_id=?");
        q.addBindValue(name);
        pid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(pid.toInt());
        mid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(mid.toInt());
        q.addBindValue(m_selectedDeptId);
        if (!q.exec()) { QMessageBox::critical(this,"错误",q.lastError().text()); return; }
        m_log("修改部门", name);
    } else {
        q.prepare("INSERT IGNORE INTO departments(dept_name,parent_id,manager_id) VALUES(?,?,?)");
        q.addBindValue(name);
        pid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(pid.toInt());
        mid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(mid.toInt());
        if (!q.exec()) { QMessageBox::critical(this,"错误",q.lastError().text()); return; }
        m_log("新增部门", name);
    }
    QMessageBox::information(this,"成功","部门信息已保存");
    m_selectedDeptId = -1; m_nameEdit->clear(); refresh();
}

void OrgTab::removeDepartment()
{
    if (m_selectedDeptId <= 0) { QMessageBox::warning(this,"提示","请先选中要删除的部门"); return; }
    if (QMessageBox::question(this,"确认",QString("确定删除\"%1\"吗？子部门将移为顶级。").arg(m_nameEdit->text()),
                               QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;
    QSqlQuery q;
    q.prepare("UPDATE departments SET parent_id=NULL WHERE parent_id=?"); q.addBindValue(m_selectedDeptId); q.exec();
    q.prepare("DELETE FROM departments WHERE dept_id=?"); q.addBindValue(m_selectedDeptId); q.exec();
    m_log("删除部门", m_nameEdit->text());
    m_selectedDeptId = -1; m_nameEdit->clear(); refresh();
}
