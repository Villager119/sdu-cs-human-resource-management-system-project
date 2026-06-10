# HRMS - 人力资源管理系统 (C++/Qt6/MySQL)

基于 **Qt6 + C++17 + MySQL** 的桌面端企业级人力资源管理系统 (HRMS)，专为软件工程与数据库系统课程设计优化。采用 C/S 二层架构，内置数据库自动表结构迁移、登录密码安全哈希与多标签页全局数据总线同步（观察者模式）等高级特性。

---

## 快速开始

```powershell
cmake -B build -S . -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.5.3/mingw_64" `
  -DCMAKE_CXX_COMPILER="D:/Qt/Tools/mingw1120_64/bin/g++.exe"

cmake --build build

# Windows 构建后默认调用 windeployqt 部署 Qt 运行时，可用启动烟测快速检查
build\HRMS.exe --test
```

默认演示账户：

| 角色 | 账号 | 密码 |
| :--- | :--- | :--- |
| 管理员 | `admin` 或 `13800000000` | `admin1` |
| 普通员工 | 管理员在员工管理中新建 | `123456` |

首次运行前需准备 MySQL 数据库 `hrms_db` 和 MySQL ODBC 驱动。若连接失败，登录窗口会提示“数据库未连接”，点击 **服务器设置** 填写连接参数后可重新连接并自动执行建表/迁移。

---

## 🌟 项目亮点

1. **数据库设计规范性**：
   - 数据库表结构完全符合第三范式 (3NF)。
   - 在操作审计日志表（`audit_logs`）中进行了合理的反规范化设计，折中查询效率与历史追溯真实性。
   - 严格遵循外键引用完整性约束，防止孤立与脏数据。
2. **ACID 事务保障**：
   - 薪酬一键核算业务采用 MySQL 事务处理，若在批量计算中途遇到任何异常均会自动回滚，确保数据强一致性。
3. **观察者模式数据同步**：
   - 采用 Qt 信号与槽结合全局单例事件总线（`GlobalEvents`）实现了**观察者模式**。在工资核算、请假审批、补卡审批、信息变更审批完成后，自动向全标签页广播并刷新数据，杜绝脏数据视觉。
4. **防御性编程（防呆设计）**：
   - 全面实现输入校验（姓名中文/英文合法性、手机号正则验证、工资数值非负验证、请假日期大小逻辑验证、空行选中安全拦截）。
   - 员工部门、岗位、职称与基础薪资通过岗位薪资标准联动校验，避免随意录入造成薪资体系混乱。
   - 数据库密码在本地配置文件 `config.ini` 中使用 Base64 编码遮蔽，避免直接肉眼可见明文密码。
5. **软工标准图表集成**：
   - 需求文档内置 Mermaid 编写的 **E-R图**、**一键核算工资数据流图(DFD)** 以及 **系统用例图(Use Case Diagram)**，便于直接导入课设报告中。
6. **动态 RBAC 权限系统**：
   - 17 项原子权限，数据库驱动的角色-权限映射，支持自定义角色与动态权限勾选，UI + 数据双层越权拦截。
7. **可视化组织架构图**：
   - 基于 Qt Graphics View 框架的交互式部门层级树状图，支持拖拽平移、无极缩放、点击联动选中编辑。
8. **QODBC 稳定性处理**：
   - 临时查询使用后显式释放语句结果，长生命周期 SQL Model 与后台轮询使用独立克隆连接，降低 QODBC 函数序列错误风险。
9. **代码可维护性整理**：
   - 认证、员工、考勤、审批、薪酬、绩效、组织、权限、通知和审计等业务逻辑抽取到 `src/services/`，UI 类主要负责交互与展示。
   - 数据库连接、迁移辅助和表结构初始化拆分到 `src/db/`，`DbUtils` 保留兼容入口，降低后续维护成本。
   - 抽取 `DbQuery`、`SqlFilterBuilder` 和 `PayrollCalculator`，统一临时查询、表格筛选条件和薪酬计算规则，减少散落在 UI/服务层中的重复逻辑。
   - 抽取 `UiStyles` 复用公共界面样式，减少重复 QSS 字符串。
10. **Windows 运行时部署优化**：
   - CMake 在 Windows 下默认启用 `HRMS_DEPLOY_QT_RUNTIME`，构建后调用 `windeployqt` 将 Qt DLL 与插件复制到可执行文件目录，降低 `Qt6Sql.dll` 等运行时依赖缺失概率。

---

## 🛠️ 技术栈

- **开发语言**: C++17
- **UI 框架**: Qt 6.5.3 (Widgets, Sql, Charts, Network, Concurrent)
- **数据库**: MySQL 8.x (通过 QODBC 驱动直连)
- **构建系统**: CMake 3.16+
- **编译器**: MinGW 64-bit (GCC 11.2)
- **操作系统**: Windows 10/11

---

## 文档索引

| 文档 | 用途 |
| :--- | :--- |
| [`docs/SRS正式稿.md`](docs/SRS正式稿.md) | 需求规格、功能清单、数据表和界面约束 |
| [`docs/SAD正式稿.md`](docs/SAD正式稿.md) | 架构视图、质量属性、架构决策和维护风险 |
| [`docs/可行性分析报告.md`](docs/可行性分析报告.md) | 技术选型、方案对比和课程设计可行性论证 |
| [`docs/CASE工具调研.md`](docs/CASE工具调研.md) | CASE/配置管理工具调研材料 |
| [`docs/项目进度与工作跟踪表.md`](docs/项目进度与工作跟踪表.md) | 周进度、成员分工、当前维护阶段与交付物跟踪 |

---

## 📦 系统功能结构

系统实行基于数据库动态配置的角色权限控制 (RBAC)，分为 **管理员 (admin)** 与 **普通员工 (user)** 两个默认角色，支持自定义角色扩展：

| 主导导航栏 | 包含子功能 Tab | 权限限制 | 说明 |
| :--- | :--- | :--- | :--- |
| **🏠 首页** | 仪表盘 + 统计图表 | 管理员 / 员工 | 管理员可看大盘看板与 5 种 Qt Charts 统计图表，员工仅看基础信息 |
| **👥 员工** | 员工管理 + 信息变更审批 | 按权限显示 | 管理员执行员工 CRUD、批量调部门；普通员工可进入信息变更页提交个人资料修改申请；审批记录保留审批人、时间与意见 |
| **⏰ 我的考勤** | 打卡 + 请假申请 + 补卡申请 | 均可见 (隔离) | 员工进行上班/下班打卡、提交请假及补卡申请 |
| **📊 考勤管理** | 考勤面板 + 审批中心 | 仅管理员可见 | 管理员查看全员考勤记录、批量审批请假与补卡申请并填写审批意见 |
| **💰 薪酬** | 工资条 + 绩效评分 + 社保配置 | 均可见 (隔离) | 员工可查询个人历史工资条；管理员可输入姓名查询某个员工工资条，并负责绩效评分、一键核算全员薪酬与社保配置 |
| **🏢 组织** | 部门树 + 员工列表 + 部门信息 + 岗位薪资标准 + 组织架构图 | 仅管理员可见 | 左侧紧凑部门树，右侧分任务维护员工列表、负责人、岗位职称与薪资范围，并支持可视化架构图拖拽缩放 |
| **📜 日志** | 操作审计日志 | 仅管理员可见 | 系统操作流水日志，包含全操作类型责任追踪 |
| **🔑 权限** | 角色管理 + 权限分配 | 仅管理员可见 | 动态 RBAC 权限配置，支持自定义角色与细粒度权限勾选 |

---

## 📂 项目目录树

```
HRMS/
├── CMakeLists.txt           # CMake 顶层构建脚本
├── config.ini               # 本地数据库连接配置文件 (自动生成，数据库密码使用 Base64 编码遮蔽)
├── style.qss                # 全局样式表 (美化控件视觉)
├── docs/                    # 项目设计与实验文档
│   ├── SRS正式稿.md         # 软件需求规格说明书 (包含 E-R图、DFD图、用例图)
│   ├── SAD正式稿.md         # 软件架构设计说明书 (ISO/IEC/IEEE 42010)
│   ├── 可行性分析报告.md    # 可行性研究报告
│   ├── CASE工具调研.md      # CASE 工具调研材料
│   └── 项目进度与工作跟踪表.md # 项目周进度、成员分工与交付物跟踪
└── src/                     # C++ 源代码
    ├── main.cpp             # 程序入口：初始化数据库连接、检测表结构、调出登录窗口
    ├── core/                # 核心机制与全局状态
    │   ├── Constants.h      # 系统全局常量与枚举定义
    │   ├── GlobalEvents.h/cpp# 全局事件总线 (单例观察者)
    │   ├── Logger.h/cpp     # 文件日志系统
    │   └── SessionManager.h/cpp # 用户登录会话与 RBAC 权限管理
    ├── ui/                  # 通用窗口与对话框
    │   ├── LoginWindow.h/cpp/ui      # 登录界面
    │   ├── MainWindow.h/cpp/ui       # 主框架窗口 (可折叠侧边栏与通知中心)
    │   ├── ChangePasswordDialog.h/cpp# 密码修改对话框
    │   ├── ServerSettingsDialog.h/cpp# 服务器连接设置对话框
    │   └── RecoverPasswordDialog.h/cpp # 自主找回密码向导
    ├── tabs/                # 业务 Tab 页面组件 (10个功能模块)
    │   ├── DashboardTab.h/cpp        # 仪表盘看板 + 高区分度统计图表 + hover明细 + PDF报表导出
    │   ├── EmployeeTab.h/cpp         # 员工信息管理 (CRUD + 分页 + 岗职薪资联动校验)
    │   ├── MyAttendanceTab.h/cpp     # 我的考勤：打卡 + 请假申请 + 补卡申请
    │   ├── AttendManageTab.h/cpp     # 考勤管理：考勤面板 + 审批中心(请假+补卡)
    │   ├── PayrollTab.h/cpp          # 薪酬管理、一键核算、按月份/姓名查询工资条
    │   ├── PerformanceTab.h/cpp      # 绩效评分 (四维度百分制)
    │   ├── OrgTab.h/cpp              # 组织架构：部门树 + 员工列表 + 部门信息 + 岗位薪资标准 + 架构图
    │   ├── AuditTab.h/cpp            # 审计日志查看
    │   ├── ProfileChangeTab.h/cpp    # 信息变更申请与审批
    │   └── RbacTab.h/cpp             # 角色权限管理
    ├── services/            # 业务服务层：封装认证、考勤、审批、薪酬、组织、权限等数据库业务逻辑
    │   ├── AuthService.h/cpp         # 登录、修改密码、自主找回密码
    │   ├── AttendanceService.h/cpp   # 打卡、请假申请、补卡申请
    │   ├── ApprovalService.h/cpp     # 请假/补卡审批
    │   ├── PayrollService.h/cpp      # 月度薪酬核算
    │   ├── PayrollCalculator.h       # 薪酬计算纯函数：计薪月份、绩效奖金、请假扣款、个税等
    │   ├── RbacService.h/cpp         # 角色与权限维护
    │   └── ...                       # 员工、绩效、组织、通知、审计、信息变更服务
    ├── widgets/             # 自定义通用 UI 控件
    │   ├── PaginationBar.h/cpp       # 表格分页栏组件
    │   ├── ComboDelegate.h/cpp       # 表格下拉框委托
    │   ├── SafeEditDelegate.h        # 表格编辑防主题色委托
    │   ├── TaxConfigPanel.h/cpp      # 社保公积金比例配置面板
    │   ├── OrgChartView.h/cpp        # 可视化组织架构图 (Graphics View)
    │   └── OptimisticSqlTableModel.h/cpp # 乐观锁表格模型
    ├── db/                  # 数据库基础设施与自愈迁移
    │   ├── DbConnection.h/cpp        # DSN 构建、密码解码、连接克隆、SQL 错误日志
    │   ├── DbMigration.h/cpp         # 字段补齐、索引创建等幂等迁移辅助
    │   └── DbSchema.h/cpp            # 17 张表初始化、默认数据和迁移编排
    └── utils/               # 工具类
        ├── DbUtils.h/cpp             # 数据库兼容入口，转发到 src/db/ 实现
        ├── DbQuery.h                 # 临时 SQL 查询执行与错误收集辅助
        ├── SqlFilterBuilder.h        # QSqlTableModel 筛选条件构造辅助
        ├── CsvExport.h/cpp           # CSV 导出工具
        ├── UiStyles.h/cpp            # 公共界面样式工具
        ├── MessageHelper.h           # 消息弹窗工具
        ├── UnsavedChangesGuard.h     # 页面切换前统一未保存修改拦截接口
        └── Toast.h                   # Toast 通知提示组件
```

---

## 💾 数据库设计

系统启动时会自动扫描数据库并执行结构自愈迁移（Idempotent Schema Migration），若不存在表则会自动创建。包含以下 **17 张表**：

数据库初始化入口为 `initDatabaseSchema()`，实现位于 `src/db/DbSchema.cpp`；字段补齐、索引创建等可复用迁移辅助位于 `src/db/DbMigration.cpp`；连接克隆与 DSN 构建位于 `src/db/DbConnection.cpp`。旧的 `src/utils/DbUtils.h` 仍保留为兼容入口，避免历史模块大面积调整 include。

1. **`employees`**：员工基础信息与系统账户（含密码 SHA-256 哈希值）。
2. **`departments`**：部门字典表（支持自引用父子层级）。
3. **`roles`**：系统角色定义表。
4. **`permissions`**：原子权限定义表（17 项）。
5. **`role_permissions`**：角色-权限多对多映射表。
6. **`attendances`**：每日考勤打卡记录（迟到/早退/缺卡自动判定）。
7. **`makeup_requests`**：补卡申请记录。
8. **`leave_requests`**：请假申请记录。
9. **`payroll`**：月度工资条（底薪 + 绩效奖金 - 请假扣款 - 五险一金 - 个税 = 实发工资）。
10. **`salary_config`**：五险一金比例配置表（6 项社保项目）。
11. **`system_settings`**：全局计薪天数（21.75天）与免税起征点（5000元）。
12. **`performance_scores`**：员工月度绩效评分（四维度百分制）。
13. **`profile_change_requests`**：员工信息修改审批流表。
14. **`notifications`**：系统未读通知表（铃铛 5 秒轮询）。
15. **`audit_logs`**：全局操作安全审计日志表。
16. **`shifts`**：班次定义表。
17. **`job_salary_standards`**：部门允许的岗位、职称、最低/最高/默认基础薪资标准。

其中 `leave_requests`、`makeup_requests` 与 `profile_change_requests` 均记录审批人、审批时间和审批意见；拒绝审批时要求填写意见，方便申请人理解原因并保留流程痕迹。

---

## 🚀 构建与运行

### 前置条件
1. 安装 **MySQL 8.0+** 服务端并创建一个空数据库 `hrms_db`。
2. 安装 **MySQL ODBC 9.6 UNICODE Driver** (或较新版本)，确保 Windows ODBC 数据源管理器中能查找到该驱动。
3. 安装 Qt6.5.3 (包含 MinGW 编译器)。

### 命令行编译步骤
在项目根目录下打开命令行工具（如 PowerShell/CMD）：

```powershell
# 1. 生成 CMake 构建工程 (请根据本机 Qt 的实际安装路径修改 D:/Qt/...)
cmake -B build -S . -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.5.3/mingw_64" `
  -DCMAKE_CXX_COMPILER="D:/Qt/Tools/mingw1120_64/bin/g++.exe"

# 2. 编译项目
cmake --build build
```

### 运行说明
1. 编译完成后，双击 `./build/HRMS.exe` 启动。
2. Windows 下默认在构建后执行 `windeployqt`，会把 Qt 运行时 DLL 和插件复制到可执行文件目录；若关闭 `HRMS_DEPLOY_QT_RUNTIME` 或旧构建目录仍缺 DLL，再临时补充 `D:\QT\6.5.3\mingw_64\bin` 与 `C:\msys64\ucrt64\bin` 到 `PATH`。
3. 若首次启动或数据库配置错误，登录窗口会显示数据库未连接提示。
4. 点击 **服务器设置**，输入 MySQL 主机 IP、端口、用户名和密码，保存后系统会重新连接并执行数据库自愈初始化。
5. 连接成功后使用默认管理员账户登录；普通员工账户可由管理员在“员工管理”中创建。
