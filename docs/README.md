# HRMS - 人力资源管理系统

基于 Qt6 + C++17 + MySQL 的桌面端人事管理系统，采用 C/S 两层架构。

## 技术栈

- **语言**: C++17
- **UI 框架**: Qt 6.5 (Widgets, Sql, Charts, Network)
- **数据库**: MySQL 8.x (QODBC 驱动)
- **构建**: CMake + MinGW 64-bit (Windows)
- **版本控制**: Git

## 功能

| 模块 | 说明 |
|------|------|
| 仪表盘首页 | 总人数、在职/离职统计、请假/待审批数、薪资总额（仅管理员） |
| 员工信息管理 | 增删改查 + 多维筛选（部门/状态/姓名）+ 状态流转 + 扩展字段（学历/婚姻/岗位） |
| 考勤与请假审批 | 请假申请 + 管理员审批（同意/拒绝） |
| 薪酬管理 | 一键核算（21.75 计薪 + 请假扣款）+ 工资查询 + CSV 导出 |
| 操作日志 | 全操作审计追踪（11 类操作自动记录，仅管理员） |
| 统计报表 | 饼图/柱状图（4 种图表类型切换）+ PDF 导出（仅管理员） |
| 系统功能 | 密码修改（SHA-256 哈希）、退出登录、服务器连接配置 |

## 构建与运行

### 依赖

- Qt 6.5+ (Core, Gui, Widgets, Sql, Charts, Network)
- MinGW 64-bit (推荐 Qt 自带的 mingw1120_64)
- MySQL 8.x 服务端
- MySQL ODBC 9.6 UNICODE Driver

### 构建

```bash
cmake -B build -S . -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH="D:/Qt/6.5.3/mingw_64" \
  -DCMAKE_CXX_COMPILER="D:/Qt/Tools/mingw1120_64/bin/g++.exe"
cmake --build build
```

### 运行

1. 确保 MySQL 服务运行，`hrms_db` 数据库已创建
2. 配置 `config.ini`（数据库连接参数）
3. 运行 `./build/HRMS.exe`

## 项目结构

```
HRMS/
├── main.cpp                 # 入口：读取 config.ini、DB 连接、启动登录
├── loginwindow.h/cpp/ui     # 登录窗口：认证 + RBAC
├── mainwindow.h/cpp/ui      # 主窗口：5 个 Tab + 全业务逻辑
├── changepassworddialog.h/cpp # 密码修改对话框
├── serversettingsdialog.h/cpp # 服务器连接设置对话框
├── CMakeLists.txt           # CMake 构建配置
├── config.ini               # 数据库连接配置（gitignored）
├── auditlog.sql             # 审计日志表 DDL
├── SRS草稿.md               # 软件需求规格说明书 V2.0
├── 可行性分析报告.md          # 可行性分析报告
├── 实验四.md                # 项目进度甘特图
└── CLAUDE.md                # AI 辅助开发文档
```

## 数据库

4 张表：`employees`、`leave_requests`、`payroll`、`audit_logs` + 1 张辅助表 `departments`。表结构自动迁移（`CREATE TABLE IF NOT EXISTS` + `ALTER TABLE ADD COLUMN`）。

## 权限

| 角色 | 可见 Tab | 功能限制 |
|------|---------|---------|
| admin | 全部（含仪表盘/员工管理/日志/报表） | 全部功能 |
| user | 请假审批、薪酬管理 | 仅可查看和操作自身数据 |
