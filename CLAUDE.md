# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

This is a Qt6 C++17 CMake project developed in Qt Creator with MinGW 64-bit on Windows.

```bash
# Configure (from project root)
cmake -B build -S . -G "MinGW Makefiles"

# Build
cmake --build build

# Run the built executable
./build/HRMS.exe
```

Qt6 dependencies: Core, Gui, Widgets, Sql. MOC/UIC/RCC are auto-run via `CMAKE_AUTOMOC`, `CMAKE_AUTOUIC`, `CMAKE_AUTORCC`.

## Architecture

**HRMS** — 人力资源管理系统 (Human Resource Management System). A Qt Widgets desktop app backed by MySQL via QODBC.

### Window flow

`main.cpp` opens the MySQL connection (QODBC driver, database `hrms_db`), then shows `LoginWindow`. On successful login, `LoginWindow` creates a `MainWindow` (passing `empId` and `role`) and closes itself.

### Models & tabs

`MainWindow` constructor (mainwindow.cpp:8-93) sets up three tabs:

| Tab | Model | Table | Notes |
|-----|-------|-------|-------|
| 员工信息管理 | `QSqlTableModel` | `employees` | Editable (double-click), manual submit, password column hidden |
| 考勤与请假审批 | `QSqlRelationalTableModel` | `leave_requests` | Read-only, emp_id resolved to name via QSqlRelation |
| 薪酬管理 | `QSqlRelationalTableModel` | `payroll` | Read-only, emp_id resolved to name |

### RBAC permission model

`currentRole` is either `"user"` (普通员工) or `"admin"` (管理员). In `user` mode (mainwindow.cpp:81-89):
- Approve/Reject buttons hidden
- Calculate payroll button hidden
- Employee management tab hidden entirely
- Leave and payroll tables filtered to the current employee's `emp_id`

### Database

MySQL 8.x accessed via ODBC driver `MySQL ODBC 9.6 UNICODE Driver`. Connection string hardcoded in `main.cpp` (server=127.0.0.1:3306, database=hrms_db).

Tables: `employees`, `leave_requests`, `payroll`.

Login authenticates by matching name or phone + password_hash against the `employees` table. SQL uses parameterized queries (`prepare` + `bindValue`).

### Payroll calculation logic

`on_btnCalculatePayroll()` (mainwindow.cpp:220-291): iterates all employees, sums approved leave days within the current month, calculates deduction as `(baseSalary / 21.75) * leaveDays`, inserts into `payroll` table. Detects duplicate month calculation and offers overwrite.
