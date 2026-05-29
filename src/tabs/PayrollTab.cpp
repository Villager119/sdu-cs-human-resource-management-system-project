#include "PayrollTab.h"
#include "../services/PayrollService.h"
#include "../utils/CsvExport.h"
#include "../utils/DbUtils.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QDate>
#include <QFileDialog>

PayrollTab::PayrollTab(int empId, const QString &role,
                       std::function<void(const QString&, const QString&)> logFn,
                       QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn)
{
    m_model = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("payroll_model"));
    m_model->setTable("payroll");
    m_model->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_model->setHeaderData(0, Qt::Horizontal, "工资条ID");
    m_model->setHeaderData(1, Qt::Horizontal, "姓名");
    m_model->setHeaderData(2, Qt::Horizontal, "薪资月份");
    m_model->setHeaderData(3, Qt::Horizontal, "基础工资");
    m_model->setHeaderData(4, Qt::Horizontal, "请假扣款");
    m_model->setHeaderData(5, Qt::Horizontal, "绩效奖金");
    m_model->setHeaderData(6, Qt::Horizontal, "养老保险");
    m_model->setHeaderData(7, Qt::Horizontal, "医疗保险");
    m_model->setHeaderData(8, Qt::Horizontal, "失业保险");
    m_model->setHeaderData(9, Qt::Horizontal, "住房公积金");
    m_model->setHeaderData(10, Qt::Horizontal, "个人所得税");
    m_model->setHeaderData(11, Qt::Horizontal, "实发最终工资");
    m_model->setHeaderData(12, Qt::Horizontal, "结算发薪日期");

    if (!SessionManager::instance()->hasPermission("calculate_payroll"))
        m_model->setFilter(QString("payroll.emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 筛选栏
    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("选择月份:"));
    m_monthCombo = new QComboBox;
    filterRow->addWidget(m_monthCombo);
    if (SessionManager::instance()->hasPermission("calculate_payroll")) {
        filterRow->addWidget(new QLabel("员工姓名:"));
        m_employeeNameEdit = new QLineEdit;
        m_employeeNameEdit->setPlaceholderText("输入姓名查询工资条");
        m_employeeNameEdit->setMinimumWidth(180);
        filterRow->addWidget(m_employeeNameEdit);
    } else {
        m_employeeNameEdit = nullptr;
    }

    m_btnSearch = new QPushButton("查询");
    m_btnReset = new QPushButton("重置");
    filterRow->addWidget(m_btnSearch);
    filterRow->addWidget(m_btnReset);
    filterRow->addStretch();
    layout->addLayout(filterRow);

    layout->addWidget(m_table, 1);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_btnCalc = new QPushButton("一键核算本月工资");
    if (!SessionManager::instance()->hasPermission("calculate_payroll")) m_btnCalc->setVisible(false);
    row->addWidget(m_btnCalc);
    auto *btnCSV = new QPushButton("导出CSV");
    row->addWidget(btnCSV);
    row->addStretch();
    layout->addLayout(row);

    connect(m_btnCalc, &QPushButton::clicked, this, &PayrollTab::calculate);
    connect(btnCSV, &QPushButton::clicked, this, &PayrollTab::exportCSV);
    connect(m_btnSearch, &QPushButton::clicked, this, &PayrollTab::filterPayroll);
    connect(m_btnReset, &QPushButton::clicked, this, [this]() {
        m_monthCombo->setCurrentIndex(0);
        if (m_employeeNameEdit) {
            m_employeeNameEdit->clear();
        }
        filterPayroll();
    });
    if (m_employeeNameEdit) {
        connect(m_employeeNameEdit, &QLineEdit::returnPressed, this, &PayrollTab::filterPayroll);
    }

    refreshMonthCombo();
    m_model->select();
}

void PayrollTab::calculate()
{
    m_btnCalc->setEnabled(false);
    auto restoreCalcButton = [this]() {
        m_btnCalc->setEnabled(true);
    };

    const QString month = QDate::currentDate().toString("yyyy-MM");
    PayrollService service;

    const bool exists = service.payrollExists(month);
    if (exists) {
        const int r = QMessageBox::question(this, "重新核算确认",
            month + " 月份的工资已存在核算记录。\n重新核算将覆盖原有数据，是否确定继续？",
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::No) {
            restoreCalcButton();
            return;
        }
    }

    const PayrollService::Result result = service.calculateMonth(month, exists);
    if (!result.success) {
        QMessageBox::critical(this, "核算失败", result.errorMessage);
        restoreCalcButton();
        return;
    }

    QMessageBox::information(this, "核算成功",
        QString("一键核算完毕。\n已成功生成了 %1 名员工的 %2 月份工资单。")
            .arg(result.generatedCount)
            .arg(result.month));
    m_log("核算工资", result.month + " (" + QString::number(result.generatedCount) + "人)");
    refresh();
    GlobalEvents::instance()->dataChanged();
    restoreCalcButton();
}

void PayrollTab::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出工资表", "薪酬数据.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    exportModelToCSVAsync(m_model, path, this);
}

void PayrollTab::refresh()
{
    refreshMonthCombo();
    filterPayroll();
}

void PayrollTab::filterPayroll()
{
    QStringList conds;
    if (!SessionManager::instance()->hasPermission("calculate_payroll")) {
        conds << QString("payroll.emp_id = %1").arg(m_empId);
    }
    if (m_monthCombo->currentIndex() > 0) {
        QString month = m_monthCombo->currentText();
        month.replace("'", "''");
        conds << QString("payroll.month = '%1'").arg(month);
    }
    if (m_employeeNameEdit && !m_employeeNameEdit->text().trimmed().isEmpty()) {
        QString name = m_employeeNameEdit->text().trimmed();
        name.replace("\\", "\\\\");
        name.replace("'", "''");
        name.replace("%", "\\%");
        name.replace("_", "\\_");
        conds << QString("payroll.emp_id IN (SELECT emp_id FROM employees WHERE name LIKE '%%1%' ESCAPE '\\\\')")
                     .arg(name);
    }
    m_model->setFilter(conds.isEmpty() ? "" : conds.join(" AND "));
    m_model->select();
}

void PayrollTab::refreshMonthCombo()
{
    m_monthCombo->blockSignals(true);
    QString curMonth = m_monthCombo->currentText();
    m_monthCombo->clear();
    m_monthCombo->addItem("全部月份");
    
    {
        QSqlQuery q;
        q.exec("SELECT DISTINCT month FROM payroll ORDER BY month DESC");
        while (q.next()) {
            m_monthCombo->addItem(q.value(0).toString());
        }
        q.finish();
    }
    
    int idx = m_monthCombo->findText(curMonth);
    m_monthCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_monthCombo->blockSignals(false);
}

