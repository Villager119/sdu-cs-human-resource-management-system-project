#include "EmployeeTab.h"
#include "../widgets/PaginationBar.h"
#include "../widgets/ComboDelegate.h"
#include "../widgets/SafeEditDelegate.h"
#include "../utils/CsvExport.h"
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
#include <QCryptographicHash>
#include <QTextStream>
#include <QShortcut>
#include <QInputDialog>
#include <QRegularExpression>

EmployeeTab::EmployeeTab(std::function<void(const QString&, const QString&)> logFn,
                         QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 自动将员工表中现有的部门导入部门表
    QSqlQuery q;
    q.exec("INSERT IGNORE INTO departments (dept_name) SELECT DISTINCT department FROM employees WHERE department IS NOT NULL AND department!=''");

    m_model = new QSqlTableModel(this);
    m_model->setTable("employees");

    // 解析动态列索引
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
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 下拉选择委托
    if (m_idxGender >= 0) m_table->setItemDelegateForColumn(m_idxGender, new ComboDelegate({"男", "女"}, this));
    if (m_idxRole >= 0) {
        QStringList roles;
        QSqlQuery rq("SELECT role_name FROM roles ORDER BY role_id");
        while (rq.next()) roles.append(rq.value(0).toString());
        rq.finish();
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
    if (m_idxDept >= 0) {
        QStringList depts;
        QSqlQuery dq("SELECT dept_name FROM departments ORDER BY dept_id");
        while (dq.next()) depts.append(dq.value(0).toString());
        dq.finish();
        m_table->setItemDelegateForColumn(m_idxDept, new ComboDelegate(depts, this));
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 筛选栏
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
    layout->addLayout(btnRow);

    QSqlQuery dq("SELECT dept_name FROM departments ORDER BY dept_id");
    while (dq.next()) m_deptCombo->addItem(dq.value(0).toString());
    dq.finish();

    // 选择变化更新按钮状态
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
    connect(btnSearch, &QPushButton::clicked, this, &EmployeeTab::search);
    connect(btnReset, &QPushButton::clicked, this, &EmployeeTab::resetFilter);
    connect(btnCSV, &QPushButton::clicked, this, &EmployeeTab::exportCSV);

    // 快捷键
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(save()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(remove()));

    // 批量操作
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_btnBatchDept = new QPushButton("批量调部门");
    m_btnBatchDept->setEnabled(false);
    btnRow->addWidget(m_btnBatchDept, 1, 0);
    connect(m_btnBatchDept, &QPushButton::clicked, this, &EmployeeTab::batchChangeDept);

    // 监听模型修改状态以启用保存/撤销按钮
    connect(m_model, &QAbstractItemModel::dataChanged, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &EmployeeTab::updateDirtyState);
    connect(m_model, &QAbstractItemModel::modelReset, this, &EmployeeTab::updateDirtyState);

    m_model->setFilter(QString("status='%1'").arg(HR::EmpStatus::ACTIVE));
    m_model->select();
    m_pagination->setTotalRecords(m_model->rowCount());
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
    static const QString defaultPwdHash = QString(QCryptographicHash::hash(HR::DEFAULT_PASSWORD.toUtf8(), QCryptographicHash::Sha256).toHex());
    for (int r = 0; r < m_model->rowCount(); r++) {
        if (m_idxPwd >= 0 && m_model->data(m_model->index(r, m_idxPwd)).toString().isEmpty())
            m_model->setData(m_model->index(r, m_idxPwd), defaultPwdHash);
    }
    // 输入校验
    for (int r = 0; r < m_model->rowCount(); r++) {
        // 跳过标记为删除的行
        if (m_model->headerData(r, Qt::Vertical).toString() == "!")
            continue;

        // 1. 姓名不能为空
        QString name = (m_idxName >= 0) ? m_model->data(m_model->index(r, m_idxName)).toString().trimmed() : "";
        if (name.isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：姓名不能为空").arg(r + 1));
            return;
        }

        // 2. 性别必须是 '男' 或 '女'
        QString gender = (m_idxGender >= 0) ? m_model->data(m_model->index(r, m_idxGender)).toString().trimmed() : "";
        if (gender != "男" && gender != "女") {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：性别必须是 '男' 或 '女'").arg(r + 1));
            return;
        }

        // 3. 手机号校验
        QString phone = (m_idxPhone >= 0) ? m_model->data(m_model->index(r, m_idxPhone)).toString().trimmed() : "";
        if (phone.isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：联系电话不能为空").arg(r + 1));
            return;
        }
        if (!QRegularExpression("^\\d{11}$").match(phone).hasMatch()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：联系电话格式不正确（必须为 11 位数字）").arg(r + 1));
            return;
        }

        // 4. 部门不能为空且必须存在于系统中
        QString dept = (m_idxDept >= 0) ? m_model->data(m_model->index(r, m_idxDept)).toString().trimmed() : "";
        if (dept.isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：所属部门不能为空").arg(r + 1));
            return;
        }
        QSqlQuery dCheck;
        dCheck.prepare("SELECT COUNT(*) FROM departments WHERE dept_name=?");
        dCheck.addBindValue(dept);
        if (!dCheck.exec() || !dCheck.next() || dCheck.value(0).toInt() == 0) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：部门 '%2' 在系统中不存在，请先在部门管理中创建该部门，或选择已有部门").arg(r + 1).arg(dept));
            return;
        }

        // 5. 岗位不能为空
        QString pos = (m_idxPos >= 0) ? m_model->data(m_model->index(r, m_idxPos)).toString().trimmed() : "";
        if (pos.isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：岗位不能为空").arg(r + 1));
            return;
        }

        // 6. 系统角色校验
        QString role = (m_idxRole >= 0) ? m_model->data(m_model->index(r, m_idxRole)).toString().trimmed() : "";
        QSqlQuery rCheck;
        rCheck.prepare("SELECT COUNT(*) FROM roles WHERE role_name=?");
        rCheck.addBindValue(role);
        if (!rCheck.exec() || !rCheck.next() || rCheck.value(0).toInt() == 0) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：系统角色 '%2' 在系统中不存在").arg(r + 1).arg(role));
            return;
        }

        // 7. 在职状态校验
        QString status = (m_idxStatus >= 0) ? m_model->data(m_model->index(r, m_idxStatus)).toString().trimmed() : "";
        QStringList validStatus = {
            HR::EmpStatus::ACTIVE,
            HR::EmpStatus::INACTIVE,
            HR::EmpStatus::TRANSFERRED_OUT,
            HR::EmpStatus::RESIGNED,
            HR::EmpStatus::DISMISSED,
            HR::EmpStatus::RETIRED
        };
        if (!validStatus.contains(status)) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：在职状态无效").arg(r + 1));
            return;
        }

        // 8. 学历校验
        QString edu = (m_idxEdu >= 0) ? m_model->data(m_model->index(r, m_idxEdu)).toString().trimmed() : "";
        QStringList validEdu = {"大专", "本科", "硕士", "博士"};
        if (!validEdu.contains(edu)) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：学历必须是 大专/本科/硕士/博士 之一").arg(r + 1));
            return;
        }

        // 9. 婚姻状况校验
        QString marital = (m_idxMarital >= 0) ? m_model->data(m_model->index(r, m_idxMarital)).toString().trimmed() : "";
        QStringList validMarital = {"未婚", "已婚", "离异"};
        if (!validMarital.contains(marital)) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：婚姻状况必须是 未婚/已婚/离异 之一").arg(r + 1));
            return;
        }

        // 10. 基础薪资校验
        QVariant salVar = (m_idxSalary >= 0) ? m_model->data(m_model->index(r, m_idxSalary)) : QVariant();
        if (salVar.toString().trimmed().isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：基础薪资不能为空").arg(r + 1));
            return;
        }
        bool ok = false;
        double salary = salVar.toDouble(&ok);
        if (!ok || salary <= 0) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：基础薪资必须为大于 0 的数值").arg(r + 1));
            return;
        }

        // 11. 日期校验
        QVariant hireVar = (m_idxHire >= 0) ? m_model->data(m_model->index(r, m_idxHire)) : QVariant();
        QDate hireDate = hireVar.toDate();
        if (!hireDate.isValid()) {
            hireDate = QDate::fromString(hireVar.toString(), "yyyy-MM-dd");
        }
        if (!hireDate.isValid()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：入职日期无效或格式不正确（需 yyyy-MM-dd）").arg(r + 1));
            return;
        }

        QVariant contractVar = (m_idxContract >= 0) ? m_model->data(m_model->index(r, m_idxContract)) : QVariant();
        if (!contractVar.toString().trimmed().isEmpty() && !contractVar.isNull()) {
            QDate contractDate = contractVar.toDate();
            if (!contractDate.isValid()) {
                contractDate = QDate::fromString(contractVar.toString(), "yyyy-MM-dd");
            }
            if (!contractDate.isValid()) {
                QMessageBox::warning(this, "校验失败", QString("第 %1 行：合同到期日期格式不正确（需 yyyy-MM-dd）").arg(r + 1));
                return;
            }
            if (contractDate < hireDate) {
                QMessageBox::warning(this, "校验失败", QString("第 %1 行：合同到期日期不能早于入职日期").arg(r + 1));
                return;
            }
        }

        // 12. 职称不能为空
        QString title = (m_idxTitle >= 0) ? m_model->data(m_model->index(r, m_idxTitle)).toString().trimmed() : "";
        if (title.isEmpty()) {
            QMessageBox::warning(this, "校验失败", QString("第 %1 行：职称不能为空（如果无职称，请填写'无'）").arg(r + 1));
            return;
        }
    }
    if (m_model->submitAll()) { m_log("保存员工信息修改", ""); QMessageBox::information(this, "成功", "所有数据修改已成功"); GlobalEvents::instance()->dataChanged(); }
    else QMessageBox::critical(this, "失败", "保存失败: " + m_model->lastError().text());
    updateDirtyState();
}

void EmployeeTab::batchChangeDept()
{
    auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) { QMessageBox::warning(this, "提示", "请先选中要调整的员工(可Ctrl多选)"); return; }
    QString dept = QInputDialog::getText(this, "批量调部门", "请输入目标部门名称:");
    if (dept.isEmpty()) return;
    for (auto &idx : sel) {
        if (m_idxDept >= 0)
            m_model->setData(m_model->index(idx.row(), m_idxDept), dept);
    }
    QSqlQuery q; q.prepare("INSERT IGNORE INTO departments(dept_name) VALUES(?)"); q.addBindValue(dept); q.exec();
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
    QString ns = (s == HR::EmpStatus::ACTIVE) ? HR::EmpStatus::INACTIVE : HR::EmpStatus::ACTIVE;
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
    exportModelToCSV(m_model, path, skipCols); // 跳过密码列
}

void EmployeeTab::refresh()
{
    m_deptCombo->blockSignals(true);
    QString curDept = m_deptCombo->currentText();
    m_deptCombo->clear();
    m_deptCombo->addItem("全部部门");
    QSqlQuery dq("SELECT dept_name FROM departments ORDER BY dept_id");
    while (dq.next()) m_deptCombo->addItem(dq.value(0).toString());
    dq.finish();
    int idx = m_deptCombo->findText(curDept);
    m_deptCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_deptCombo->blockSignals(false);

    if (m_idxDept >= 0) {
        QStringList depts;
        QSqlQuery dq2("SELECT dept_name FROM departments ORDER BY dept_id");
        while (dq2.next()) depts.append(dq2.value(0).toString());
        dq2.finish();
        m_table->setItemDelegateForColumn(m_idxDept, new ComboDelegate(depts, this));
    }

    if (m_idxRole >= 0) {
        QStringList roles;
        QSqlQuery rq3("SELECT role_name FROM roles ORDER BY role_id");
        while (rq3.next()) roles.append(rq3.value(0).toString());
        rq3.finish();
        if (roles.isEmpty()) roles << "admin" << "user";
        m_table->setItemDelegateForColumn(m_idxRole, new ComboDelegate(roles, this));
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
