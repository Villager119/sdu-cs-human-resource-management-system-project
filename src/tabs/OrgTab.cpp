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
    // 部门表扩展迁移
    QSqlQuery q;
    for (auto &col : {"parent_id", "manager_id"}) {
        q.exec(QString("SHOW COLUMNS FROM departments LIKE '%1'").arg(col));
        if (q.size() == 0) {
            if (QString(col) == "parent_id")
                q.exec("ALTER TABLE departments ADD COLUMN parent_id INT DEFAULT NULL REFERENCES departments(dept_id)");
            else
                q.exec("ALTER TABLE departments ADD COLUMN manager_id INT DEFAULT NULL REFERENCES employees(emp_id)");
        }
    }

    m_tree = new QTreeView;
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeModel = new QStandardItemModel(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tree, 1);

    // 管理表单
    auto *panel = new QGroupBox("部门管理");
    auto *form = new QFormLayout(panel);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("部门名称");
    form->addRow("名称:", m_nameEdit);

    m_parentCombo = new QComboBox;
    m_parentCombo->addItem("(无 - 顶级部门)", QVariant());
    form->addRow("上级部门:", m_parentCombo);

    m_managerCombo = new QComboBox;
    m_managerCombo->addItem("(无)", QVariant());
    form->addRow("负责人:", m_managerCombo);

    auto *btnRow = new QHBoxLayout;
    auto *btnSave = new QPushButton("保存部门");
    btnSave->setStyleSheet("QPushButton { background: #1e3a5f; color: #fff; } QPushButton:hover { background: #2a5080; }");
    auto *btnDel = new QPushButton("删除选中部门");
    btnDel->setStyleSheet("QPushButton { color: #d14343; } QPushButton:hover { color: #fff; background: #d14343; }");
    btnRow->addWidget(btnSave);
    btnRow->addWidget(btnDel);
    form->addRow(btnRow);

    layout->addWidget(panel);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &OrgTab::onTreeSelectionChanged);
    connect(btnSave, &QPushButton::clicked, this, &OrgTab::saveDepartment);
    connect(btnDel, &QPushButton::clicked, this, &OrgTab::removeDepartment);

    refresh();
}

void OrgTab::refresh()
{
    // 重建树
    m_treeModel->clear();
    m_managerCombo->clear();
    m_managerCombo->addItem("(无)", QVariant());
    m_parentCombo->clear();
    m_parentCombo->addItem("(无 - 顶级部门)", QVariant());

    QSqlQuery eq("SELECT emp_id, name FROM employees");
    while (eq.next())
        m_managerCombo->addItem(eq.value(1).toString(), eq.value(0).toInt());

    QSqlQuery pq("SELECT dept_id, dept_name FROM departments ORDER BY dept_id");
    QMap<int, QStandardItem *> items;
    while (pq.next()) {
        int id = pq.value(0).toInt();
        auto *item = new QStandardItem(pq.value(1).toString());
        item->setData(id, Qt::UserRole);
        items[id] = item;
        m_parentCombo->addItem(pq.value(1).toString(), id);
    }

    // 按 parent_id 构建树层级
    QSqlQuery dq("SELECT dept_id, parent_id FROM departments");
    QMap<int, int> parentMap;
    while (dq.next())
        parentMap[dq.value(0).toInt()] = dq.value(1).isNull() ? -1 : dq.value(1).toInt();

    for (auto it = items.begin(); it != items.end(); ++it) {
        int pid = parentMap.value(it.key(), -1);
        if (pid == -1 || !items.contains(pid))
            m_treeModel->appendRow(it.value());
        else
            items[pid]->appendRow(it.value());
    }
    m_tree->setModel(m_treeModel);
    m_tree->expandAll();
}

void OrgTab::onTreeSelectionChanged()
{
    auto *item = m_treeModel->itemFromIndex(m_tree->currentIndex());
    if (!item) { m_selectedDeptId = -1; m_nameEdit->clear(); return; }
    m_selectedDeptId = item->data(Qt::UserRole).toInt();

    QSqlQuery q;
    q.prepare("SELECT dept_name, parent_id, manager_id FROM departments WHERE dept_id=?");
    q.addBindValue(m_selectedDeptId); q.exec();
    if (q.next()) {
        m_nameEdit->setText(q.value(0).toString());
        int pid = q.value(1).isNull() ? 0 : m_parentCombo->findData(q.value(1).toInt());
        m_parentCombo->setCurrentIndex(pid > 0 ? pid : 0);
        int mid = q.value(2).isNull() ? 0 : m_managerCombo->findData(q.value(2).toInt());
        m_managerCombo->setCurrentIndex(mid > 0 ? mid : 0);
    }
}

void OrgTab::saveDepartment()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) { QMessageBox::warning(this, "提示", "请输入部门名称"); return; }
    QVariant pid = m_parentCombo->currentData();
    QVariant mid = m_managerCombo->currentData();

    QSqlQuery q;
    if (m_selectedDeptId > 0) {
        // 修改现有部门
        q.prepare("UPDATE departments SET dept_name=?, parent_id=?, manager_id=? WHERE dept_id=?");
        q.addBindValue(name);
        pid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(pid.toInt());
        mid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(mid.toInt());
        q.addBindValue(m_selectedDeptId);
        if (!q.exec()) { QMessageBox::critical(this, "错误", q.lastError().text()); return; }
        m_log("修改部门", name);
    } else {
        // 新增部门
        q.prepare("INSERT IGNORE INTO departments(dept_name,parent_id,manager_id) VALUES(?,?,?)");
        q.addBindValue(name);
        pid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(pid.toInt());
        mid.isNull() ? q.addBindValue(QVariant(QMetaType(QMetaType::Int))) : q.addBindValue(mid.toInt());
        if (!q.exec()) { QMessageBox::critical(this, "错误", q.lastError().text()); return; }
        m_log("新增部门", name);
    }
    QMessageBox::information(this, "成功", "部门信息已保存");
    m_selectedDeptId = -1;
    m_nameEdit->clear();
    refresh();
}

void OrgTab::removeDepartment()
{
    if (m_selectedDeptId <= 0) { QMessageBox::warning(this, "提示", "请先选中要删除的部门"); return; }
    QString name = m_nameEdit->text();
    if (QMessageBox::question(this, "确认删除", QString("确定删除部门 \"%1\" 吗？\n子部门将移为顶级部门。").arg(name),
                               QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    QSqlQuery q;
    q.prepare("UPDATE departments SET parent_id=NULL WHERE parent_id=?");
    q.addBindValue(m_selectedDeptId); q.exec();
    q.prepare("DELETE FROM departments WHERE dept_id=?");
    q.addBindValue(m_selectedDeptId);
    if (q.exec()) {
        m_log("删除部门", name);
        QMessageBox::information(this, "成功", "部门已删除");
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
    m_selectedDeptId = -1;
    m_nameEdit->clear();
    refresh();
}
