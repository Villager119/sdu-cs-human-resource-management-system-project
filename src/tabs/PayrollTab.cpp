#include "PayrollTab.h"
#include "../utils/CsvExport.h"
#include "../utils/DbUtils.h"
#include "../core/Constants.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QSqlDatabase>
#include <QDate>
#include <QFileDialog>
#include <QTextStream>
#include <QSqlError>

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
        filterPayroll();
    });

    refreshMonthCombo();
    m_model->select();
}

void PayrollTab::calculate()
{
    QString month = QDate::currentDate().toString("yyyy-MM");
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    bool exists = payrollExists(month);
    if (exists) {
        int r = QMessageBox::question(this, "重新核算警告",
            month + " 月份的工资已经核算过了！\n再次核算将覆盖原有数据，确定要继续吗？",
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::No) return;

    }

    const double workDays = loadWorkDaysPerMonth();
    const QList<TaxItem> taxItems = loadTaxItems();
    const double taxThreshold = loadTaxThreshold();
    const QList<ActiveEmployee> activeEmployees = loadActiveEmployees();

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    if (exists) {
        QSqlQuery del;
        del.prepare("DELETE FROM payroll WHERE month=?");
        del.addBindValue(month);
        if (!del.exec()) {
            db.rollback();
            QMessageBox::critical(this, "删除失败", "删除原有工资条失败: " + del.lastError().text());
            del.finish();
            return;
        }
        del.finish();
    }

    int ok = 0;
    for (const auto &emp : activeEmployees) {
        QString insErr;
        const int leaveDays = leaveDaysForEmployee(emp.empId, month);
        const double leaveDeduction = (emp.baseSalary / workDays) * leaveDays;
        const double performanceBonus = performanceBonusForEmployee(emp.empId, month, emp.baseSalary);

        if (!insertPayrollRow(emp.empId, month, emp.baseSalary, leaveDeduction,
                              performanceBonus, taxItems, taxThreshold, today, &insErr)) {
            db.rollback();
            QMessageBox::critical(this, "核算失败", "写入工资条时出错: " + insErr);
            return;
        }
        ok++;
    }
    db.commit();
    QMessageBox::information(this, "核算完成", QString("一键核算完毕！\n成功生成了 %1 名员工的 %2 月份工资单。").arg(ok).arg(month));
    m_log("核算工资", month + " (" + QString::number(ok) + "人)");
    m_model->select();
    GlobalEvents::instance()->dataChanged();
}

bool PayrollTab::payrollExists(const QString &month)
{
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM payroll WHERE month=?");
    check.addBindValue(month);

    bool exists = false;
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        exists = true;
    }
    check.finish();
    return exists;
}

double PayrollTab::loadWorkDaysPerMonth()
{
    QSqlQuery query;
    double workDays = 21.75;

    if (query.exec("SELECT value FROM system_settings WHERE key_name='work_days_per_month'") && query.next()) {
        double configured = query.value(0).toDouble();
        if (configured > 0) {
            workDays = configured;
        }
    }
    query.finish();
    return workDays;
}

QList<PayrollTab::TaxItem> PayrollTab::loadTaxItems()
{
    QList<TaxItem> taxItems;
    QSqlQuery query;

    query.exec("SELECT item_name, rate_personal FROM salary_config WHERE enabled=1");
    while (query.next()) {
        taxItems.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    query.finish();
    return taxItems;
}

double PayrollTab::loadTaxThreshold()
{
    QSqlQuery query;
    double threshold = 5000;

    if (query.exec("SELECT value FROM system_settings WHERE key_name='tax_threshold'") && query.next()) {
        threshold = query.value(0).toDouble();
    }
    query.finish();
    return threshold;
}

QList<PayrollTab::ActiveEmployee> PayrollTab::loadActiveEmployees()
{
    QList<ActiveEmployee> employees;
    QSqlQuery query;

    query.exec("SELECT emp_id, base_salary FROM employees WHERE status='在职'");
    while (query.next()) {
        employees.append({query.value(0).toInt(), query.value(1).toDouble()});
    }
    query.finish();
    return employees;
}

int PayrollTab::leaveDaysForEmployee(int empId, const QString &month)
{
    QDate monthStart = QDate::fromString(month + "-01", "yyyy-MM-dd");
    QString monthStartStr = monthStart.toString("yyyy-MM-dd");
    QString monthEndStr = monthStart.addMonths(1).addDays(-1).toString("yyyy-MM-dd");

    QSqlQuery query;
    query.prepare("SELECT SUM(DATEDIFF(LEAST(end_date, ?), GREATEST(start_date, ?)) + 1) "
                  "FROM leave_requests "
                  "WHERE emp_id=? AND status='已同意' AND start_date <= ? AND end_date >= ?");
    query.addBindValue(monthEndStr);
    query.addBindValue(monthStartStr);
    query.addBindValue(empId);
    query.addBindValue(monthEndStr);
    query.addBindValue(monthStartStr);

    int days = 0;
    if (query.exec() && query.next()) {
        days = query.value(0).toInt();
    }
    query.finish();
    return days;
}

double PayrollTab::performanceBonusForEmployee(int empId, const QString &month, double baseSalary)
{
    QSqlQuery query;
    query.prepare("SELECT score FROM performance_scores WHERE emp_id=? AND eval_month=?");
    query.addBindValue(empId);
    query.addBindValue(month);

    double bonus = 0.0;
    if (query.exec() && query.next()) {
        double score = query.value(0).toDouble();
        if (score >= 90) {
            bonus = baseSalary * 0.10;
        } else if (score >= 70) {
            bonus = baseSalary * 0.05;
        }
    }
    query.finish();
    return bonus;
}

bool PayrollTab::insertPayrollRow(int empId, const QString &month, double baseSalary,
                                  double leaveDeduction, double performanceBonus,
                                  const QList<TaxItem> &taxItems, double taxThreshold,
                                  const QString &issueDate, QString *errorText)
{
    double pension = 0;
    double medical = 0;
    double unemployment = 0;
    double housing = 0;
    for (const auto &item : taxItems) {
        double value = baseSalary * item.rate;
        if (item.name == "养老保险") pension = value;
        else if (item.name == "医疗保险") medical = value;
        else if (item.name == "失业保险") unemployment = value;
        else if (item.name == "住房公积金") housing = value;
    }

    double taxable = baseSalary - leaveDeduction + performanceBonus - pension - medical - unemployment - housing - taxThreshold;
    double tax = 0;
    if (taxable > 25000) tax = taxable * 0.25 - 2660;
    else if (taxable > 12000) tax = taxable * 0.20 - 1410;
    else if (taxable > 3000) tax = taxable * 0.10 - 210;
    else if (taxable > 0) tax = taxable * 0.03;
    if (tax < 0) tax = 0;

    double net = baseSalary - leaveDeduction + performanceBonus - pension - medical - unemployment - housing - tax;

    QSqlQuery query;
    query.prepare("INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,performance_bonus,pension,medical,unemployment,housing_fund,income_tax,net_salary,issue_date) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)");
    query.addBindValue(empId);
    query.addBindValue(month);
    query.addBindValue(baseSalary);
    query.addBindValue(leaveDeduction);
    query.addBindValue(performanceBonus);
    query.addBindValue(pension);
    query.addBindValue(medical);
    query.addBindValue(unemployment);
    query.addBindValue(housing);
    query.addBindValue(tax);
    query.addBindValue(net);
    query.addBindValue(issueDate);

    bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
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

