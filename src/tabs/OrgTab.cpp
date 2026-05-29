#include "OrgTab.h"
#include "../services/OrgService.h"
#include "../utils/Toast.h"
#include "../utils/DbUtils.h"
#include "../utils/MessageHelper.h"
#include "../widgets/OrgChartView.h"
#include "../core/GlobalEvents.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QTabWidget>
#include <QHeaderView>
#include <QMap>
#include <QStringList>
#include <QSqlError>
#include <QSqlQuery>
#include <QFrame>

OrgTab::OrgTab(std::function<void(const QString&, const QString&)> logFn,
               QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 右侧员工列表
    m_empModel = new QSqlTableModel(this, createClonedDatabaseConnection("org_employee_model"));
    m_empModel->setTable("employees");

    // 解析动态列索引
    int idxEmpId = m_empModel->fieldIndex("emp_id");
    int idxName = m_empModel->fieldIndex("name");
    int idxPhone = m_empModel->fieldIndex("phone");
    int idxDept = m_empModel->fieldIndex("department");
    int idxPosition = m_empModel->fieldIndex("position");
    int idxTitle = m_empModel->fieldIndex("title");
    int idxSalary = m_empModel->fieldIndex("base_salary");
    int idxStatus = m_empModel->fieldIndex("status");

    if (idxEmpId >= 0) m_empModel->setHeaderData(idxEmpId, Qt::Horizontal, "员工工号");
    if (idxName >= 0) m_empModel->setHeaderData(idxName, Qt::Horizontal, "姓名");
    if (idxPhone >= 0) m_empModel->setHeaderData(idxPhone, Qt::Horizontal, "电话");
    if (idxDept >= 0) m_empModel->setHeaderData(idxDept, Qt::Horizontal, "部门");
    if (idxPosition >= 0) m_empModel->setHeaderData(idxPosition, Qt::Horizontal, "岗位");
    if (idxTitle >= 0) m_empModel->setHeaderData(idxTitle, Qt::Horizontal, "职称");
    if (idxSalary >= 0) m_empModel->setHeaderData(idxSalary, Qt::Horizontal, "薪资");
    if (idxStatus >= 0) m_empModel->setHeaderData(idxStatus, Qt::Horizontal, "状态");

    m_empTable = new QTableView;
    m_empTable->setModel(m_empModel);
    m_empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_empTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_jobModel = new QSqlTableModel(this, createClonedDatabaseConnection("org_job_standard_model"));
    m_jobModel->setTable("job_salary_standards");
    m_jobModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("position"), Qt::Horizontal, "岗位");
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("title"), Qt::Horizontal, "职称");
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("min_salary"), Qt::Horizontal, "最低薪资");
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("default_salary"), Qt::Horizontal, "默认薪资");
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("max_salary"), Qt::Horizontal, "最高薪资");
    m_jobModel->setHeaderData(m_jobModel->fieldIndex("enabled"), Qt::Horizontal, "启用");

    m_jobTable = new QTableView;
    m_jobTable->setModel(m_jobModel);
    m_jobTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_jobTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_jobTable->hideColumn(m_jobModel->fieldIndex("standard_id"));
    m_jobTable->hideColumn(m_jobModel->fieldIndex("dept_id"));
    m_jobTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 动态隐藏除了 工号、姓名、电话、部门、岗位、职称、薪资、状态 以外的所有列
    QList<int> visibleCols;
    if (idxEmpId >= 0) visibleCols.append(idxEmpId);
    if (idxName >= 0) visibleCols.append(idxName);
    if (idxPhone >= 0) visibleCols.append(idxPhone);
    if (idxDept >= 0) visibleCols.append(idxDept);
    if (idxPosition >= 0) visibleCols.append(idxPosition);
    if (idxTitle >= 0) visibleCols.append(idxTitle);
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

    m_chartView = new OrgChartView(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);

    auto *treePanel = new QWidget(this);
    treePanel->setObjectName("orgTreePanel");
    treePanel->setStyleSheet(
        "QWidget#orgTreePanel { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }"
        "QLabel#treeTitle { color: #0f172a; font-weight: bold; font-size: 14px; }"
    );
    auto *treeLayout = new QVBoxLayout(treePanel);
    treeLayout->setContentsMargins(12, 12, 12, 12);
    treeLayout->setSpacing(8);
    auto *treeTitle = new QLabel("部门树", treePanel);
    treeTitle->setObjectName("treeTitle");
    treeLayout->addWidget(treeTitle);
    m_tree->setStyleSheet(
        "QTreeView { border: none; background: transparent; outline: none; }"
        "QTreeView::item { padding: 5px 6px; border-radius: 4px; }"
        "QTreeView::item:selected { background: #eff6ff; color: #1d4ed8; }"
    );
    treeLayout->addWidget(m_tree, 1);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto *summaryBar = new QFrame(rightPanel);
    summaryBar->setObjectName("orgSummaryBar");
    summaryBar->setStyleSheet(
        "QFrame#orgSummaryBar { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }"
        "QLabel#summaryTitle { color: #0f172a; font-weight: bold; font-size: 15px; }"
        "QLabel#summaryHint { color: #64748b; font-size: 12px; }"
    );
    auto *summaryLayout = new QHBoxLayout(summaryBar);
    summaryLayout->setContentsMargins(14, 10, 14, 10);
    summaryLayout->setSpacing(10);
    auto *summaryTitle = new QLabel("组织管理", summaryBar);
    summaryTitle->setObjectName("summaryTitle");
    auto *summaryHint = new QLabel("选择部门后，在右侧页签中维护员工、部门信息和岗位薪资标准。", summaryBar);
    summaryHint->setObjectName("summaryHint");
    summaryLayout->addWidget(summaryTitle);
    summaryLayout->addWidget(summaryHint, 1);
    rightLayout->addWidget(summaryBar);

    m_tabWidget = new QTabWidget(rightPanel);
    m_tabWidget->setObjectName("orgWorkTabs");
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #e2e8f0; border-radius: 8px; background: #ffffff; }"
        "QTabBar::tab { padding: 8px 18px; color: #64748b; background: #f8fafc; border: 1px solid #e2e8f0; border-bottom: none; }"
        "QTabBar::tab:selected { color: #2563eb; background: #ffffff; font-weight: bold; }"
    );

    auto *employeesPage = new QWidget(m_tabWidget);
    auto *employeesLayout = new QVBoxLayout(employeesPage);
    employeesLayout->setContentsMargins(12, 12, 12, 12);
    employeesLayout->setSpacing(8);
    auto *empToolbar = new QHBoxLayout;
    auto *empTitle = new QLabel("员工列表", employeesPage);
    empTitle->setStyleSheet("font-weight: bold; color: #0f172a;");
    empToolbar->addWidget(empTitle);
    empToolbar->addStretch();
    employeesLayout->addLayout(empToolbar);
    employeesLayout->addWidget(m_empTable, 1);
    m_tabWidget->addTab(employeesPage, "员工列表");

    auto *deptPage = new QWidget(m_tabWidget);
    auto *deptLayout = new QVBoxLayout(deptPage);
    deptLayout->setContentsMargins(18, 16, 18, 16);
    deptLayout->setSpacing(12);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    m_nameEdit = new QLineEdit; m_nameEdit->setPlaceholderText("部门名称");
    form->addRow("名称:", m_nameEdit);
    m_parentCombo = new QComboBox; m_parentCombo->addItem("(无-顶级)", QVariant());
    form->addRow("上级:", m_parentCombo);
    m_managerCombo = new QComboBox; m_managerCombo->addItem("(无)", QVariant());
    form->addRow("负责人:", m_managerCombo);
    auto *btnRow = new QHBoxLayout;
    m_btnSave = new QPushButton("保存部门");
    m_btnSave->setProperty("theme", "dark");
    m_btnAssignEmployee = new QPushButton("调入员工");
    m_btnAssignEmployee->setEnabled(false);
    m_btnDel = new QPushButton("删除");
    m_btnDel->setProperty("theme", "danger");
    m_btnDel->setEnabled(false);
    btnRow->addStretch();
    btnRow->addWidget(m_btnSave);
    btnRow->addWidget(m_btnAssignEmployee);
    btnRow->addWidget(m_btnDel);
    deptLayout->addLayout(form);
    deptLayout->addStretch();
    deptLayout->addLayout(btnRow);
    m_tabWidget->addTab(deptPage, "部门信息");

    auto *jobPage = new QWidget(m_tabWidget);
    auto *jobLayout = new QVBoxLayout(jobPage);
    jobLayout->setContentsMargins(12, 12, 12, 12);
    jobLayout->setSpacing(8);
    auto *jobToolbar = new QHBoxLayout;
    auto *jobTitle = new QLabel("岗位薪资标准", jobPage);
    jobTitle->setStyleSheet("font-weight: bold; color: #0f172a;");
    auto *jobHint = new QLabel("维护当前部门允许的岗位、职称与基础薪资范围。", jobPage);
    jobHint->setStyleSheet("color: #64748b;");
    m_btnAddJob = new QPushButton("新增标准");
    m_btnDelJob = new QPushButton("删除标准");
    m_btnSaveJobs = new QPushButton("保存标准");
    m_btnAddJob->setEnabled(false);
    m_btnDelJob->setEnabled(false);
    m_btnSaveJobs->setEnabled(false);
    jobToolbar->addWidget(jobTitle);
    jobToolbar->addWidget(jobHint);
    jobToolbar->addStretch();
    jobToolbar->addWidget(m_btnAddJob);
    jobToolbar->addWidget(m_btnDelJob);
    jobToolbar->addWidget(m_btnSaveJobs);
    jobLayout->addLayout(jobToolbar);
    jobLayout->addWidget(m_jobTable, 1);
    m_tabWidget->addTab(jobPage, "岗位薪资标准");

    m_tabWidget->addTab(m_chartView, "架构图");
    rightLayout->addWidget(m_tabWidget, 1);

    m_splitter->addWidget(treePanel);
    m_splitter->addWidget(rightPanel);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 4);
    m_splitter->setSizes({300, 1200});
    layout->addWidget(m_splitter, 1);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &OrgTab::onTreeSelectionChanged);
    connect(m_btnSave, &QPushButton::clicked, this, &OrgTab::saveDepartment);
    connect(m_btnAssignEmployee, &QPushButton::clicked, this, &OrgTab::assignEmployeeToSelectedDepartment);
    connect(m_btnDel, &QPushButton::clicked, this, &OrgTab::removeDepartment);
    connect(m_btnAddJob, &QPushButton::clicked, this, &OrgTab::addJobStandard);
    connect(m_btnDelJob, &QPushButton::clicked, this, &OrgTab::removeJobStandard);
    connect(m_btnSaveJobs, &QPushButton::clicked, this, &OrgTab::saveJobStandards);
    connect(m_nameEdit, &QLineEdit::textEdited, this, &OrgTab::markDepartmentDirty);
    connect(m_parentCombo, &QComboBox::currentIndexChanged, this, &OrgTab::markDepartmentDirty);
    connect(m_managerCombo, &QComboBox::currentIndexChanged, this, &OrgTab::markDepartmentDirty);
    connect(m_jobTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &OrgTab::updateJobStandardButtons);
    connect(m_jobModel, &QAbstractItemModel::dataChanged, this, &OrgTab::updateJobStandardButtons);
    connect(m_jobModel, &QAbstractItemModel::rowsInserted, this, &OrgTab::updateJobStandardButtons);
    connect(m_jobModel, &QAbstractItemModel::rowsRemoved, this, &OrgTab::updateJobStandardButtons);

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
    m_loadingDepartment = true;
    m_treeModel->clear();
    m_parentCombo->clear(); m_parentCombo->addItem("(无-顶级)", QVariant());
    m_managerCombo->clear(); m_managerCombo->addItem("(请先选择部门)", QVariant());

    const OrgService service;
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
    m_loadingDepartment = false;
    m_departmentDirty = false;

    // Delete orphaned items in circular references to prevent memory leak
    for (auto *item : items.values()) {
        if (!item->model()) {
            delete item;
        }
    }

    m_tree->expandAll();
    m_chartView->refresh();
    if (m_selectedDeptId <= 0) {
        m_jobModel->setFilter("1=0");
        m_jobModel->select();
        updateJobStandardButtons();
    }
}

void OrgTab::onTreeSelectionChanged()
{
    auto *item = m_treeModel->itemFromIndex(m_tree->currentIndex());
    if (!item) {
        m_selectedDeptId = -1;
        m_empTable->setModel(nullptr);
        m_btnDel->setEnabled(false);
        m_btnAssignEmployee->setEnabled(false);
        m_jobModel->setFilter("1=0");
        m_jobModel->select();
        m_departmentDirty = false;
        updateJobStandardButtons();
        return;
    }
    m_selectedDeptId = item->data(Qt::UserRole).toInt();
    m_btnDel->setEnabled(m_selectedDeptId > 0);
    m_btnAssignEmployee->setEnabled(m_selectedDeptId > 0);

    const OrgService::DepartmentDetail detail = OrgService().departmentDetail(m_selectedDeptId);
    if (detail.found) {
        m_loadingDepartment = true;
        m_nameEdit->setText(detail.name);
        int pid = detail.parentId > 0 ? m_parentCombo->findData(detail.parentId) : 0;
        m_parentCombo->setCurrentIndex(pid > 0 ? pid : 0);

        m_managerCombo->clear();
        m_managerCombo->addItem("(无)", QVariant());
        QSqlQuery managerQuery;
        managerQuery.prepare("SELECT emp_id, name FROM employees WHERE department=? AND status='在职' ORDER BY emp_id");
        managerQuery.addBindValue(detail.name);
        if (managerQuery.exec()) {
            while (managerQuery.next()) {
                m_managerCombo->addItem(QString("%1（工号 %2）")
                                            .arg(managerQuery.value(1).toString())
                                            .arg(managerQuery.value(0).toInt()),
                                        managerQuery.value(0).toInt());
            }
        }
        managerQuery.finish();
        int mid = detail.managerId > 0 ? m_managerCombo->findData(detail.managerId) : 0;
        m_managerCombo->setCurrentIndex(mid > 0 ? mid : 0);
        m_loadingDepartment = false;
        m_departmentDirty = false;
        // 显示该部门在职员工
        QString escapedName = detail.name;
        escapedName.replace("'", "''");
        m_empModel->setFilter(QString("department='%1' AND status='在职'").arg(escapedName));
        m_empModel->select();
        m_empTable->setModel(m_empModel);

        m_jobModel->setFilter(QString("dept_id=%1").arg(m_selectedDeptId));
        m_jobModel->select();
        updateJobStandardButtons();
    }
}

void OrgTab::addJobStandard()
{
    if (m_selectedDeptId <= 0) {
        Toast::show(this, "请先选择部门", Toast::Warning);
        return;
    }

    const int row = m_jobModel->rowCount();
    if (!m_jobModel->insertRow(row)) {
        MessageHelper::operationFailed(this, "新增岗位薪资标准", "无法插入新行，请刷新后重试。");
        return;
    }
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("dept_id")), m_selectedDeptId);
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("position")), "新岗位");
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("title")), "初级");
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("min_salary")), "5000.00");
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("default_salary")), "6500.00");
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("max_salary")), "8000.00");
    m_jobModel->setData(m_jobModel->index(row, m_jobModel->fieldIndex("enabled")), 1);
    m_jobTable->setCurrentIndex(m_jobModel->index(row, m_jobModel->fieldIndex("position")));
    updateJobStandardButtons();
}

void OrgTab::removeJobStandard()
{
    const int row = m_jobTable->currentIndex().row();
    if (row < 0) {
        Toast::show(this, "请先选中要删除的岗位薪资标准", Toast::Warning);
        return;
    }
    if (!MessageHelper::confirm(this, "删除确认", "确定删除选中的岗位薪资标准吗？已有员工若仍使用该组合，后续保存会被校验拦截。")) {
        return;
    }
    m_jobModel->removeRow(row);
    updateJobStandardButtons();
}

void OrgTab::saveJobStandards()
{
    for (int row = 0; row < m_jobModel->rowCount(); ++row) {
        const QString position = m_jobModel->data(m_jobModel->index(row, m_jobModel->fieldIndex("position"))).toString().trimmed();
        const QString title = m_jobModel->data(m_jobModel->index(row, m_jobModel->fieldIndex("title"))).toString().trimmed();
        const double minSalary = m_jobModel->data(m_jobModel->index(row, m_jobModel->fieldIndex("min_salary"))).toDouble();
        const double defaultSalary = m_jobModel->data(m_jobModel->index(row, m_jobModel->fieldIndex("default_salary"))).toDouble();
        const double maxSalary = m_jobModel->data(m_jobModel->index(row, m_jobModel->fieldIndex("max_salary"))).toDouble();
        if (position.isEmpty() || title.isEmpty()) {
            MessageHelper::formWarning(this, "岗位/职称", QString("第 %1 行岗位和职称不能为空").arg(row + 1));
            return;
        }
        if (minSalary <= 0 || defaultSalary <= 0 || maxSalary <= 0 || minSalary > defaultSalary || defaultSalary > maxSalary) {
            MessageHelper::formWarning(this, "薪资范围", QString("第 %1 行需满足：最低薪资 <= 默认薪资 <= 最高薪资，且均大于 0").arg(row + 1));
            return;
        }
    }

    if (!m_jobModel->submitAll()) {
        MessageHelper::operationFailed(this, "保存岗位薪资标准", m_jobModel->lastError().text());
        return;
    }

    m_log("维护岗位薪资标准", QString("部门ID: %1").arg(m_selectedDeptId));
    Toast::show(this, "岗位薪资标准已保存", Toast::Success);
    emit GlobalEvents::instance()->dataChanged();
    m_jobModel->select();
    updateJobStandardButtons();
}

void OrgTab::updateJobStandardButtons()
{
    const bool hasDept = m_selectedDeptId > 0;
    const bool hasSelection = m_jobTable->selectionModel() && m_jobTable->selectionModel()->hasSelection();
    m_btnAddJob->setEnabled(hasDept);
    m_btnDelJob->setEnabled(hasDept && hasSelection);
    m_btnSaveJobs->setEnabled(hasDept && m_jobModel->isDirty());
}

void OrgTab::markDepartmentDirty()
{
    if (!m_loadingDepartment) {
        m_departmentDirty = true;
    }
}

bool OrgTab::hasUnsavedChanges() const
{
    return m_departmentDirty || (m_jobModel && m_jobModel->isDirty());
}

bool OrgTab::saveChanges()
{
    if (m_departmentDirty) {
        saveDepartment();
        if (m_departmentDirty) {
            return false;
        }
    }
    if (m_jobModel && m_jobModel->isDirty()) {
        saveJobStandards();
        if (m_jobModel->isDirty()) {
            return false;
        }
    }
    return true;
}

void OrgTab::discardChanges()
{
    if (m_jobModel) {
        m_jobModel->revertAll();
        m_jobModel->select();
    }
    m_departmentDirty = false;
    onTreeSelectionChanged();
}

void OrgTab::assignEmployeeToSelectedDepartment()
{
    if (m_selectedDeptId <= 0) {
        Toast::show(this, "请先选择目标部门", Toast::Warning);
        return;
    }

    OrgService service;
    const OrgService::DepartmentDetail detail = service.departmentDetail(m_selectedDeptId);
    if (!detail.found) {
        MessageHelper::operationFailed(this, "调入员工", "当前部门不存在，请刷新组织架构后重试。");
        return;
    }

    const QVector<OrgService::EmployeeOption> employees = service.activeEmployees();
    QStringList items;
    QMap<QString, int> employeeIds;
    for (const auto &employee : employees) {
        const QString label = QString("%1（工号 %2）").arg(employee.name).arg(employee.employeeId);
        items << label;
        employeeIds.insert(label, employee.employeeId);
    }
    if (items.isEmpty()) {
        Toast::show(this, "暂无可调入的在职员工", Toast::Info);
        return;
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(this, "调入员工",
        QString("选择要调入「%1」的员工:").arg(detail.name), items, 0, false, &ok);
    if (!ok || selected.isEmpty()) return;

    const OrgService::Result result = service.assignEmployeeToDepartment(employeeIds.value(selected), detail.name);
    if (!result.success) {
        MessageHelper::operationFailed(this, "调入员工", result.errorMessage);
        return;
    }

    m_log("调入部门", QString("%1 → %2").arg(selected, detail.name));
    Toast::show(this, "员工已调入当前部门", Toast::Success);
    emit GlobalEvents::instance()->dataChanged();
    refresh();
}

void OrgTab::saveDepartment()
{
    m_btnSave->setEnabled(false);
    m_btnDel->setEnabled(false);
    auto restoreButtons = [this]() {
        m_btnSave->setEnabled(true);
        m_btnDel->setEnabled(m_selectedDeptId > 0);
        m_btnAssignEmployee->setEnabled(m_selectedDeptId > 0);
    };

    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        Toast::show(this, "请输入部门名称", Toast::Warning);
        restoreButtons();
        return;
    }
    QVariant pid = m_parentCombo->currentData(), mid = m_managerCombo->currentData();
    const OrgService::Result result = OrgService().saveDepartment(m_selectedDeptId, name, pid, mid);
    if (!result.success) {
        MessageHelper::operationFailed(this, "保存部门", result.errorMessage);
        restoreButtons();
        return;
    }
    if (m_selectedDeptId > 0) {
        m_log("修改部门", name);
    } else {
        m_log("新增部门", name);
    }
    Toast::show(this, "部门信息已保存", Toast::Success);
    m_departmentDirty = false;
    m_selectedDeptId = -1;
    m_nameEdit->clear();
    refresh();
    if (m_tree->selectionModel()) {
        m_tree->selectionModel()->clearSelection();
    }
    restoreButtons();
}

void OrgTab::removeDepartment()
{
    m_btnSave->setEnabled(false);
    m_btnDel->setEnabled(false);
    auto restoreButtons = [this]() {
        m_btnSave->setEnabled(true);
        m_btnDel->setEnabled(m_selectedDeptId > 0);
        m_btnAssignEmployee->setEnabled(m_selectedDeptId > 0);
    };

    if (m_selectedDeptId <= 0) {
        Toast::show(this, "请先选中要删除的部门", Toast::Warning);
        restoreButtons();
        return;
    }
    if (!MessageHelper::confirm(this, "删除确认",
        QString("确定删除\"%1\"吗？\n删除前请确认该部门下没有在职员工；子部门将移为顶级。").arg(m_nameEdit->text()))) {
        restoreButtons();
        return;
    }
    const OrgService::Result result = OrgService().removeDepartment(m_selectedDeptId);
    if (!result.success) {
        MessageHelper::operationFailed(this, "删除部门", result.errorMessage);
        restoreButtons();
        return;
    }
    m_log("删除部门", m_nameEdit->text());
    Toast::show(this, "部门已成功删除", Toast::Success);
    m_selectedDeptId = -1;
    m_nameEdit->clear();
    refresh();
    if (m_tree->selectionModel()) {
        m_tree->selectionModel()->clearSelection();
    }
    restoreButtons();
}
