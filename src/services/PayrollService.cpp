#include "PayrollService.h"

#include "../core/Constants.h"
#include "../utils/DbQuery.h"

#include <QDate>
#include <QHash>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

PayrollService::PayrollService(const QSqlDatabase &db)
    : m_db(db)
{
}

bool PayrollService::payrollExists(const QString &month, QString *errorText)
{
    DbQuery::clearError(errorText);

    QSqlQuery check(m_db);
    if (!DbQuery::execPrepared(check, "SELECT COUNT(*) FROM payroll WHERE month=?", {month}, errorText)) {
        return false;
    }

    const bool exists = check.next() && check.value(0).toInt() > 0;
    check.finish();
    return exists;
}

PayrollService::Result PayrollService::calculateMonth(const QString &month, bool overwriteExisting)
{
    QString loadError;
    QDate monthStart;
    if (!PayrollCalc::parseMonth(month, &monthStart, &loadError)) {
        return fail(month, loadError);
    }

    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    double workDays = PayrollCalc::DefaultWorkDaysPerMonth;
    if (!loadWorkDaysPerMonth(&workDays, &loadError)) {
        return fail(month, "读取月计薪天数失败: " + loadError);
    }

    QList<PayrollCalc::TaxItem> taxItems;
    if (!loadTaxItems(&taxItems, &loadError)) {
        return fail(month, "读取社保/公积金配置失败: " + loadError);
    }

    double taxThreshold = PayrollCalc::DefaultTaxThreshold;
    if (!loadTaxThreshold(&taxThreshold, &loadError)) {
        return fail(month, "读取个税起征点失败: " + loadError);
    }

    QList<ActiveEmployee> activeEmployees;
    if (!loadActiveEmployees(monthStart, &activeEmployees, &loadError)) {
        return fail(month, "读取在职员工失败: " + loadError);
    }

    QHash<int, int> leaveDaysByEmployee;
    if (!loadLeaveDaysByEmployee(monthStart, &leaveDaysByEmployee, &loadError)) {
        return fail(month, "读取请假扣款数据失败: " + loadError);
    }

    QHash<int, double> performanceScoresByEmployee;
    if (!loadPerformanceScoresByEmployee(month, &performanceScoresByEmployee, &loadError)) {
        return fail(month, "读取绩效奖金数据失败: " + loadError);
    }

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
        PayrollCalc::Input input;
        input.baseSalary = emp.baseSalary;
        input.leaveDays = leaveDaysByEmployee.value(emp.empId, 0);
        input.performanceScore = performanceScoresByEmployee.value(emp.empId, 0.0);
        input.workDaysPerMonth = workDays;
        input.taxThreshold = taxThreshold;
        input.taxItems = taxItems;
        const PayrollCalc::Breakdown breakdown = PayrollCalc::calculate(input);

        if (!insertPayrollRow(emp.empId, month, emp.baseSalary, breakdown, today, &insertError)) {
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

bool PayrollService::loadWorkDaysPerMonth(double *workDays, QString *errorText)
{
    DbQuery::clearError(errorText);
    QSqlQuery query(m_db);
    double result = PayrollCalc::DefaultWorkDaysPerMonth;

    if (!DbQuery::execPrepared(query, "SELECT value FROM system_settings WHERE key_name=?",
                               {HR::Config::WORK_DAYS}, errorText)) {
        return false;
    }
    if (query.next()) {
        const double configured = query.value(0).toDouble();
        if (configured > 0) {
            result = configured;
        }
    }
    query.finish();
    if (workDays) *workDays = result;
    return true;
}

bool PayrollService::loadTaxItems(QList<PayrollCalc::TaxItem> *taxItems, QString *errorText)
{
    DbQuery::clearError(errorText);
    if (taxItems) taxItems->clear();

    QSqlQuery query(m_db);

    if (!DbQuery::exec(query, "SELECT item_name, rate_personal FROM salary_config WHERE enabled=1", errorText)) {
        return false;
    }
    while (query.next()) {
        if (taxItems) taxItems->append({query.value(0).toString(), query.value(1).toDouble()});
    }
    query.finish();
    return true;
}

bool PayrollService::loadTaxThreshold(double *taxThreshold, QString *errorText)
{
    DbQuery::clearError(errorText);
    QSqlQuery query(m_db);
    double threshold = PayrollCalc::DefaultTaxThreshold;

    if (!DbQuery::execPrepared(query, "SELECT value FROM system_settings WHERE key_name=?",
                               {HR::Config::TAX_THRESHOLD}, errorText)) {
        return false;
    }
    if (query.next()) {
        threshold = query.value(0).toDouble();
    }
    query.finish();
    if (taxThreshold) *taxThreshold = threshold;
    return true;
}

bool PayrollService::loadActiveEmployees(const QDate &monthStart, QList<ActiveEmployee> *employees,
                                         QString *errorText)
{
    DbQuery::clearError(errorText);
    if (employees) employees->clear();

    if (!monthStart.isValid()) {
        DbQuery::setError(errorText, PayrollCalc::invalidMonthMessage());
        return false;
    }
    const QString monthEnd = monthStart.addMonths(1).addDays(-1).toString("yyyy-MM-dd");

    QSqlQuery query(m_db);

    if (!DbQuery::execPrepared(query,
                               "SELECT emp_id, base_salary FROM employees "
                               "WHERE status=? AND (hire_date IS NULL OR hire_date<=?)",
                               {HR::EmpStatus::ACTIVE, monthEnd}, errorText)) {
        return false;
    }
    while (query.next()) {
        if (employees) employees->append({query.value(0).toInt(), query.value(1).toDouble()});
    }
    query.finish();
    return true;
}

bool PayrollService::loadLeaveDaysByEmployee(const QDate &monthStart, QHash<int, int> *leaveDaysByEmployee,
                                             QString *errorText)
{
    DbQuery::clearError(errorText);
    if (leaveDaysByEmployee) leaveDaysByEmployee->clear();

    if (!monthStart.isValid()) {
        DbQuery::setError(errorText, PayrollCalc::invalidMonthMessage());
        return false;
    }
    const QDate monthEnd = monthStart.addMonths(1).addDays(-1);
    const QString monthStartStr = monthStart.toString("yyyy-MM-dd");
    const QString monthEndStr = monthEnd.toString("yyyy-MM-dd");

    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT emp_id, start_date, end_date "
                               "FROM leave_requests "
                               "WHERE status=? AND start_date <= ? AND end_date >= ?",
                               {HR::LeaveStatus::APPROVED, monthEndStr, monthStartStr},
                               errorText)) {
        return false;
    }

    QHash<int, QSet<qint64>> workdaysByEmployee;
    while (query.next()) {
        const int empId = query.value(0).toInt();
        QDate start = query.value(1).toDate();
        QDate end = query.value(2).toDate();
        if (!start.isValid() || !end.isValid()) {
            continue;
        }
        if (start < monthStart) start = monthStart;
        if (end > monthEnd) end = monthEnd;
        for (QDate date = start; date <= end; date = date.addDays(1)) {
            if (PayrollCalc::isWorkday(date)) {
                workdaysByEmployee[empId].insert(date.toJulianDay());
            }
        }
    }
    query.finish();

    if (leaveDaysByEmployee) {
        for (auto it = workdaysByEmployee.cbegin(); it != workdaysByEmployee.cend(); ++it) {
            leaveDaysByEmployee->insert(it.key(), it.value().size());
        }
    }
    return true;
}

bool PayrollService::loadPerformanceScoresByEmployee(const QString &month,
                                                     QHash<int, double> *scoresByEmployee,
                                                     QString *errorText)
{
    DbQuery::clearError(errorText);
    if (scoresByEmployee) scoresByEmployee->clear();

    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT emp_id, score FROM performance_scores "
                               "WHERE eval_month=? AND status=?",
                               {month, HR::PerformanceStatus::PUBLISHED}, errorText)) {
        return false;
    }

    while (query.next()) {
        if (scoresByEmployee) {
            scoresByEmployee->insert(query.value(0).toInt(), query.value(1).toDouble());
        }
    }
    query.finish();
    return true;
}

bool PayrollService::deletePayrollRows(const QString &month, QString *errorText)
{
    QSqlQuery query(m_db);
    return DbQuery::execPreparedAndFinish(query, "DELETE FROM payroll WHERE month=?", {month}, errorText);
}

bool PayrollService::insertPayrollRow(int empId, const QString &month, double baseSalary,
                                      const PayrollCalc::Breakdown &breakdown,
                                      const QString &issueDate, QString *errorText)
{
    QSqlQuery query(m_db);
    return DbQuery::execPreparedAndFinish(
        query,
        "INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,"
        "performance_bonus,pension,medical,unemployment,housing_fund,"
        "income_tax,net_salary,issue_date) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?)",
        {empId,
         month,
         baseSalary,
         breakdown.leaveDeduction,
         breakdown.performanceBonus,
         breakdown.pension,
         breakdown.medical,
         breakdown.unemployment,
         breakdown.housing,
         breakdown.incomeTax,
         breakdown.netSalary,
         issueDate},
        errorText);
}
