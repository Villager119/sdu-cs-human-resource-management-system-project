#include "EmployeeTab.h"
#include "../widgets/PaginationBar.h"
#include "../widgets/ComboDelegate.h"
#include "../widgets/SafeEditDelegate.h"
#include "../services/EmployeeService.h"
#include "../utils/CsvExport.h"
#include "../utils/DbUtils.h"
#include "../core/Constants.h"
#include "../core/GlobalEvents.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QShortcut>
#include <QInputDialog>

EmployeeTab::EmployeeTab(std::function<void(const QString&, const QString&)> logFn,
                         QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    importExistingDepartments();
    setupModel();
    setupDelegates();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setupFilterBar(layout);
    layout->addWidget(m_table, 1);
    setupPagination(layout);
    setupActionBar(layout);
    setupModelSignals();

    m_model->setFilter(QString("status='%1'").arg(HR::EmpStatus::ACTIVE));
    m_model->select();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::importExistingDepartments()
{
    QSqlQuery q;
    q.exec("INSERT IGNORE INTO departments (dept_name) SELECT DISTINCT department FROM employees WHERE department IS NOT NULL AND department!=''");
    q.finish();
}

void EmployeeTab::setupModel()
{
    m_model = new OptimisticSqlTableModel(this, createClonedDatabaseConnection("employee_model"));
    m_model->setTable("employees");

    m_idxEmpId = m_model->fieldIndex("emp_id");
    m_idxName = m_model->fieldIndex("name");
    m_idxGender = m_model->fieldIndex("gender");
    m_idxPhone = m_model->fieldIndex("phone");
    m_idxDept = m_model->fieldIndex("department");
    m_idxRole = m_model->fieldIndex("role");
    m_idxPwd = m_model->fieldIndex("password_hash");
    m_idxSalary = m_model->fieldIndex("base_salary");
    m_idxHire = m_model->fieldIndex("hire_date");
    m_idxContract = m_model->fieldIndex("contract_end_date");
    m_idxStatus = m_model->fieldIndex("status");
    m_idxEdu = m_model->fieldIndex("education");
    m_idxMarital = m_model->fieldIndex("marital_status");
    m_idxPos = m_model->fieldIndex("position");
    m_idxTitle = m_model->fieldIndex("title");

    if (m_idxEmpId >= 0) m_model->setHeaderData(m_idxEmpId, Qt::Horizontal, "员工编号");
    if (m_idxName >= 0) m_model->setHeaderData(m_idxName, Qt::Horizontal, "姓名");
    if (m_idxGender >= 0) m_model->setHeaderData(m_idxGender, Qt::Horizontal, "性别");
    if (m_idxPhone >= 0) m_model->setHeaderData(m_idxPhone, Qt::Horizontal, "联系电话");
    if (m_idxDept >= 0) m_model->setHeaderData(m_idxDept, Qt::Horizontal, "所属部门");
    if (m_idxRole >= 0) m_model->setHeaderData(m_idxRole, Qt::Horizontal, "系统角色");
    if (m_idxSalary >= 0) m_model->setHeaderData(m_idxSalary, Qt::Horizontal, "基础薪资");
    if (m_idxHire >= 0) m_model->setHeaderData(m_idxHire, Qt::Horizontal, "入职日期");
    if (m_idxContract >= 0) m_model->setHeaderData(m_idxContract, Qt::Horizontal, "合同到期");
    if (m_idxStatus >= 0) m_model->setHeaderData(m_idxStatus, Qt::Horizontal, "在职状态");
    if (m_idxEdu >= 0) m_model->setHeaderData(m_idxEdu, Qt::Horizontal, "学历");
    if (m_idxMarital >= 0) m_model->setHeaderData(m_idxMarital, Qt::Horizontal, "婚姻状况");
    if (m_idxPos >= 0) m_model->setHeaderData(m_idxPos, Qt::Horizontal, "岗位");
    if (m_idxTitle >= 0) m_model->setHeaderData(m_idxTitle, Qt::Horizontal, "职称");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setItemDelegate(new SafeEditDelegate(this));
    if (m_idxPwd >= 0) m_table->hideColumn(m_idxPwd);
    int idxVersion = m_model->fieldIndex("version");
    if (idxVersion >= 0) m_table->hideColumn(idxVersion);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void EmployeeTab::setupDelegates()
{
    if (m_idxGender >= 0) m_table->setItemDelegateForColumn(m_idxGender, new ComboDelegate({"男", "女"}, this));
    if (m_idxRole >= 0) {
        QStringList roles = loadRoles();
        if (roles.isEmpty()) roles << "admin" << "user";
        m_table->setItemDelegateForColumn(m_idxRole, new ComboDelegate(roles, this));
    }
    if (m_idxStatus >= 0) {
        m_table->setItemDelegateForColumn(m_idxStatus, new ComboDelegate({
            HR::EmpStatus::ACTIVE,
            HR::EmpStatus::INACTIVE,
            HR::EmpStatus::TRANSFERRED_OUT,
            HR::EmpStatus::RESIGNED,
            HR::EmpStatus::DISMISSED,
            HR::EmpStatus::RETIRED
        }, this));
    }
    if (m_idxEdu >= 0) m_table->setItemDelegateForColumn(m_idxEdu, new ComboDelegate({"大专", "本科", "硕士", "博士"}, this));
    if (m_idxMarital >= 0) m_table->setItemDelegateForColumn(m_idxMarital, new ComboDelegate({"未婚", "已婚", "离异"}, this));
    if (m_idxDept >= 0) m_table->setItemDelegateForColumn(m_idxDept, new ComboDelegate(loadDepartments(), this));
}

void EmployeeTab::setupFilterBar(QVBoxLayout *layout)
{
    auto *filter = new QHBoxLayout;
    m_deptCombo = new QComboBox;
    m_deptCombo->addItem("全部部门");
    filter->addWidget(new QLabel("部门:"));
    filter->addWidget(m_deptCombo);

    m_statusCombo = new QComboBox;
    m_statusCombo->addItems({
        "全部状态",
        HR::EmpStatus::ACTIVE,
        HR::EmpStatus::INACTIVE,
        HR::EmpStatus::TRANSFERRED_OUT,
        HR::EmpStatus::RESIGNED,
        HR::EmpStatus::DISMISSED,
        HR::EmpStatus::RETIRED
    });
    m_statusCombo->setCurrentIndex(1); // 默认在职
    filter->addWidget(new QLabel("状态:"));
    filter->addWidget(m_statusCombo);

    m_maritalCombo = new QComboBox;
    m_maritalCombo->addItems({"全部婚姻", "未婚", "已婚", "离异"});
    filter->addWidget(new QLabel("婚姻:"));
    filter->addWidget(m_maritalCombo);

    m_eduCombo = new QComboBox;
    m_eduCombo->addItems({"全部学历", "大专", "本科", "硕士", "博士"});
    filter->addWidget(new QLabel("学历:"));
    filter->addWidget(m_eduCombo);

    m_posSearch = new QLineEdit;
    m_posSearch->setPlaceholderText("岗位/职称...");
    m_posSearch->setFixedWidth(100);
    filter->addWidget(new QLabel("岗位/职称:"));
    filter->addWidget(m_posSearch);

    m_nameSearch = new QLineEdit;
    m_nameSearch->setPlaceholderText("姓名...");
    m_nameSearch->setFixedWidth(80);
    filter->addWidget(new QLabel("姓名:"));
    filter->addWidget(m_nameSearch);

    auto *btnSearch = new QPushButton("查询");
    auto *btnReset = new QPushButton("重置");
    filter->addWidget(btnSearch);
    filter->addWidget(btnReset);
    layout->addLayout(filter);

    for (const auto &dept : loadDepartments()) {
        m_deptCombo->addItem(dept);
    }

    connect(btnSearch, &QPushButton::clicked, this, &EmployeeTab::search);
    connect(btnReset, &QPushButton::clicked, this, &EmployeeTab::resetFilter);
}

void EmployeeTab::setupPagination(QVBoxLayout *layout)
{
    m_pagination = new PaginationBar(20, this);
    layout->addWidget(m_pagination);
    connect(m_pagination, &PaginationBar::pageChanged, this, [this](int page) {
        Q_UNUSED(page);
        m_table->scrollToTop();
    });
}

void EmployeeTab::setupActionBar(QVBoxLayout *layout)
{
    auto *btnRow = new QGridLayout;
    auto *btnAdd = new QPushButton("添加员工");
    m_btnDel = new QPushButton("删除选中行");
    m_btnDel->setEnabled(false);
    m_btnRevert = new QPushButton("撤销修改");
    m_btnRevert->setEnabled(false);
    m_btnSave = new QPushButton("保存修改");
    m_btnSave->setEnabled(false);
    m_btnToggleStatus = new QPushButton("标记离职");
    m_btnToggleStatus->setEnabled(false);
    auto *btnCSV = new QPushButton("导出CSV");
    btnRow->addWidget(btnAdd, 0, 0);
    btnRow->addWidget(m_btnDel, 0, 1);
    btnRow->addWidget(m_btnRevert, 0, 2);
    btnRow->addWidget(m_btnSave, 0, 3);
    btnRow->addWidget(m_btnToggleStatus, 0, 4);
    btnRow->addWidget(btnCSV, 0, 5);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_btnBatchDept = new QPushButton("批量调部门");
    m_btnBatchDept->setEnabled(false);
    btnRow->addWidget(m_btnBatchDept, 1, 0);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
        bool hasSelection = m_table->selectionModel()->hasSelection();
        m_btnDel->setEnabled(hasSelection);
        m_btnBatchDept->setEnabled(hasSelection);

        auto selectedRows = m_table->selectionModel()->selectedRows();
        if (selectedRows.size() == 1) {
            m_btnToggleStatus->setEnabled(true);
            int row = selectedRows.first().row();
            int col = (m_idxStatus >= 0) ? m_idxStatus : 10;
            m_btnToggleStatus->setText(m_model->data(m_model->index(row, col)).toString() == HR::EmpStatus::ACTIVE ? "标记离职" : "标记在职");
        } else {
            m_btnToggleStatus->setEnabled(false);
            m_btnToggleStatus->setText("标记离职");
        }
    });

    connect(btnAdd, &QPushButton::clicked, this, &EmployeeTab::add);
    connect(m_btnDel, &QPushButton::clicked, this, &EmployeeTab::remove);
    connect(m_btnSave, &QPushButton::clicked, this, &EmployeeTab::save);
    connect(m_btnRevert, &QPushButton::clicked, this, &EmployeeTab::revert);
    connect(m_btnToggleStatus, &QPushButton::clicked, this, &EmployeeTab::toggleStatus);
    connect(btnCSV, &QPushButton::clicked, this, &EmployeeTab::exportCSV);
    connect(m_btnBatchDept, &QPushButton::clicked, this, &EmployeeTab::batchChangeDept);

    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(save()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(remove()));

    layout->addLayout(btnRow);
}

void EmployeeTab::setupModelSignals()
{
    connect(m_model, &QAbstractItemModel::dataChanged, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::modelReset, this, &EmployeeTab::updateDirtyState);
}

QStringList EmployeeTab::loadDepartments() const
{
    QStringList depts;
    QSqlQuery dq;
    dq.exec("SELECT dept_name FROM departments ORDER BY dept_id");
    while (dq.next()) depts.append(dq.value(0).toString());
    dq.finish();
    return depts;
}

QStringList EmployeeTab::loadRoles() const
{
    QStringList roles;
    QSqlQuery rq;
    rq.exec("SELECT role_name FROM roles ORDER BY role_id");
    while (rq.next()) roles.append(rq.value(0).toString());
    rq.finish();
    return roles;
}

void EmployeeTab::add()
{
    int rc = m_model->rowCount();
    m_model->insertRow(rc);
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_idxHire >= 0) m_model->setData(m_model->index(rc, m_idxHire), today);              // 入职日期默认今天
    if (m_idxContract >= 0) m_model->setData(m_model->index(rc, m_idxContract), QDate::currentDate().addYears(3).toString("yyyy-MM-dd")); // 合同到期默认3年后
    if (m_idxStatus >= 0) m_model->setData(m_model->index(rc, m_idxStatus), HR::EmpStatus::ACTIVE);
    if (m_idxRole >= 0) m_model->setData(m_model->index(rc, m_idxRole), "user");
    if (m_idxName >= 0) m_table->setCurrentIndex(m_model->index(rc, m_idxName));
    m_log("新增员工记录", "");
    updateDirtyState();
}

void EmployeeTab::remove()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请先在表格中选中要删除的员工！"); return; }
    QString name = (m_idxName >= 0) ? m_model->data(m_model->index(row, m_idxName)).toString() : "";
    m_model->removeRow(row);
    m_log("删除员工记录", name);
    updateDirtyState();
}

void EmployeeTab::save()
{
    // 新员工无密码时自动注入默认密码 123456 的 SHA-256 哈希
    const QString defaultPwdHash = EmployeeService().defaultPasswordHash();
    for (int r = 0; r < m_model->rowCount(); r++) {
        if (m_idxPwd >= 0 && m_model->data(m_model->index(r, m_idxPwd)).toString().isEmpty())
            m_model->setData(m_model->index(r, m_idxPwd), defaultPwdHash);
    }

    if (!validateRows()) {
        return;
    }

    if (m_model->submitAll()) {
        m_log("保存员工信息修改", "");
        QMessageBox::information(this, "成功", "所有数据修改已成功");
        GlobalEvents::instance()->dataChanged();
    } else {
        QSqlError err = m_model->lastError();
        if (err.driverText() == "OptimisticLockError") {
            QMessageBox::critical(this, "并发冲突", "保存失败！数据已被其他用户修改，请先查询/重置以刷新到最新数据。");
        } else {
            QMessageBox::critical(this, "失败", "保存失败: " + err.text());
        }
    }
    updateDirtyState();
}

bool EmployeeTab::validateRows()
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (m_model->headerData(row, Qt::Vertical).toString() == "!") {
            continue;
        }
        if (!validateEmployeeRow(row)) {
            return false;
        }
    }
    return true;
}

bool EmployeeTab::validateEmployeeRow(int row)
{
    EmployeeService::EmployeeRecord record;
    record.name = (m_idxName >= 0) ? m_model->data(m_model->index(row, m_idxName)).toString().trimmed() : "";
    record.gender = (m_idxGender >= 0) ? m_model->data(m_model->index(row, m_idxGender)).toString().trimmed() : "";
    record.phone = (m_idxPhone >= 0) ? m_model->data(m_model->index(row, m_idxPhone)).toString().trimmed() : "";
    record.department = (m_idxDept >= 0) ? m_model->data(m_model->index(row, m_idxDept)).toString().trimmed() : "";
    record.position = (m_idxPos >= 0) ? m_model->data(m_model->index(row, m_idxPos)).toString().trimmed() : "";
    record.role = (m_idxRole >= 0) ? m_model->data(m_model->index(row, m_idxRole)).toString().trimmed() : "";
    record.status = (m_idxStatus >= 0) ? m_model->data(m_model->index(row, m_idxStatus)).toString().trimmed() : "";
    record.education = (m_idxEdu >= 0) ? m_model->data(m_model->index(row, m_idxEdu)).toString().trimmed() : "";
    record.maritalStatus = (m_idxMarital >= 0) ? m_model->data(m_model->index(row, m_idxMarital)).toString().trimmed() : "";
    record.baseSalary = (m_idxSalary >= 0) ? m_model->data(m_model->index(row, m_idxSalary)) : QVariant();
    record.hireDate = (m_idxHire >= 0) ? m_model->data(m_model->index(row, m_idxHire)) : QVariant();
    record.contractEndDate = (m_idxContract >= 0) ? m_model->data(m_model->index(row, m_idxContract)) : QVariant();
    record.title = (m_idxTitle >= 0) ? m_model->data(m_model->index(row, m_idxTitle)).toString().trimmed() : "";

    QString errorMessage;
    if (!EmployeeService().validateEmployeeRecord(record, row + 1, &errorMessage)) {
        QMessageBox::warning(this, "校验失败", errorMessage);
        return false;
    }

    return true;
}

void EmployeeTab::batchChangeDept()
{
    auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) { QMessageBox::warning(this, "提示", "请先选中要调整的员工(可Ctrl多选)"); return; }
    QString dept = QInputDialog::getText(this, "批量调部门", "请输入目标部门名称:");
    if (dept.isEmpty()) return;
    if (!EmployeeService().departmentExists(dept)) {
        QMessageBox::warning(this, "提示", QString("部门 '%1' 不存在，请先在组织管理中创建该部门").arg(dept));
        return;
    }
    for (auto &idx : sel) {
        if (m_idxDept >= 0)
            m_model->setData(m_model->index(idx.row(), m_idxDept), dept);
    }
    {
        QSqlQuery q;
        q.prepare("INSERT IGNORE INTO departments(dept_name) VALUES(?)");
        q.addBindValue(dept);
        q.exec();
        q.finish();
    }
    QMessageBox::information(this, "完成", QString("已为 %1 名员工设置部门: %2\n记得点击保存修改").arg(sel.size()).arg(dept));
    m_log("批量调部门", QString("%1人 → %2").arg(sel.size()).arg(dept));
}

void EmployeeTab::revert()
{
    m_model->revertAll();
    updateDirtyState();
}

void EmployeeTab::toggleStatus()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请先在表格中选中员工！"); return; }
    QString s = (m_idxStatus >= 0) ? m_model->data(m_model->index(row, m_idxStatus)).toString() : "";
    QString ns = EmployeeService().toggledEmploymentStatus(s);
    if (m_idxStatus >= 0) m_model->setData(m_model->index(row, m_idxStatus), ns);
    QString name = (m_idxName >= 0) ? m_model->data(m_model->index(row, m_idxName)).toString() : "";
    m_log("变更员工状态", name + " → " + ns);
    m_btnToggleStatus->setText(ns == HR::EmpStatus::ACTIVE ? "标记离职" : "标记在职");
    updateDirtyState();
}

void EmployeeTab::search()
{
    QStringList cond;
    if (m_deptCombo->currentIndex() > 0)
        cond << QString("department='%1'").arg(m_deptCombo->currentText().replace("'", "''"));
    if (m_statusCombo->currentIndex() > 0)
        cond << QString("status='%1'").arg(m_statusCombo->currentText().replace("'", "''"));
    if (m_maritalCombo->currentIndex() > 0)
        cond << QString("marital_status='%1'").arg(m_maritalCombo->currentText().replace("'", "''"));
    if (m_eduCombo->currentIndex() > 0)
        cond << QString("education='%1'").arg(m_eduCombo->currentText().replace("'", "''"));
    if (!m_posSearch->text().isEmpty()) {
        QString escaped = m_posSearch->text().trimmed();
        escaped.replace("'", "''");
        cond << QString("(position LIKE '%%1%' OR title LIKE '%%1%')").arg(escaped);
    }
    if (!m_nameSearch->text().isEmpty()) {
        QString escaped = m_nameSearch->text().trimmed();
        escaped.replace("'", "''");
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
    m_statusCombo->setCurrentIndex(1); // 默认在职
    m_maritalCombo->setCurrentIndex(0);
    m_eduCombo->setCurrentIndex(0);
    m_posSearch->clear();
    m_nameSearch->clear();
    m_model->setFilter(QString("status='%1'").arg(HR::EmpStatus::ACTIVE));
    m_model->select();
    m_pagination->refresh();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出员工表", "员工信息.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QList<int> skipCols;
    if (m_idxPwd >= 0) skipCols.append(m_idxPwd);
    exportModelToCSVAsync(m_model, path, this, skipCols); // 异步导出
}

void EmployeeTab::refresh()
{
    m_deptCombo->blockSignals(true);
    QString curDept = m_deptCombo->currentText();
    m_deptCombo->clear();
    m_deptCombo->addItem("全部部门");
    {
        QSqlQuery dq;
        dq.exec("SELECT dept_name FROM departments ORDER BY dept_id");
        while (dq.next()) m_deptCombo->addItem(dq.value(0).toString());
        dq.finish();
    }
    int idx = m_deptCombo->findText(curDept);
    m_deptCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_deptCombo->blockSignals(false);

    if (m_idxDept >= 0) {
        QStringList depts;
        {
            QSqlQuery dq2;
            dq2.exec("SELECT dept_name FROM departments ORDER BY dept_id");
            while (dq2.next()) depts.append(dq2.value(0).toString());
            dq2.finish();
        }
        auto *oldDel = m_table->itemDelegateForColumn(m_idxDept);
        m_table->setItemDelegateForColumn(m_idxDept, new ComboDelegate(depts, this));
        if (oldDel) oldDel->deleteLater();
    }

    if (m_idxRole >= 0) {
        QStringList roles;
        {
            QSqlQuery rq3;
            rq3.exec("SELECT role_name FROM roles ORDER BY role_id");
            while (rq3.next()) roles.append(rq3.value(0).toString());
            rq3.finish();
        }
        if (roles.isEmpty()) roles << "admin" << "user";
        auto *oldDel = m_table->itemDelegateForColumn(m_idxRole);
        m_table->setItemDelegateForColumn(m_idxRole, new ComboDelegate(roles, this));
        if (oldDel) oldDel->deleteLater();
    }

    m_model->select();
    m_pagination->refresh();
    m_pagination->setTotalRecords(m_model->rowCount());
}

void EmployeeTab::updateDirtyState()
{
    bool dirty = m_model->isDirty();
    m_btnSave->setEnabled(dirty);
    m_btnRevert->setEnabled(dirty);
}
