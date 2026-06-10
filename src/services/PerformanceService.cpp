#include "PerformanceService.h"

#include "../core/Constants.h"
#include "../utils/DbQuery.h"

#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

PerformanceService::PerformanceService(const QSqlDatabase &db)
    : m_db(db)
{
}

QList<PerformanceService::EmployeeOption> PerformanceService::activeEmployees() const
{
    QList<EmployeeOption> employees;
    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query, "SELECT emp_id, name FROM employees WHERE status=?",
                               {HR::EmpStatus::ACTIVE})) {
        return employees;
    }
    while (query.next()) {
        employees.append({query.value(0).toInt(), query.value(1).toString()});
    }
    query.finish();
    return employees;
}

PerformanceService::ScoreDetail PerformanceService::scoreDetail(int scoreId) const
{
    ScoreDetail detail;
    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT emp_id, eval_month, attitude, capability, teamwork, innovation, comment "
                               "FROM performance_scores WHERE score_id = ?",
                               {scoreId})) {
        return detail;
    }

    if (query.next()) {
        detail.found = true;
        detail.employeeId = query.value(0).toInt();
        detail.month = query.value(1).toString();
        detail.attitude = query.value(2).toInt();
        detail.capability = query.value(3).toInt();
        detail.teamwork = query.value(4).toInt();
        detail.innovation = query.value(5).toInt();
        detail.comment = query.value(6).toString();
    }
    query.finish();
    return detail;
}

PerformanceService::Result PerformanceService::saveScore(const ScoreInput &input)
{
    if (input.employeeId <= 0) {
        return fail("请选择员工");
    }
    if (input.month.isEmpty()) {
        return fail("请输入考核月份");
    }
    if (input.attitude < 0 || input.attitude > 25 ||
        input.capability < 0 || input.capability > 25 ||
        input.teamwork < 0 || input.teamwork > 25 ||
        input.innovation < 0 || input.innovation > 25) {
        return fail("单项评分必须在 0 到 25 分之间");
    }

    QRegularExpression re("^\\d{4}-(0[1-9]|1[0-2])$");
    if (!re.match(input.month).hasMatch()) {
        return fail("考核月份格式不正确，请输入 YYYY-MM 格式");
    }

    const int total = input.attitude + input.capability + input.teamwork + input.innovation;
    bool exists = false;
    QString errorText;
    if (!scoreExists(input.employeeId, input.month, &exists, &errorText)) {
        return fail("校验绩效记录是否存在失败: " + errorText);
    }

    QSqlQuery query(m_db);
    if (exists) {
        query.prepare("UPDATE performance_scores SET attitude=?, capability=?, teamwork=?, innovation=?, "
                      "score=?, comment=?, status=?, evaluator=?, created_at=CURRENT_TIMESTAMP "
                      "WHERE emp_id=? AND eval_month=?");
        query.addBindValue(input.attitude);
        query.addBindValue(input.capability);
        query.addBindValue(input.teamwork);
        query.addBindValue(input.innovation);
        query.addBindValue(total);
        query.addBindValue(input.comment);
        query.addBindValue(HR::PerformanceStatus::PUBLISHED);
        query.addBindValue(input.evaluator);
        query.addBindValue(input.employeeId);
        query.addBindValue(input.month);
    } else {
        query.prepare("INSERT INTO performance_scores(emp_id, eval_month, attitude, capability, teamwork, "
                      "innovation, score, comment, status, evaluator) "
                      "VALUES(?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(input.employeeId);
        query.addBindValue(input.month);
        query.addBindValue(input.attitude);
        query.addBindValue(input.capability);
        query.addBindValue(input.teamwork);
        query.addBindValue(input.innovation);
        query.addBindValue(total);
        query.addBindValue(input.comment);
        query.addBindValue(HR::PerformanceStatus::PUBLISHED);
        query.addBindValue(input.evaluator);
    }

    if (!DbQuery::execCurrent(query, &errorText)) {
        return fail(errorText);
    }
    query.finish();

    Result result;
    result.success = true;
    result.totalScore = total;
    result.logAction = "绩效评分";
    result.logDetails = QString("%1 %2月 态度%3 能力%4 协作%5 创新%6 → 总分%7")
        .arg(input.employeeName, input.month)
        .arg(input.attitude)
        .arg(input.capability)
        .arg(input.teamwork)
        .arg(input.innovation)
        .arg(total);
    result.message = QString("%1月份绩效已提交，总分: %2").arg(input.month).arg(total);
    return result;
}

PerformanceService::Result PerformanceService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}

bool PerformanceService::scoreExists(int employeeId, const QString &month, bool *exists, QString *errorText) const
{
    if (exists) {
        *exists = false;
    }
    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT COUNT(*) FROM performance_scores WHERE emp_id=? AND eval_month=?",
                               {employeeId, month}, errorText)) {
        return false;
    }
    if (query.next() && exists) {
        *exists = query.value(0).toInt() > 0;
    }
    query.finish();
    return true;
}
