#include "DbUtils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>
#include <QList>
#include <QDate>
#include <QByteArray>

QString buildDsn(const QString &driver, const QString &server, int port,
                 const QString &database, const QString &uid, const QString &pwd)
{
    return QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
        .arg(driver, server)
        .arg(port)
        .arg(database, uid, pwd);
}

QString decodeConfigPassword(const QString &storedPassword)
{
    if (storedPassword.isEmpty()) {
        return {};
    }

    QByteArray decoded = QByteArray::fromBase64(storedPassword.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    return decoded.isEmpty() ? storedPassword : QString::fromUtf8(decoded);
}

QSqlDatabase createClonedDatabaseConnection(const QString &prefix)
{
    static int sequence = 0;
    const QString connectionName = QString("hrms_%1_%2").arg(prefix).arg(++sequence);
    QSqlDatabase db = QSqlDatabase::cloneDatabase(QSqlDatabase::database(), connectionName);
    if (!db.open()) {
        qWarning() << "Failed to open cloned DB connection" << connectionName << db.lastError().text();
    }
    return db;
}

bool logSqlError(const QSqlQuery &query, const QString &context)
{
    const QSqlError error = query.lastError();
    if (!error.isValid()) {
        return false;
    }

    qWarning() << context << "failed:" << error.text();
    return true;
}

bool initDatabaseSchema()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "Database connection is not open during schema initialization.";
        return false;
    }

    db.transaction();
    QSqlQuery q;

    // 1. roles
    if (!q.exec("CREATE TABLE IF NOT EXISTS roles ("
                "role_id INT AUTO_INCREMENT PRIMARY KEY,"
                "role_name VARCHAR(50) NOT NULL UNIQUE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create roles table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 2. permissions
    if (!q.exec("CREATE TABLE IF NOT EXISTS permissions ("
                "permission_id INT AUTO_INCREMENT PRIMARY KEY,"
                "permission_key VARCHAR(50) NOT NULL UNIQUE,"
                "permission_name VARCHAR(100) NOT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create permissions table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 3. role_permissions
    if (!q.exec("CREATE TABLE IF NOT EXISTS role_permissions ("
                "role_id INT NOT NULL,"
                "permission_id INT NOT NULL,"
                "PRIMARY KEY (role_id, permission_id),"
                "FOREIGN KEY (role_id) REFERENCES roles(role_id) ON DELETE CASCADE,"
                "FOREIGN KEY (permission_id) REFERENCES permissions(permission_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create role_permissions table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 4. employees
    if (!q.exec("CREATE TABLE IF NOT EXISTS employees ("
                "emp_id INT AUTO_INCREMENT PRIMARY KEY,"
                "name VARCHAR(50) NOT NULL,"
                "gender VARCHAR(10) DEFAULT '男',"
                "phone VARCHAR(20) DEFAULT NULL,"
                "department VARCHAR(50) DEFAULT NULL,"
                "role VARCHAR(50) DEFAULT 'user',"
                "password_hash VARCHAR(255) NOT NULL DEFAULT '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92',"
                "base_salary DECIMAL(10,2) DEFAULT 0.00,"
                "hire_date DATE DEFAULT NULL,"
                "contract_end_date DATE DEFAULT NULL,"
                "status VARCHAR(20) DEFAULT '在职',"
                "title VARCHAR(50) DEFAULT '',"
                "education VARCHAR(50) DEFAULT '',"
                "marital_status VARCHAR(50) DEFAULT '',"
                "position VARCHAR(50) DEFAULT '',"
                "version INT DEFAULT 1"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create employees table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 5. departments
    if (!q.exec("CREATE TABLE IF NOT EXISTS departments ("
                "dept_id INT AUTO_INCREMENT PRIMARY KEY,"
                "dept_name VARCHAR(50) NOT NULL UNIQUE,"
                "parent_id INT DEFAULT NULL,"
                "manager_id INT DEFAULT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create departments table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 6. shifts
    if (!q.exec("CREATE TABLE IF NOT EXISTS shifts ("
                "shift_id INT AUTO_INCREMENT PRIMARY KEY,"
                "shift_name VARCHAR(30) NOT NULL,"
                "start_time TIME NOT NULL,"
                "end_time TIME NOT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create shifts table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 7. attendances
    if (!q.exec("CREATE TABLE IF NOT EXISTS attendances ("
                "att_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "att_date DATE NOT NULL,"
                "clock_in TIME,"
                "clock_out TIME,"
                "status VARCHAR(20) DEFAULT '正常',"
                "remark TEXT,"
                "UNIQUE KEY uk_emp_date (emp_id, att_date),"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create attendances table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 8. makeup_requests
    if (!q.exec("CREATE TABLE IF NOT EXISTS makeup_requests ("
                "makeup_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "att_date DATE NOT NULL,"
                "request_type VARCHAR(10) NOT NULL,"
                "request_time TIME NOT NULL,"
                "reason TEXT NOT NULL,"
                "status VARCHAR(20) DEFAULT '待审批',"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create makeup_requests table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 9. salary_config
    if (!q.exec("CREATE TABLE IF NOT EXISTS salary_config ("
                "config_id INT AUTO_INCREMENT PRIMARY KEY,"
                "item_name VARCHAR(50) NOT NULL UNIQUE,"
                "rate_personal DECIMAL(5,4) NOT NULL DEFAULT 0.0000,"
                "enabled TINYINT DEFAULT 1"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create salary_config table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 10. system_settings
    if (!q.exec("CREATE TABLE IF NOT EXISTS system_settings ("
                "key_name VARCHAR(50) PRIMARY KEY,"
                "value VARCHAR(200) NOT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create system_settings table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 11. performance_scores
    if (!q.exec("CREATE TABLE IF NOT EXISTS performance_scores ("
                "score_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "eval_month VARCHAR(7) NOT NULL,"
                "attitude DECIMAL(5,2) DEFAULT 0.00,"
                "capability DECIMAL(5,2) DEFAULT 0.00,"
                "teamwork DECIMAL(5,2) DEFAULT 0.00,"
                "innovation DECIMAL(5,2) DEFAULT 0.00,"
                "score DECIMAL(5,2) NOT NULL,"
                "comment TEXT,"
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create performance_scores table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 12. profile_change_requests
    if (!q.exec("CREATE TABLE IF NOT EXISTS profile_change_requests ("
                "request_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "field_name VARCHAR(30) NOT NULL,"
                "old_value VARCHAR(200) DEFAULT NULL,"
                "new_value VARCHAR(200) NOT NULL,"
                "status VARCHAR(20) DEFAULT '待审批',"
                "reason TEXT,"
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create profile_change_requests table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 13. notifications
    if (!q.exec("CREATE TABLE IF NOT EXISTS notifications ("
                "notif_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "title VARCHAR(100) NOT NULL,"
                "content VARCHAR(200) DEFAULT NULL,"
                "is_read TINYINT DEFAULT 0,"
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create notifications table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 14. audit_logs
    if (!q.exec("CREATE TABLE IF NOT EXISTS audit_logs ("
                "log_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "emp_name VARCHAR(50) NOT NULL,"
                "action VARCHAR(100) NOT NULL,"
                "target VARCHAR(200) DEFAULT '',"
                "log_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create audit_logs table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 15. leave_requests
    if (!q.exec("CREATE TABLE IF NOT EXISTS leave_requests ("
                "request_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "start_date DATE NOT NULL,"
                "end_date DATE NOT NULL,"
                "reason VARCHAR(255) DEFAULT NULL,"
                "status VARCHAR(20) DEFAULT '待审批',"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create leave_requests table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // 16. payroll
    if (!q.exec("CREATE TABLE IF NOT EXISTS payroll ("
                "payroll_id INT AUTO_INCREMENT PRIMARY KEY,"
                "emp_id INT NOT NULL,"
                "month VARCHAR(7) NOT NULL,"
                "base_salary DECIMAL(10,2) NOT NULL,"
                "leave_deduction DECIMAL(10,2) DEFAULT 0.00,"
                "performance_bonus DECIMAL(10,2) DEFAULT 0.00,"
                "pension DECIMAL(10,2) DEFAULT 0.00,"
                "medical DECIMAL(10,2) DEFAULT 0.00,"
                "unemployment DECIMAL(10,2) DEFAULT 0.00,"
                "housing_fund DECIMAL(10,2) DEFAULT 0.00,"
                "income_tax DECIMAL(10,2) DEFAULT 0.00,"
                "net_salary DECIMAL(10,2) NOT NULL,"
                "issue_date DATE DEFAULT NULL,"
                "FOREIGN KEY (emp_id) REFERENCES employees(emp_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create payroll table:" << q.lastError().text();
        db.rollback();
        return false;
    }

    // Seed default roles
    q.exec("INSERT IGNORE INTO roles (role_name) VALUES ('admin'), ('user')");
    q.finish();

    // Seed default permissions
    q.exec("INSERT IGNORE INTO permissions (permission_key, permission_name) VALUES "
           "('view_dashboard', '查看仪表盘'),"
           "('manage_employees', '管理员工信息'),"
           "('request_profile_change', '申请信息变更'),"
           "('approve_profile_change', '审批信息变更'),"
           "('apply_leave_makeup', '打卡与补签'),"
           "('apply_leave', '请假申请'),"
           "('approve_leave', '审批请假'),"
           "('approve_makeup', '审批补卡'),"
           "('view_personal_payroll', '查看个人工资条'),"
           "('calculate_payroll', '核算发放工资'),"
           "('view_personal_performance', '查看个人绩效'),"
           "('evaluate_performance', '评估绩效'),"
           "('manage_org', '组织架构管理'),"
           "('view_audit_logs', '查看审计日志'),"
           "('manage_rbac', '管理角色权限'),"
           "('manage_tax_config', '社保比例配置'),"
           "('view_reports', '查看统计报表')");
    q.finish();

    // Seed default role-permission mappings for 'user' role
    q.exec("INSERT IGNORE INTO role_permissions (role_id, permission_id) "
           "SELECT (SELECT role_id FROM roles WHERE role_name='user'), permission_id "
           "FROM permissions WHERE permission_key IN "
           "('view_dashboard', 'request_profile_change', 'apply_leave_makeup', "
           "'apply_leave', 'view_personal_payroll', 'view_personal_performance')");
    q.finish();

    // Seed default role-permission mappings for 'admin' role
    q.exec("INSERT IGNORE INTO role_permissions (role_id, permission_id) "
           "SELECT (SELECT role_id FROM roles WHERE role_name='admin'), permission_id "
           "FROM permissions");
    q.finish();

    // Seed default shifts
    q.exec("INSERT IGNORE INTO shifts (shift_name, start_time, end_time) VALUES ('标准班', '09:00:00', '18:00:00')");
    q.finish();

    // Seed default salary configs
    q.exec("INSERT IGNORE INTO salary_config (item_name, rate_personal) VALUES "
           "('养老保险', 0.0800), ('医疗保险', 0.0200), ('失业保险', 0.0050),"
           "('工伤保险', 0.0000), ('生育保险', 0.0000), ('住房公积金', 0.1200)");
    q.finish();

    // Seed default system settings
    q.exec("INSERT IGNORE INTO system_settings (key_name, value) VALUES ('work_days_per_month', '21.75'), ('tax_threshold', '5000')");
    q.finish();

    // Check if employees is empty. If so, seed initial admin user (admin / admin1)
    q.exec("SELECT COUNT(*) FROM employees");
    bool employeesEmpty = false;
    if (q.next() && q.value(0).toInt() == 0) {
        employeesEmpty = true;
    }
    q.finish();
    if (employeesEmpty) {
        q.prepare("INSERT INTO employees (name, phone, role, password_hash, hire_date, base_salary, status) "
                  "VALUES ('admin', '13800000000', 'admin', '25f43b1486ad95a1398e3eeb3d83bc4010015fcc9bedb35b432e00298d5021f7', ?, 8000.00, '在职')");
        q.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
        if (!q.exec()) {
            qDebug() << "Failed to seed default admin user:" << q.lastError().text();
            db.rollback();
            return false;
        }
        q.finish();
    }

    // 17. Migration: check if status and evaluator columns exist in performance_scores
    {
        bool statusExists = false;
        {
            QSqlQuery ck;
            if (ck.exec("SHOW COLUMNS FROM performance_scores LIKE 'status'") && ck.next()) {
                statusExists = true;
            }
            ck.finish();
        }

        if (!statusExists) {
            QSqlQuery q;
            if (!q.exec("ALTER TABLE performance_scores ADD COLUMN status VARCHAR(20) DEFAULT '已发布'")) {
                qDebug() << "Failed to add status column to performance_scores:" << q.lastError().text();
            }
            q.finish();
        }

        bool evaluatorExists = false;
        {
            QSqlQuery ck;
            if (ck.exec("SHOW COLUMNS FROM performance_scores LIKE 'evaluator'") && ck.next()) {
                evaluatorExists = true;
            }
            ck.finish();
        }

        if (!evaluatorExists) {
            QSqlQuery q;
            if (!q.exec("ALTER TABLE performance_scores ADD COLUMN evaluator VARCHAR(50) DEFAULT '系统管理员'")) {
                qDebug() << "Failed to add evaluator column to performance_scores:" << q.lastError().text();
            }
            q.finish();
        }
    }

    // 18. Migration: check if version column exists in employees
    {
        bool versionExists = false;
        {
            QSqlQuery ck;
            if (ck.exec("SHOW COLUMNS FROM employees LIKE 'version'") && ck.next()) {
                versionExists = true;
            }
            ck.finish();
        }

        if (!versionExists) {
            QSqlQuery q;
            if (!q.exec("ALTER TABLE employees ADD COLUMN version INT DEFAULT 1")) {
                qDebug() << "Failed to add version column to employees:" << q.lastError().text();
            }
            q.finish();
        }
    }

    db.commit();
    qDebug() << "Database schema initialized successfully.";
    return true;
}
