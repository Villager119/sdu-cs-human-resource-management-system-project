#include "EmployeeService.h"

#include "../core/Constants.h"

#include <QCryptographicHash>
#include <QDate>
#include <QRegularExpression>
#include <QSqlQuery>

EmployeeService::EmployeeService(const QSqlDatabase &db)
    : m_db(db)
{
}

QString EmployeeService::defaultPasswordHash() const
{
    return QString(QCryptographicHash::hash(HR::DEFAULT_PASSWORD.toUtf8(),
                                            QCryptographicHash::Sha256).toHex());
}

QString EmployeeService::toggledEmploymentStatus(const QString &currentStatus) const
{
    return currentStatus == HR::EmpStatus::ACTIVE
        ? HR::EmpStatus::INACTIVE
        : HR::EmpStatus::ACTIVE;
}

bool EmployeeService::validateEmployeeRecord(const EmployeeRecord &record, int displayRow, QString *errorMessage)
{
    auto fail = [displayRow, errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = QString("第 %1 行：%2").arg(displayRow).arg(message);
        }
        return false;
    };

    if (record.name.isEmpty()) return fail("姓名不能为空");
    if (!isValidEmployeeName(record.name)) {
        return fail("姓名只能包含中文、英文字母或间隔点，长度需为 2-30 个字符，不能包含数字和特殊符号");
    }
    if (record.gender != "男" && record.gender != "女") return fail("性别必须是 '男' 或 '女'");

    if (record.phone.isEmpty()) return fail("联系电话不能为空");
    if (!QRegularExpression("^\\d{11}$").match(record.phone).hasMatch()) {
        return fail("联系电话格式不正确（必须为 11 位数字）");
    }

    if (record.department.isEmpty()) return fail("所属部门不能为空");
    if (!departmentExists(record.department)) {
        return fail(QString("部门 '%1' 在系统中不存在，请先在部门管理中创建该部门，或选择已有部门")
                    .arg(record.department));
    }

    if (record.position.isEmpty()) return fail("岗位不能为空");
    if (record.title.isEmpty()) return fail("职称不能为空");
    if (!roleExists(record.role)) return fail(QString("系统角色 '%1' 在系统中不存在").arg(record.role));

    const QStringList validStatus = {
        HR::EmpStatus::ACTIVE,
        HR::EmpStatus::INACTIVE,
        HR::EmpStatus::TRANSFERRED_OUT,
        HR::EmpStatus::RESIGNED,
        HR::EmpStatus::DISMISSED,
        HR::EmpStatus::RETIRED
    };
    if (!validStatus.contains(record.status)) return fail("在职状态无效");

    const QStringList validEducation = {"大专", "本科", "硕士", "博士"};
    if (!validEducation.contains(record.education)) return fail("学历必须是 大专/本科/硕士/博士 之一");

    const QStringList validMaritalStatus = {"未婚", "已婚", "离异"};
    if (!validMaritalStatus.contains(record.maritalStatus)) return fail("婚姻状况必须是 未婚/已婚/离异 之一");

    if (record.baseSalary.toString().trimmed().isEmpty()) return fail("基础薪资不能为空");
    bool salaryOk = false;
    const double salary = record.baseSalary.toDouble(&salaryOk);
    if (!salaryOk || salary <= 0) return fail("基础薪资必须为大于 0 的数值");

    const QDate hireDate = parseDate(record.hireDate);
    if (!hireDate.isValid()) return fail("入职日期无效或格式不正确（需 yyyy-MM-dd）");

    if (!record.contractEndDate.toString().trimmed().isEmpty() && !record.contractEndDate.isNull()) {
        const QDate contractDate = parseDate(record.contractEndDate);
        if (!contractDate.isValid()) return fail("合同到期日期格式不正确（需 yyyy-MM-dd）");
        if (contractDate < hireDate) return fail("合同到期日期不能早于入职日期");
    }

    const JobSalaryStandard standard = jobSalaryStandard(record.department, record.position, record.title);
    if (!standard.found) {
        return fail(QString("部门 '%1' 下不存在岗位 '%2' 与职称 '%3' 的薪资标准，请先在岗位薪资标准中配置")
                    .arg(record.department, record.position, record.title));
    }
    if (salary < standard.minSalary || salary > standard.maxSalary) {
        return fail(QString("%1 / %2 / %3 的基础薪资范围为 %4-%5 元，建议默认薪资为 %6 元")
                    .arg(record.department, record.position, record.title)
                    .arg(standard.minSalary, 0, 'f', 2)
                    .arg(standard.maxSalary, 0, 'f', 2)
                    .arg(standard.defaultSalary, 0, 'f', 2));
    }

    return true;
}

bool EmployeeService::isValidEmployeeName(const QString &name) const
{
    static const QRegularExpression re(QStringLiteral("^[\\p{Han}A-Za-z·]{2,30}$"));
    return re.match(name.trimmed()).hasMatch();
}

bool EmployeeService::departmentExists(const QString &department) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM departments WHERE dept_name=?");
    query.addBindValue(department);

    const bool exists = query.exec() && query.next() && query.value(0).toInt() > 0;
    query.finish();
    return exists;
}

bool EmployeeService::roleExists(const QString &role) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM roles WHERE role_name=?");
    query.addBindValue(role);

    const bool exists = query.exec() && query.next() && query.value(0).toInt() > 0;
    query.finish();
    return exists;
}

EmployeeService::JobSalaryStandard EmployeeService::jobSalaryStandard(const QString &department,
                                                                       const QString &position,
                                                                       const QString &title) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT s.min_salary, s.max_salary, s.default_salary "
                  "FROM job_salary_standards s "
                  "JOIN departments d ON d.dept_id=s.dept_id "
                  "WHERE d.dept_name=? AND s.position=? AND s.title=? AND s.enabled=1");
    query.addBindValue(department);
    query.addBindValue(position);
    query.addBindValue(title);

    JobSalaryStandard standard;
    if (query.exec() && query.next()) {
        standard.found = true;
        standard.minSalary = query.value(0).toDouble();
        standard.maxSalary = query.value(1).toDouble();
        standard.defaultSalary = query.value(2).toDouble();
    }
    query.finish();
    return standard;
}

QDate EmployeeService::parseDate(const QVariant &value) const
{
    QDate date = value.toDate();
    if (!date.isValid()) {
        date = QDate::fromString(value.toString(), "yyyy-MM-dd");
    }
    return date;
}
