-- 操作审计日志表
CREATE TABLE IF NOT EXISTS audit_logs (
    log_id INT PRIMARY KEY AUTO_INCREMENT,
    emp_id INT NOT NULL,
    emp_name VARCHAR(50) NOT NULL,
    action VARCHAR(100) NOT NULL,
    target VARCHAR(200) DEFAULT '',
    log_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (emp_id) REFERENCES employees(emp_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
