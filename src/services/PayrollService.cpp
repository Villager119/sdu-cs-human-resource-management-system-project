#include "PayrollService.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>

PayrollService::PayrollService(const QSqlDatabase &db)
    : m_db(db)
{
}

static bool payrollWorkday(const QDate &date)
{
    return date.isValid() && date.dayOfWeek() >= 1 && date.dayOfWeek() <= 5;
}

bool PayrollService::payrollExists(const QString &month)
{
    QSqlQuery check(m_db);
    check.prepare("SELECT COUNT(*) FROM payroll WHERE month=?");
    check.addBindValue(month);

    bool exists = false;
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        exists = true;
    }
    check.finish();
    return exists;
}

PayrollService::Result PayrollService::calculateMonth(const QString &month, bool overwriteExisting)
{
    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    const double workDays = loadWorkDaysPerMonth();
    const QList<TaxItem> taxItems = loadTaxItems();
    const double taxThreshold = loadTaxThreshold();
    const QList<ActiveEmployee> activeEmployees = loadActiveEmployees(month);

    if (!m_db.transaction()) {
        return fail(month, "启动薪酬核算事务失败: " + m_db.lastError().text());
    }

    if (overwriteExisting) {
        QString deleteError;
        if (!deletePayrollRows(month, &deleteError)) {
            m_db.rollback();
            return fail(month, "删除原有工资条失败: " + deleteError);
        }
    }

    int generatedCount = 0;
    for (const auto &emp : activeEmployees) {
        QString insertError;
        const int leaveDays = leaveDaysForEmployee(emp.empId, month);
        const double leaveDeduction = std::min((emp.baseSalary / workDays) * leaveDays, emp.baseSalary);
        const double performanceBonus = performanceBonusForEmployee(emp.empId, month, emp.baseSalary);

        if (!insertPayrollRow(emp.empId, month, emp.baseSalary, leaveDeduction,
                              performanceBonus, taxItems, taxThreshold, today, &insertError)) {
            m_db.rollback();
            return fail(month, "写入工资条时出错: " + insertError);
        }
        generatedCount++;
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail(month, "提交薪酬核算事务失败: " + commitErr);
    }

    Result result;
    result.success = true;
    result.generatedCount = generatedCount;
    result.month = month;
    return result;
}

PayrollService::Result PayrollService::fail(const QString &month, const QString &message)
{
    Result result;
    result.month = month;
    result.errorMessage = message;
    return result;
}

double PayrollService::loadWorkDaysPerMonth()
{
    QSqlQuery query(m_db);
    double workDays = 21.75;

    if (query.exec("SELECT value FROM system_settings WHERE key_name='work_days_per_month'") && query.next()) {
        const double configured = query.value(0).toDouble();
        if (configured > 0) {
            workDays = configured;
        }
    }
    query.finish();
    return workDays;
}

QList<PayrollService::TaxItem> PayrollService::loadTaxItems()
{
    QList<TaxItem> taxItems;
    QSqlQuery query(m_db);

    query.exec("SELECT item_name, rate_personal FROM salary_config WHERE enabled=1");
    while (query.next()) {
        taxItems.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    query.finish();
    return taxItems;
}

double PayrollService::loadTaxThreshold()
{
    QSqlQuery query(m_db);
    double threshold = 5000;

    if (query.exec("SELECT value FROM system_settings WHERE key_name='tax_threshold'") && query.next()) {
        threshold = query.value(0).toDouble();
    }
    query.finish();
    return threshold;
}

QList<PayrollService::ActiveEmployee> PayrollService::loadActiveEmployees(const QString &month)
{
    QList<ActiveEmployee> employees;
    const QDate monthStart = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!monthStart.isValid()) {
        return employees;
    }
    const QString monthEnd = monthStart.addMonths(1).addDays(-1).toString("yyyy-MM-dd");

    QSqlQuery query(m_db);

    query.prepare("SELECT emp_id, base_salary FROM employees "
                  "WHERE status='在职' AND (hire_date IS NULL OR hire_date<=?)");
    query.addBindValue(monthEnd);
    query.exec();
    while (query.next()) {
        employees.append({query.value(0).toInt(), query.value(1).toDouble()});
    }
    query.finish();
    return employees;
}

int PayrollService::leaveDaysForEmployee(int empId, const QString &month)
{
    const QDate monthStart = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!monthStart.isValid()) {
        return 0;
    }
    const QDate monthEnd = monthStart.addMonths(1).addDays(-1);
    const QString monthStartStr = monthStart.toString("yyyy-MM-dd");
    const QString monthEndStr = monthEnd.toString("yyyy-MM-dd");

    QSqlQuery query(m_db);
    query.prepare("SELECT start_date, end_date "
                  "FROM leave_requests "
                  "WHERE emp_id=? AND status='已同意' AND start_date <= ? AND end_date >= ?");
    query.addBindValue(empId);
    query.addBindValue(monthEndStr);
    query.addBindValue(monthStartStr);

    int days = 0;
    if (query.exec()) {
        while (query.next()) {
            QDate start = query.value(0).toDate();
            QDate end = query.value(1).toDate();
            if (!start.isValid() || !end.isValid()) {
                continue;
            }
            if (start < monthStart) start = monthStart;
            if (end > monthEnd) end = monthEnd;
            for (QDate date = start; date <= end; date = date.addDays(1)) {
                if (payrollWorkday(date)) {
                    ++days;
                }
            }
        }
    }
    query.finish();
    return days;
}

double PayrollService::performanceBonusForEmployee(int empId, const QString &month, double baseSalary)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT score FROM performance_scores "
                  "WHERE emp_id=? AND eval_month=? AND status='已发布'");
    query.addBindValue(empId);
    query.addBindValue(month);

    double bonus = 0.0;
    if (query.exec() && query.next()) {
        const double score = query.value(0).toDouble();
        if (score >= 90) {
            bonus = baseSalary * 0.10;
        } else if (score >= 70) {
            bonus = baseSalary * 0.05;
        }
    }
    query.finish();
    return bonus;
}

bool PayrollService::deletePayrollRows(const QString &month, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM payroll WHERE month=?");
    query.addBindValue(month);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}

bool PayrollService::insertPayrollRow(int empId, const QString &month, double baseSalary,
                                      double leaveDeduction, double performanceBonus,
                                      const QList<TaxItem> &taxItems, double taxThreshold,
                                      const QString &issueDate, QString *errorText)
{
    double pension = 0;
    double medical = 0;
    double unemployment = 0;
    double housing = 0;
    for (const auto &item : taxItems) {
        const double value = baseSalary * item.rate;
        if (item.name == "养老保险") pension = value;
        else if (item.name == "医疗保险") medical = value;
        else if (item.name == "失业保险") unemployment = value;
        else if (item.name == "住房公积金") housing = value;
    }

    const double taxable = baseSalary - leaveDeduction + performanceBonus
                           - pension - medical - unemployment - housing - taxThreshold;
    double tax = 0;
    if (taxable > 25000) tax = taxable * 0.25 - 2660;
    else if (taxable > 12000) tax = taxable * 0.20 - 1410;
    else if (taxable > 3000) tax = taxable * 0.10 - 210;
    else if (taxable > 0) tax = taxable * 0.03;
    if (tax < 0) tax = 0;

    const double net = std::max(0.0, baseSalary - leaveDeduction + performanceBonus
                                      - pension - medical - unemployment - housing - tax);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,"
                  "performance_bonus,pension,medical,unemployment,housing_fund,"
                  "income_tax,net_salary,issue_date) "
                  "VALUES(?,?,?,?,?,?,?,?,?,?,?,?)");
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

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}
