# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

This is a Qt6 C++17 CMake project developed in Qt Creator with MinGW 64-bit on Windows.

```bash
# Configure (from project root, with Qt6 prefix path)
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:/Qt/6.5.3/mingw_64" -DCMAKE_CXX_COMPILER="D:/Qt/Tools/mingw1120_64/bin/g++.exe"

# Build
cmake --build build

# Run the built executable
./build/HRMS.exe
```

Qt6 dependencies: Core, Gui, Widgets, Sql, Network, Charts. MOC/UIC/RCC are auto-run via `CMAKE_AUTOMOC`, `CMAKE_AUTOUIC`, `CMAKE_AUTORCC`.

## Architecture

**HRMS** — 人力资源管理系统 (Human Resource Management System). A Qt Widgets desktop app backed by MySQL via QODBC.

### Window flow

`main.cpp` reads `config.ini` (multi-path search), opens the MySQL connection (QODBC driver, database `hrms_db`), then shows `LoginWindow`. On successful login, `LoginWindow` creates a `MainWindow` (passing `empId`, `role`) and closes itself. MainWindow writes a login audit log entry on construction.

### Source files

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point: config.ini parsing, DB connection, launch LoginWindow |
| `loginwindow.h/cpp/ui` | Login form: name/phone + password auth, RBAC role extraction |
| `mainwindow.h/cpp/ui` | Main window: 5 tabs, system menu, status bar, all business logic |
| `changepassworddialog.h/cpp` | Password change dialog (old password verify + new password confirm) |
| `serversettingsdialog.h/cpp` | Server connection settings dialog |
| `CMakeLists.txt` | CMake config (Qt6 Core/Gui/Widgets/Sql/Network/Charts) |
| `config.ini` | Database connection config (gitignored) |
| `auditlog.sql` | DDL reference for audit_logs table |

### Models & tabs

`MainWindow` constructor sets up five tabs:

| Tab | Model | Table | Notes |
|-----|-------|-------|-------|
| 员工信息管理 | `QSqlTableModel` | `employees` | Editable (double-click), manual submit, password hidden, 12 visible columns |
| 考勤与请假审批 | `QSqlRelationalTableModel` | `leave_requests` | Read-only, emp_id resolved to name via QSqlRelation |
| 薪酬管理 | `QSqlRelationalTableModel` | `payroll` | Read-only, emp_id resolved to name |
| 操作日志 | `QSqlTableModel` | `audit_logs` | Read-only, admin only, sorted by time descending |
| 统计报表 | `QChartView` | — | Pie/bar charts via Qt Charts, PDF export, admin only |

### RBAC permission model

`currentRole` is either `"user"` (普通员工) or `"admin"` (管理员). In `user` mode:
- Tab 0 (员工信息管理), Tab 3 (操作日志), Tab 4 (统计报表) — hidden
- Approve/Reject buttons hidden
- Calculate payroll button hidden
- Leave and payroll tables filtered to the current employee's `emp_id`
- System menu (修改密码/退出登录) still accessible

### Database

MySQL 8.x accessed via ODBC driver `MySQL ODBC 9.6 UNICODE Driver`. Connection parameters read from `config.ini`.

Tables: `employees`, `leave_requests`, `payroll`, `audit_logs`.

Login authenticates by matching name or phone + password_hash against the `employees` table. SQL uses parameterized queries (`prepare` + `bindValue`).

Table migrations run automatically in the constructor:
- `audit_logs`: `CREATE TABLE IF NOT EXISTS`
- `employees` extended columns (education, marital_status, position): `SHOW COLUMNS` check then `ALTER TABLE ADD COLUMN`

### Payroll calculation logic

`on_btnCalculatePayroll()`: iterates all employees, sums approved leave days within the current month, calculates deduction as `(baseSalary / 21.75) * leaveDays`, inserts into `payroll` table. Detects duplicate month calculation and offers overwrite.

### Audit logging

`logAction(action, target)` writes to `audit_logs` table. Logged events: login, logout, add/delete/save employee, leave apply/approve/reject, payroll calculate, password change, status toggle.

### Statistical reports

`refreshChart()` queries the database and renders one of four chart types via Qt Charts:
1. 部门人数分布 (QPieSeries)
2. 在职/离职比例 (QPieSeries)
3. 各部门平均薪资 (QBarSeries)
4. 月度请假统计 (QBarSeries)

PDF export uses QPdfWriter + QPainter to render the current chart to A4 landscape at 300 DPI.
