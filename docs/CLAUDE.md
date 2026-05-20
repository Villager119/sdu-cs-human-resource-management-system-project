# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Build & Run

Qt6 C++17 CMake project, MinGW 64-bit on Windows.

```bash
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:/Qt/6.5.3/mingw_64" -DCMAKE_CXX_COMPILER="D:/Qt/Tools/mingw1120_64/bin/g++.exe"
cmake --build build
./build/HRMS.exe
```

Depends: Qt6 Core/Gui/Widgets/Sql/Network/Charts. AUTOMOC/AUTOUIC/AUTORCC enabled.

## Architecture

HRMS — C/S desktop personnel management. Qt Widgets + MySQL via QODBC.

### Directory structure

```
HRMS/
├── CMakeLists.txt, config.ini, style.qss
├── docs/          (documents)
└── src/
    ├── main.cpp
    ├── ui/         LoginWindow, MainWindow, ChangePasswordDialog, ServerSettingsDialog
    ├── tabs/       10 business tabs (Dashboard/Employee/Leave/Payroll/Audit/Reports/Performance/Org/ProfileChange/AttendTax)
    ├── widgets/    PaginationBar, ComboDelegate
    └── core/       Constants, Logger, SessionManager, GlobalEvents
```

### Navigation (sidebar, 6 items)

| Nav | Sub-tabs inside | Admin-only |
|-----|-----------------|------------|
| 🏠 首页 | 仪表盘 + 统计图表(5种) | ✓ |
| 👥 员工 | 员工CRUD + 信息变更审批 | ✓ |
| ⏰ 考勤 | 打卡补卡 + 请假审批 | - |
| 💰 薪酬 | 工资条 + 绩效评分 | - |
| 🏢 组织 | 部门树 + 员工列表 | ✓ |
| 📜 日志 | 审计日志 | ✓ |

### Database (MySQL 8.x, QODBC)

Tables: employees, leave_requests, payroll, audit_logs, departments, performance_scores, notifications, profile_change_requests, attendances, makeup_requests, shifts, salary_config, system_settings

Migrations: CREATE TABLE IF NOT EXISTS + SHOW COLUMNS check + ALTER TABLE ADD COLUMN (all idempotent, run on startup).

### Key features
- SHA-256 password hashing, Base64 config.ini obfuscation
- Payroll: 21.75 work days + leave deduction + social insurance (五险一金) + progressive tax + performance bonus
- Charts: 5 types via Qt Charts (pie/bar/line), PDF export via QPdfWriter
- Notifications: bell icon with unread count, auto-notify on approval/rejection
- Audit: all CRUD operations logged to audit_logs
- RBAC: sidebar item visibility + data filter per role
