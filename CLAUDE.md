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

HRMS — two-tier C/S desktop personnel management system. Qt Widgets client connects directly to MySQL 8.x via QODBC.

### Directory structure

```
HRMS/
├── CMakeLists.txt, config.ini, style.qss
├── docs/          (SRS, SAD, defense guide, feasibility report, etc.)
└── src/
    ├── main.cpp           Entry point: logger init, QSS loading, DB connect, schema migration, LoginWindow
    ├── ui/                LoginWindow, MainWindow, ChangePasswordDialog, ServerSettingsDialog, RecoverPasswordDialog
    ├── tabs/              10 business tabs (see below)
    ├── widgets/           PaginationBar, ComboDelegate, SafeEditDelegate, TaxConfigPanel, OrgChartView
    ├── utils/             DbUtils (DSN builder + schema migration), CsvExport, Toast
    └── core/              Constants (HR namespace), Logger, SessionManager (singleton, RBAC), GlobalEvents (event bus)
```

### Navigation (sidebar, 8 items, collapsible)

| Nav | Stacked page | Sub-tabs inside | Permission gates |
|-----|-------------|-----------------|------------------|
| 🏠 首页 | DashboardTab only | — | view_dashboard |
| 👥 员工 | QTabWidget wrapper | 员工管理 (EmployeeTab) + 信息变更 (ProfileChangeTab) | manage_employees / request_profile_change |
| ⏰ 我的考勤 | MyAttendanceTab (direct) | 打卡 + 请假申请 + 补卡申请 | apply_leave_makeup / apply_leave |
| 📊 考勤管理 | AttendManageTab (direct) | 考勤面板 + 审批中心(请假审批 + 补卡审批) | approve_leave / approve_makeup |
| 💰 薪酬 | QTabWidget wrapper | 工资条 (PayrollTab) + 绩效评分 (PerformanceTab) + 社保配置 (TaxConfigPanel) | view_personal_payroll / calculate_payroll / manage_tax_config |
| 🏢 组织 | OrgTab (direct) | 部门树 + 员工列表 + 组织架构图 | manage_org |
| 📜 日志 | AuditTab (direct) | 审计日志只读表格 | view_audit_logs |
| 🔑 权限 | RbacTab (direct) | 角色管理 + 权限分配 | manage_rbac |

### Database (MySQL 8.x, QODBC — MySQL ODBC 9.6 Unicode driver)

16 tables (all InnoDB, utf8mb4): employees, departments, roles, permissions, role_permissions, attendances, makeup_requests, leave_requests, payroll, salary_config, system_settings, performance_scores, profile_change_requests, notifications, audit_logs, shifts.

Schema migration: `initDatabaseSchema()` runs on every startup — idempotent `CREATE TABLE IF NOT EXISTS` + seed data (default roles, shifts, salary config, system settings, admin user).

### Key patterns

- **Singleton + Event bus**: `SessionManager` holds current user session + RBAC permissions. `GlobalEvents` emits `dataChanged()` / `dashboardRefresh()` / `auditRefresh()` signals; MainWindow wires them to tab `refresh()` slots.
- **Callback injection**: Tabs receive `std::function` callbacks (`logFn`, `notifyFn`) instead of inheriting an abstract interface — lightweight DI.
- **RBAC**: `SessionManager::hasPermission(permKey)` gates sidebar visibility, sub-tab creation, and in-page data filtering. Admin role bypasses all checks.
- **Notification system**: Bell icon with 5-second polling on `notifications` table, unread count badge, flashing animation when unread. Permission-aware routing: notifications for leave/makeup/profile-change approvals go only to users with the relevant permission.
- **Payroll calculation**: 21.75 work-day standard, leave deduction, social insurance (五险一金 from salary_config), progressive income tax (from system_settings.tax_threshold), performance bonus — all in a MySQL transaction.
- **SHA-256** password hashing. **Base64** obfuscation for config.ini credentials. All CRUD operations logged to audit_logs.

### Coding conventions

- UI labels, log messages, comments, and constant values in Chinese. Class names and code identifiers in English.
- No `.qrc` resource file — stylesheet loaded from `style.qss` at runtime. Unicode emoji used as sidebar icons.
- `Toast` and `SafeEditDelegate` are header-only classes.
- Source files explicitly listed in CMakeLists.txt (no GLOB).
- New tabs: constructor takes `(empId, role, logFn, notifyFn, parent)` or `(logFn, parent)` for admin-only tabs.
- When adding a tab, register it in `MainWindow` constructor, `refreshActiveTab()`, `CMakeLists.txt`, and add a permission-gated nav item.
