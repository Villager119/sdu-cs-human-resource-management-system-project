#include "OrgTab.h"
#include "../services/OrgService.h"
#include "../utils/Toast.h"
#include "../utils/DbUtils.h"
#include "../widgets/OrgChartView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QTabWidget>

OrgTab::OrgTab(std::function<void(const QString&, const QString&)> logFn,
               QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 右侧员工列表
    m_empModel = new QSqlTableModel(this, createClonedDatabaseConnection("org_employee_model"));
    m_empModel->setTable("employees");

    // 解析动态列索引
    int idxName = m_empModel->fieldIndex("name");
    int idxPhone = m_empModel->fieldIndex("phone");
    int idxDept = m_empModel->fieldIndex("department");
    int idxSalary = m_empModel->fieldIndex("base_salary");
    int idxStatus = m_empModel->fieldIndex("status");

    if (idxName >= 0) m_empModel->setHeaderData(idxName, Qt::Horizontal, "姓名");
    if (idxPhone >= 0) m_empModel->setHeaderData(idxPhone, Qt::Horizontal, "电话");
    if (idxDept >= 0) m_empModel->setHeaderData(idxDept, Qt::Horizontal, "部门");
    if (idxSalary >= 0) m_empModel->setHeaderData(idxSalary, Qt::Horizontal, "薪资");
    if (idxStatus >= 0) m_empModel->setHeaderData(idxStatus, Qt::Horizontal, "状态");

    m_empTable = new QTableView;
    m_empTable->setModel(m_empModel);
    m_empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_empTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 动态隐藏除了 姓名、电话、部门、薪资、状态 以外的所有列
    QList<int> visibleCols;
    if (idxName >= 0) visibleCols.append(idxName);
    if (idxPhone >= 0) visibleCols.append(idxPhone);
    if (idxDept >= 0) visibleCols.append(idxDept);
    if (idxSalary >= 0) visibleCols.append(idxSalary);
    if (idxStatus >= 0) visibleCols.append(idxStatus);

    for (int i = 0; i < m_empModel->columnCount(); ++i) {
        if (!visibleCols.contains(i)) {
            m_empTable->hideColumn(i);
        }
    }

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

    m_chartView = new OrgChartView(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(m_splitter, "部门结构列表");
    m_tabWidget->addTab(m_chartView, "可视化架构图");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabWidget, 1);

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
    btnSave->setProperty("theme", "dark");
    m_btnDel = new QPushButton("删除");
    m_btnDel->setProperty("theme", "danger");
    m_btnDel->setEnabled(false);
    btnRow->addWidget(btnSave); btnRow->addWidget(m_btnDel);
    form->addRow(btnRow);
    layout->addWidget(panel);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &OrgTab::onTreeSelectionChanged);
    connect(btnSave, &QPushButton::clicked, this, &OrgTab::saveDepartment);
    connect(m_btnDel, &QPushButton::clicked, this, &OrgTab::removeDepartment);

    connect(m_chartView, &OrgChartView::departmentSelected, this, [this](int deptId) {
        for (int i = 0; i < m_treeModel->rowCount(); ++i) {
            QStandardItem *item = m_treeModel->item(i);
            if (item) {
                std::function<bool(QStandardItem*)> findAndSelect = [&](QStandardItem *it) -> bool {
                    if (it->data(Qt::UserRole).toInt() == deptId) {
                        m_tree->setCurrentIndex(it->index());
                        return true;
                    }
                    for (int j = 0; j < it->rowCount(); ++j) {
                        if (findAndSelect(it->child(j))) return true;
                    }
                    return false;
                };
                if (findAndSelect(item)) break;
            }
        }
    });

    refresh();
}

void OrgTab::refresh()
{
    m_treeModel->clear();
    m_parentCombo->clear(); m_parentCombo->addItem("(无-顶级)", QVariant());
    m_managerCombo->clear(); m_managerCombo->addItem("(无)", QVariant());

    const OrgService service;
    const QVector<OrgService::EmployeeOption> employees = service.activeEmployees();
    for (const auto &employee : employees) {
        m_managerCombo->addItem(employee.name, employee.employeeId);
    }

    const QMap<QString, int> empCountMap = service.activeEmployeeCountsByDepartment();

    QMap<int, QStandardItem *> items;
    QMap<int, int> parentMap;
    const QVector<OrgService::DepartmentNode> departments = service.departments();
    for (const auto &department : departments) {
        parentMap[department.departmentId] = department.parentId;
        int cnt = empCountMap.value(department.name, 0);
        auto *item = new QStandardItem(QString("%1  (%2人)").arg(department.name).arg(cnt));
        item->setData(department.departmentId, Qt::UserRole);
        items[department.departmentId] = item;
        m_parentCombo->addItem(department.name, department.departmentId);
    }

    for (auto it = items.begin(); it != items.end(); ++it) {
        int pid = parentMap.value(it.key(), -1);
        if (pid == -1 || !items.contains(pid)) m_treeModel->appendRow(it.value());
        else items[pid]->appendRow(it.value());
    }

    // Delete orphaned items in circular references to prevent memory leak
    for (auto *item : items.values()) {
        if (!item->model()) {
            delete item;
        }
    }

    m_tree->setModel(m_treeModel);
    m_tree->expandAll();
    m_chartView->refresh();
}

void OrgTab::onTreeSelectionChanged()
{
    auto *item = m_treeModel->itemFromIndex(m_tree->currentIndex());
    if (!item) {
        m_selectedDeptId = -1;
        m_empTable->setModel(nullptr);
        m_btnDel->setEnabled(false);
        return;
    }
    m_selectedDeptId = item->data(Qt::UserRole).toInt();
    m_btnDel->setEnabled(m_selectedDeptId > 0);

    const OrgService::DepartmentDetail detail = OrgService().departmentDetail(m_selectedDeptId);
    if (detail.found) {
        m_nameEdit->setText(detail.name);
        int pid = detail.parentId > 0 ? m_parentCombo->findData(detail.parentId) : 0;
        m_parentCombo->setCurrentIndex(pid > 0 ? pid : 0);
        int mid = detail.managerId > 0 ? m_managerCombo->findData(detail.managerId) : 0;
        m_managerCombo->setCurrentIndex(mid > 0 ? mid : 0);
        // 显示该部门在职员工
        QString escapedName = detail.name;
        escapedName.replace("'", "''");
        m_empModel->setFilter(QString("department='%1' AND status='在职'").arg(escapedName));
        m_empModel->select();
        m_empTable->setModel(m_empModel);
    }
}

void OrgTab::saveDepartment()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) { Toast::show(this, "请输入部门名称", Toast::Warning); return; }
    QVariant pid = m_parentCombo->currentData(), mid = m_managerCombo->currentData();
    const OrgService::Result result = OrgService().saveDepartment(m_selectedDeptId, name, pid, mid);
    if (!result.success) {
        QMessageBox::critical(this, "错误", result.errorMessage);
        return;
    }
    if (m_selectedDeptId > 0) {
        m_log("修改部门", name);
    } else {
        m_log("新增部门", name);
    }
    Toast::show(this, "部门信息已保存", Toast::Success);
    m_selectedDeptId = -1; m_nameEdit->clear(); m_btnDel->setEnabled(false); refresh();
}

void OrgTab::removeDepartment()
{
    if (m_selectedDeptId <= 0) { Toast::show(this, "请先选中要删除的部门", Toast::Warning); return; }
    if (QMessageBox::question(this,"确认",QString("确定删除\"%1\"吗？子部门将移为顶级。").arg(m_nameEdit->text()),
                               QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;
    const OrgService::Result result = OrgService().removeDepartment(m_selectedDeptId);
    if (!result.success) {
        QMessageBox::critical(this, "错误", result.errorMessage);
        return;
    }
    m_log("删除部门", m_nameEdit->text());
    Toast::show(this, "部门已成功删除", Toast::Success);
    m_selectedDeptId = -1; m_nameEdit->clear(); m_btnDel->setEnabled(false); refresh();
}
