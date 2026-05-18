#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "changepassworddialog.h"
#include "loginwindow.h"
#include<QMessageBox>
#include<QPushButton>
#include<QMenu>
#include<QComboBox>
#include<QLabel>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QFrame>
#include<QSqlError>
#include<QDate>
#include<QSqlQuery>
#include<QPainter>
#include<QPdfWriter>
#include<QFileDialog>
#include<QTextStream>
#include<QCryptographicHash>

MainWindow::MainWindow(int empId,QString role,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentEmpId(empId)
    , currentRole(role)

{
    ui->setupUi(this);
    this->setWindowTitle("HRMS-员工信息管理");

    // 查询当前用户姓名 + 初始化审计表 + 扩展字段迁移
    {
        QSqlQuery q;
        q.exec("CREATE TABLE IF NOT EXISTS audit_logs ("
               "log_id INT PRIMARY KEY AUTO_INCREMENT,"
               "emp_id INT NOT NULL,"
               "emp_name VARCHAR(50) NOT NULL,"
               "action VARCHAR(100) NOT NULL,"
               "target VARCHAR(200) DEFAULT '',"
               "log_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
        // P4: 部门表初始化
        q.exec("CREATE TABLE IF NOT EXISTS departments ("
               "dept_id INT PRIMARY KEY AUTO_INCREMENT,"
               "dept_name VARCHAR(50) NOT NULL UNIQUE"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
        q.exec("INSERT IGNORE INTO departments (dept_name) "
               "SELECT DISTINCT department FROM employees "
               "WHERE department IS NOT NULL AND department != ''");

        // F11: 扩展字段迁移（检查列是否存在再添加）
        q.exec("SHOW COLUMNS FROM employees LIKE 'education'");
        if (q.size() == 0)
            q.exec("ALTER TABLE employees ADD COLUMN education VARCHAR(20) DEFAULT '' AFTER status");
        q.exec("SHOW COLUMNS FROM employees LIKE 'marital_status'");
        if (q.size() == 0)
            q.exec("ALTER TABLE employees ADD COLUMN marital_status VARCHAR(20) DEFAULT '' AFTER education");
        q.exec("SHOW COLUMNS FROM employees LIKE 'position'");
        if (q.size() == 0)
            q.exec("ALTER TABLE employees ADD COLUMN position VARCHAR(50) DEFAULT '' AFTER marital_status");

        q.prepare("SELECT name FROM employees WHERE emp_id = ?");
        q.addBindValue(currentEmpId);
        if (q.exec() && q.next())
            currentEmpName = q.value("name").toString();
    }
    // 登录日志
    {
        QSqlQuery q;
        q.prepare("INSERT INTO audit_logs (emp_id, emp_name, action, target, log_time) VALUES (?, ?, '用户登录', '', NOW())");
        q.addBindValue(currentEmpId);
        q.addBindValue(currentEmpName);
        q.exec();
    }

    //=====================员工信息管理模块====================
    //连接数据库并指定employees表
    empModel=new QSqlTableModel(this);
    empModel->setTable("employees");
    //设置中文表头
    empModel->setHeaderData(0,Qt::Horizontal,"员工编号");
    empModel->setHeaderData(1,Qt::Horizontal,"姓名");
    empModel->setHeaderData(2,Qt::Horizontal,"性别");
    empModel->setHeaderData(3,Qt::Horizontal,"联系电话");
    empModel->setHeaderData(4,Qt::Horizontal,"所属部门");
    empModel->setHeaderData(5,Qt::Horizontal,"系统角色");
    empModel->setHeaderData(7,Qt::Horizontal,"基础薪资");
    empModel->setHeaderData(8,Qt::Horizontal,"入职日期");
    empModel->setHeaderData(9,Qt::Horizontal,"在职状态");
    empModel->setHeaderData(10,Qt::Horizontal,"学历");
    empModel->setHeaderData(11,Qt::Horizontal,"婚姻状况");
    empModel->setHeaderData(12,Qt::Horizontal,"岗位");
    empModel->select();
    //绑定table views
    ui->tableView_employees->setModel(empModel);
    //隐藏密码那一列
    ui->tableView_employees->hideColumn(6);
    //双击修改
    ui->tableView_employees->setEditTriggers(QAbstractItemView::DoubleClicked);
    //修改存到内存里，手都确认后完成同步
    empModel->setEditStrategy(QSqlTableModel::OnManualSubmit);


    //=================考勤与请假模块====================
    //连接请假表数据库并指定请假表
    leaveModel=new QSqlRelationalTableModel(this);
    leaveModel->setTable("leave_requests");
    //去employees表中将emp_id显示为对应的名字
    leaveModel->setRelation(1,QSqlRelation("employees","emp_id","name"));
    //设置中文表头
    leaveModel->setHeaderData(0, Qt::Horizontal, "请假单号");
    leaveModel->setHeaderData(1, Qt::Horizontal, "申请人");
    leaveModel->setHeaderData(2, Qt::Horizontal, "开始日期");
    leaveModel->setHeaderData(3, Qt::Horizontal, "结束日期");
    leaveModel->setHeaderData(4, Qt::Horizontal, "请假事由");
    leaveModel->setHeaderData(5, Qt::Horizontal, "审批状态");
    //将管家拉取的数据绑定到ui界面的表格上
    ui->tableView_leaves->setModel(leaveModel);
    //设置只读，整行选中
    ui->tableView_leaves->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_leaves->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Tab 2 布局：表格 + 请假表单行 + 审批按钮行
    auto *leaveTabLayout = new QVBoxLayout(ui->widget_2);
    leaveTabLayout->addWidget(ui->tableView_leaves, 1);
    auto *applyRow = new QHBoxLayout;
    applyRow->addWidget(new QLabel("开始日期:"));
    applyRow->addWidget(ui->dateEdit_start);
    applyRow->addWidget(new QLabel("结束日期:"));
    applyRow->addWidget(ui->dateEdit_end);
    applyRow->addWidget(new QLabel("事由:"));
    applyRow->addWidget(ui->lineEdit_reason);
    applyRow->addWidget(ui->btnApplyLeave);
    leaveTabLayout->addLayout(applyRow);
    auto *approveRow = new QHBoxLayout;
    approveRow->addStretch();
    approveRow->addWidget(ui->btnApprove);
    approveRow->addWidget(ui->btnReject);
    leaveTabLayout->addLayout(approveRow);

    //===============薪酬管理模块======================
    payrollModel=new QSqlRelationalTableModel(this);
    payrollModel->setTable("payroll");
    payrollModel->setRelation(1,QSqlRelation("employees","emp_id","name"));
    payrollModel->setHeaderData(0,Qt::Horizontal,"工资条ID");
    payrollModel->setHeaderData(1,Qt::Horizontal,"姓名");
    payrollModel->setHeaderData(2,Qt::Horizontal,"薪资月份");
    payrollModel->setHeaderData(3,Qt::Horizontal,"基础工资");
    payrollModel->setHeaderData(4,Qt::Horizontal,"请假扣款");
    payrollModel->setHeaderData(5,Qt::Horizontal,"实发最终工资");
    payrollModel->setHeaderData(6,Qt::Horizontal,"结算发薪日期");

    ui->tableView_payroll->setModel(payrollModel);
    ui->tableView_payroll->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_payroll->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Tab 3 布局：表格 + 核算按钮
    auto *payrollTabLayout = new QVBoxLayout(ui->tab);
    payrollTabLayout->addWidget(ui->tableView_payroll, 1);
    auto *calcRow = new QHBoxLayout;
    calcRow->addStretch();
    calcRow->addWidget(ui->btnCalculatePayroll);
    auto *btnExportPayroll = new QPushButton("导出CSV");
    calcRow->addWidget(btnExportPayroll);
    calcRow->addStretch();
    payrollTabLayout->addLayout(calcRow);
    connect(btnExportPayroll, &QPushButton::clicked, this, &MainWindow::on_btnExportPayrollCSV_clicked);

    //===============权限隔离=================
    if(currentRole=="user"){
        ui->btnApprove->setVisible(false);
        ui->btnReject->setVisible(false);
        ui->btnCalculatePayroll->setVisible(false);
        ui->tabWidget->setTabVisible(1,false);  // 员工管理
        leaveModel->setFilter(QString("leave_requests.emp_id=%1").arg(currentEmpId));
        payrollModel->setFilter(QString("payroll.emp_id=%1").arg(currentEmpId));
    }

    // 系统菜单
    QMenu *sysMenu = ui->menubar->addMenu("系统");
    QAction *changePwdAction = sysMenu->addAction("修改密码");
    connect(changePwdAction, &QAction::triggered, this, &MainWindow::on_actionChangePassword_triggered);
    sysMenu->addSeparator();
    QAction *logoutAction = sysMenu->addAction("退出登录");
    connect(logoutAction, &QAction::triggered, this, &MainWindow::on_actionLogout_triggered);

    // 状态栏
    QString roleText = (currentRole == "admin") ? "管理员" : "普通员工";
    ui->statusbar->showMessage(QString("当前用户: %1 | 角色: %2").arg(currentEmpName, roleText));

    // 员工状态流转按钮
    QPushButton *btnToggleStatus = new QPushButton("标记离职");
    ui->gridLayout_2->addWidget(btnToggleStatus, 2, 0);
    connect(btnToggleStatus, &QPushButton::clicked, this, &MainWindow::on_btnToggleStatus_clicked);
    connect(ui->tableView_employees->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [btnToggleStatus, this](const QModelIndex &current, const QModelIndex &) {
        if (!current.isValid()) {
            btnToggleStatus->setText("标记离职");
            btnToggleStatus->setEnabled(false);
            return;
        }
        btnToggleStatus->setEnabled(true);
        QString status = empModel->data(empModel->index(current.row(), 9)).toString();
        btnToggleStatus->setText(status == "在职" ? "标记离职" : "标记在职");
    });
    btnToggleStatus->setEnabled(false);

    // 员工筛选栏
    QComboBox *deptCombo = new QComboBox;
    deptCombo->setObjectName("deptCombo");
    deptCombo->addItem("全部部门");
    QComboBox *statusCombo = new QComboBox;
    statusCombo->setObjectName("statusCombo");
    statusCombo->addItems({"全部状态", "在职", "离职"});
    QLineEdit *nameSearch = new QLineEdit;
    nameSearch->setObjectName("nameSearch");
    nameSearch->setPlaceholderText("输入姓名搜索...");
    QPushButton *btnSearch = new QPushButton("查询");
    QPushButton *btnReset = new QPushButton("重置");

    // 填充部门下拉框（从 departments 表读取）
    QSqlQuery deptQuery("SELECT dept_name FROM departments ORDER BY dept_id");
    while (deptQuery.next())
        deptCombo->addItem(deptQuery.value(0).toString());

    // 重组网格布局：筛选栏 → 表格 → 操作按钮 → 状态流转按钮
    QGridLayout *grid = ui->gridLayout_2;
    auto *filterBar = new QHBoxLayout;
    filterBar->addWidget(new QLabel("部门:"));
    filterBar->addWidget(deptCombo);
    filterBar->addWidget(new QLabel("状态:"));
    filterBar->addWidget(statusCombo);
    filterBar->addWidget(new QLabel("姓名:"));
    filterBar->addWidget(nameSearch);
    filterBar->addWidget(btnSearch);
    filterBar->addWidget(btnReset);

    // 提取现有控件
    QTableView *table = ui->tableView_employees;
    QPushButton *addBtn = ui->btnAdd;
    QPushButton *delBtn = ui->btnDelete;
    QPushButton *revBtn = ui->btnRevert;
    QPushButton *saveBtn = ui->btnSave;

    // 清空并重建
    while (grid->count() > 0)
        grid->removeItem(grid->itemAt(0));

    grid->addLayout(filterBar, 0, 0, 1, 4);
    grid->addWidget(table, 1, 0, 1, 4);
    grid->addWidget(addBtn, 2, 0);
    grid->addWidget(delBtn, 2, 1);
    grid->addWidget(revBtn, 2, 2);
    grid->addWidget(saveBtn, 2, 3);
    grid->addWidget(btnToggleStatus, 3, 0);
    auto *btnExportEmp = new QPushButton("导出CSV");
    grid->addWidget(btnExportEmp, 3, 1);
    connect(btnExportEmp, &QPushButton::clicked, this, &MainWindow::on_btnExportEmpCSV_clicked);

    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::on_btnSearch_clicked);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::on_btnResetFilter_clicked);

    //=================操作审计日志模块==================
    auditLogModel = new QSqlTableModel(this);
    auditLogModel->setTable("audit_logs");
    auditLogModel->setHeaderData(0, Qt::Horizontal, "日志编号");
    auditLogModel->setHeaderData(1, Qt::Horizontal, "操作人ID");
    auditLogModel->setHeaderData(2, Qt::Horizontal, "操作人姓名");
    auditLogModel->setHeaderData(3, Qt::Horizontal, "操作动作");
    auditLogModel->setHeaderData(4, Qt::Horizontal, "操作对象");
    auditLogModel->setHeaderData(5, Qt::Horizontal, "操作时间");
    auditLogModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    ui->tableView_audit->setModel(auditLogModel);
    ui->tableView_audit->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_audit->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_audit->hideColumn(1);
    auditLogModel->setSort(5, Qt::DescendingOrder);
    auditLogModel->select();

    // 普通员工隐藏操作日志和统计报表
    if (currentRole == "user") {
        ui->tabWidget->setTabVisible(4, false);  // 操作日志
        ui->tabWidget->setTabVisible(5, false);  // 统计报表
    }

    //=================统计报表模块==================
    chartView = new QChartView;
    chartView->setRenderHint(QPainter::Antialiasing);
    auto *containerLayout = new QVBoxLayout(ui->chartContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(chartView);
    ui->comboChartType->addItems({"部门人数分布", "在职/离职比例", "各部门平均薪资", "月度请假统计"});
    connect(ui->comboChartType, &QComboBox::currentIndexChanged,
            this, &MainWindow::on_comboChartType_currentIndexChanged);
    connect(ui->btnExportPDF, &QPushButton::clicked,
            this, &MainWindow::on_btnExportPDF_clicked);
    refreshChart();

    //===============仪表盘首页==================
    auto *dashWidget = new QWidget;
    auto *dashGrid = new QGridLayout(dashWidget);
    dashGrid->setContentsMargins(20, 20, 20, 20);
    dashGrid->setSpacing(15);

    struct Card { QFrame *frame; QLabel *value; };
    Card cards[6];
    QString titles[] = {"总员工数", "在职人数", "离职人数", "本月请假", "待审批", "本月薪资总额"};
    for (int i = 0; i < 6; i++) {
        auto *frame = new QFrame;
        frame->setFrameStyle(QFrame::Box | QFrame::Raised);
        frame->setMinimumSize(200, 100);
        frame->setStyleSheet("QFrame { background: #fff; border: 1px solid #ddd; border-radius: 6px; }");
        auto *layout = new QVBoxLayout(frame);
        auto *title = new QLabel(titles[i]);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 13px; color: #666; border: none;");
        auto *value = new QLabel("-");
        value->setAlignment(Qt::AlignCenter);
        value->setStyleSheet("font-size: 28px; font-weight: bold; color: #333; border: none;");
        value->setObjectName("dash_" + QString::number(i));
        layout->addWidget(title);
        layout->addWidget(value);
        dashGrid->addWidget(frame, i / 3, i % 3);
        cards[i] = {frame, value};
    }

    ui->tabWidget->insertTab(0, dashWidget, "首页");
    if (currentRole == "user")
        ui->tabWidget->setTabVisible(0, false);

    // 状态栏随 Tab 切换动态更新 + 仪表盘刷新
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 0) refreshDashboard();
        QString tabName = ui->tabWidget->tabText(idx);
        QString info;
        if (idx == 1)
            info = QString(" | 共 %1 条记录").arg(empModel->rowCount());
        else if (idx == 2)
            info = QString(" | 共 %1 条记录").arg(leaveModel->rowCount());
        else if (idx == 3)
            info = QString(" | 共 %1 条记录").arg(payrollModel->rowCount());
        else if (idx == 4)
            info = QString(" | 共 %1 条记录").arg(auditLogModel->rowCount());
        ui->statusbar->showMessage(QString("当前用户: %1 | %2 | %3")
            .arg(currentEmpName, (currentRole == "admin") ? "管理员" : "普通员工", tabName + info));
    });
    refreshDashboard();

    payrollModel->select();
    leaveModel->select();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnAdd_clicked()
{
    //获取当前行数，在最后一行插入空白行，并让表格自动滚动选中刚添加的空白行
    int rowCount=empModel->rowCount();
    empModel->insertRow(rowCount);
    ui->tableView_employees->setCurrentIndex(empModel->index(rowCount,1));
    logAction("新增员工记录");
}


void MainWindow::on_btnDelete_clicked()
{
    //获取当前鼠标选中的那一行
    int currentRow=ui->tableView_employees->currentIndex().row();
    if(currentRow>=0){
        QString name = empModel->data(empModel->index(currentRow, 1)).toString();
        empModel->removeRow(currentRow);
        logAction("删除员工记录", name);
    }else{
        QMessageBox::warning(this,"提示","请先在表格中选中要删除的员工！");
    }
}


void MainWindow::on_btnSave_clicked()
{
    //将所有修改一次性提交给MySQL
    if(empModel->submitAll()){
        logAction("保存员工信息修改");
        QMessageBox::information(this,"成功","所有数据修改已成功");
    }else{
        QMessageBox::critical(this,"失败","保存失败: \n"+empModel->lastError().text());
    }
}


void MainWindow::on_btnRevert_clicked()
{
    //放弃所有为保存的修改
    empModel->revertAll();
}


void MainWindow::on_btnApplyLeave_clicked()
{
    //读取输入的数据
    QDate startDate=ui->dateEdit_start->date();
    QDate endDate=ui->dateEdit_end->date();
    QString reason=ui->lineEdit_reason->text();
    //基础逻辑校验
    if(startDate>endDate){
        QMessageBox::warning(this,"填写错误","结束日期不能早于开始日期，请重新选择");
        return;
    }
    if(reason.isEmpty()){
        QMessageBox::warning(this,"填写错误","请假理由不能为空");
        return;
    }
    //准备sql插入语句
    QSqlQuery query;
    query.prepare("INSERT INTO leave_requests(emp_id,start_date,end_date,reason,status) "
                  "VALUES(:emp_id,:start_date,:end_date,:reason,'待审批')");
    query.bindValue(":emp_id",currentEmpId);
    query.bindValue(":start_date",startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end_date",endDate.toString("yyyy-MM-dd"));
    query.bindValue(":reason",reason);
    //执行插入操作
    if(query.exec()){
        QMessageBox::information(this,"成功","你的请假申请已成功提交，请等待管理员审批");
        logAction("提交请假申请", startDate.toString("MM-dd") + " ~ " + endDate.toString("MM-dd"));
        //刷新表格
        leaveModel->select();
        //清空输入框
        ui->lineEdit_reason->clear();
    }else{
        QMessageBox::critical(this,"数据库错误","提交失败: \n"+query.lastError().text());
    }
}


void MainWindow::on_btnApprove_clicked()
{
    //获取鼠标选中的哪一行
    int currentRow=ui->tableView_leaves->currentIndex().row();
    if(currentRow<0){
        QMessageBox::warning(this,"提示","请选中请假单");
        return;
    }
    int requestID=leaveModel->data(leaveModel->index(currentRow,0)).toInt();
    QSqlQuery query;
    query.prepare("UPDATE leave_requests SET status = '已同意' WHERE request_id=?");
    query.addBindValue(requestID);
    if(query.exec()){
        QMessageBox::information(this,"审批成功","已成功批改请假申请");
        logAction("同意请假", "请假单号: " + QString::number(requestID));
        leaveModel->select();
    }else{
        QMessageBox::critical(this,"审批失败","审批失败: \n"+query.lastError().text());
    }
}


void MainWindow::on_btnReject_clicked()
{
    //获取鼠标选中的哪一行
    int currentRow=ui->tableView_leaves->currentIndex().row();
    if(currentRow<0){
        QMessageBox::warning(this,"提示","请选中请假单");
        return;
    }
    int requestID=leaveModel->data(leaveModel->index(currentRow,0)).toInt();
    QSqlQuery query;
    query.prepare("UPDATE leave_requests SET status = '已拒绝' WHERE request_id=?");
    query.addBindValue(requestID);
    if(query.exec()){
        QMessageBox::information(this,"审批成功","已拒绝请假申请");
        logAction("拒绝请假", "请假单号: " + QString::number(requestID));
        leaveModel->select();
    }else{
        QMessageBox::critical(this,"审批失败","审批失败: \n"+query.lastError().text());
    }
}


void MainWindow::on_btnCalculatePayroll_clicked()
{
    //  获取当前系统月份
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    // 检查本月是否已经核算过工资了
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM payroll WHERE month = ?");
    checkQuery.addBindValue(currentMonth);
    checkQuery.exec();

    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        // 如果查出本月记录大于0，弹窗警告
        int ret = QMessageBox::question(this, "重新核算警告",
                                        currentMonth + " 月份的工资已经核算过了！\n再次核算将覆盖原有数据，确定要继续吗？",
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) return; // 老板说算了，那就直接退出函数

        // 老板非要重算，那就把本月旧数据全部删掉
        QSqlQuery deleteQuery;
        deleteQuery.prepare("DELETE FROM payroll WHERE month = ?");
        deleteQuery.addBindValue(currentMonth);
        deleteQuery.exec();
    }

    //开始遍历所有员工的id和基础工资
    QSqlQuery empQuery("SELECT emp_id, base_salary FROM employees");
    int successCount = 0;

    // 开启循环：遍历每一个员工
    while (empQuery.next()) {
        int empId = empQuery.value("emp_id").toInt();
        double baseSalary = empQuery.value("base_salary").toDouble();

        // 统计该员工本月已同意的请假总天数
        QSqlQuery leaveQuery;
        leaveQuery.prepare("SELECT SUM(DATEDIFF(end_date, start_date) + 1) FROM leave_requests "
                           "WHERE emp_id = ? AND status = '已同意' AND start_date LIKE ?");
        leaveQuery.addBindValue(empId);
        leaveQuery.addBindValue(currentMonth + "%"); // 巧妙利用 LIKE 匹配本月开头的日期
        leaveQuery.exec();

        int leaveDays = 0;
        if (leaveQuery.next()) {
            leaveDays = leaveQuery.value(0).toInt(); // 取出请假总天数
        }

        //计算扣款与最终工资
        double deduction = (baseSalary / 21.75) * leaveDays;
        double netSalary = baseSalary - deduction;

        //将生成的工资单插入薪资表
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO payroll (emp_id, month, base_salary, leave_deduction, net_salary, issue_date) "
                            "VALUES (?, ?, ?, ?, ?, ?)");
        insertQuery.addBindValue(empId);
        insertQuery.addBindValue(currentMonth);
        insertQuery.addBindValue(baseSalary);
        insertQuery.addBindValue(deduction);
        insertQuery.addBindValue(netSalary);
        insertQuery.addBindValue(today);

        if (insertQuery.exec()) {
            successCount++;
        }
    }

    QMessageBox::information(this, "核算完成", QString("一键核算完毕！\n成功生成了 %1 名员工的 %2 月份工资单。").arg(successCount).arg(currentMonth));
    logAction("核算工资", currentMonth + " (" + QString::number(successCount) + "人)");
    payrollModel->select();
}

void MainWindow::on_actionChangePassword_triggered()
{
    ChangePasswordDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    // 验证旧密码
    QSqlQuery query;
    query.prepare("SELECT password_hash FROM employees WHERE emp_id = ?");
    query.addBindValue(currentEmpId);
    query.exec();
    if (query.next()) {
        QString oldHash = QString(QCryptographicHash::hash(dlg.oldPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
        if (query.value("password_hash").toString() != oldHash) {
            QMessageBox::warning(this, "修改失败", "旧密码错误！");
            return;
        }
    }

    // 更新密码（SHA-256 哈希存储）
    QString newHash = QString(QCryptographicHash::hash(dlg.newPassword().toUtf8(), QCryptographicHash::Sha256).toHex());
    query.prepare("UPDATE employees SET password_hash = ? WHERE emp_id = ?");
    query.addBindValue(newHash);
    query.addBindValue(currentEmpId);
    if (query.exec()) {
        QMessageBox::information(this, "成功", "密码修改成功！");
        logAction("修改密码");
    } else {
        QMessageBox::critical(this, "失败", "密码更新失败: " + query.lastError().text());
    }
}

void MainWindow::on_btnToggleStatus_clicked()
{
    int row = ui->tableView_employees->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中员工！");
        return;
    }
    QString currentStatus = empModel->data(empModel->index(row, 9)).toString();
    QString newStatus = (currentStatus == "在职") ? "离职" : "在职";
    empModel->setData(empModel->index(row, 9), newStatus);
    QString name = empModel->data(empModel->index(row, 1)).toString();
    logAction("变更员工状态", name + " → " + newStatus);
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (btn)
        btn->setText(newStatus == "在职" ? "标记离职" : "标记在职");
}

void MainWindow::on_btnSearch_clicked()
{
    auto *deptCombo = findChild<QComboBox *>("deptCombo");
    auto *statusCombo = findChild<QComboBox *>("statusCombo");
    auto *nameSearch = findChild<QLineEdit *>("nameSearch");

    QStringList conditions;
    if (deptCombo && deptCombo->currentIndex() > 0)
        conditions << QString("department='%1'").arg(deptCombo->currentText());
    if (statusCombo && statusCombo->currentIndex() > 0)
        conditions << QString("status='%1'").arg(statusCombo->currentText());
    if (nameSearch && !nameSearch->text().isEmpty())
        conditions << QString("name LIKE '%%1%'").arg(nameSearch->text());

    empModel->setFilter(conditions.isEmpty() ? "" : conditions.join(" AND "));
    empModel->select();
}

void MainWindow::on_btnResetFilter_clicked()
{
    auto *deptCombo = findChild<QComboBox *>("deptCombo");
    auto *statusCombo = findChild<QComboBox *>("statusCombo");
    auto *nameSearch = findChild<QLineEdit *>("nameSearch");

    if (deptCombo) deptCombo->setCurrentIndex(0);
    if (statusCombo) statusCombo->setCurrentIndex(0);
    if (nameSearch) nameSearch->clear();

    empModel->setFilter("");
    empModel->select();
}

void MainWindow::on_actionLogout_triggered()
{
    logAction("退出登录");
    auto *loginWin = new LoginWindow;
    loginWin->show();
    this->close();
}

void MainWindow::logAction(const QString &action, const QString &target)
{
    QSqlQuery query;
    query.prepare("INSERT INTO audit_logs (emp_id, emp_name, action, target, log_time) "
                  "VALUES (?, ?, ?, ?, NOW())");
    query.addBindValue(currentEmpId);
    query.addBindValue(currentEmpName);
    query.addBindValue(action);
    query.addBindValue(target);
    query.exec();
}

void MainWindow::on_comboChartType_currentIndexChanged(int)
{
    refreshChart();
}

void MainWindow::refreshChart()
{
    int type = ui->comboChartType->currentIndex();
    auto *oldChart = chartView->chart();
    chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QSqlQuery q;
    switch (type) {
    case 0: { // 部门人数分布
        q.exec("SELECT department, COUNT(*) FROM employees GROUP BY department");
        auto *series = new QPieSeries;
        while (q.next()) {
            QString dept = q.value(0).toString();
            if (dept.isEmpty()) dept = "未分配";
            series->append(dept, q.value(1).toInt());
        }
        chart->addSeries(series);
        chart->setTitle("部门人数分布");
        break;
    }
    case 1: { // 在职/离职比例
        q.exec("SELECT status, COUNT(*) FROM employees GROUP BY status");
        auto *series = new QPieSeries;
        while (q.next())
            series->append(q.value(0).toString(), q.value(1).toInt());
        chart->addSeries(series);
        chart->setTitle("在职/离职比例");
        break;
    }
    case 2: { // 各部门平均薪资
        q.exec("SELECT department, AVG(base_salary) FROM employees WHERE base_salary > 0 GROUP BY department");
        auto *set = new QBarSet("平均薪资");
        QStringList cats;
        while (q.next()) {
            cats << q.value(0).toString();
            *set << q.value(1).toDouble();
        }
        auto *series = new QBarSeries;
        series->append(set);
        chart->addSeries(series);
        auto *axisX = new QBarCategoryAxis;
        axisX->append(cats);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto *axisY = new QValueAxis;
        axisY->setTitleText("元");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        chart->setTitle("各部门平均薪资");
        break;
    }
    case 3: { // 月度请假统计
        q.exec("SELECT DATE_FORMAT(start_date, '%Y-%m'), COUNT(*) FROM leave_requests GROUP BY DATE_FORMAT(start_date, '%Y-%m') ORDER BY 1");
        auto *set = new QBarSet("请假人次");
        QStringList cats;
        while (q.next()) {
            cats << q.value(0).toString();
            *set << q.value(1).toInt();
        }
        auto *series = new QBarSeries;
        series->append(set);
        chart->addSeries(series);
        auto *axisX = new QBarCategoryAxis;
        axisX->append(cats);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto *axisY = new QValueAxis;
        axisY->setTitleText("人次");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        chart->setTitle("月度请假统计");
        break;
    }
    }
    chartView->setChart(chart);
    if (oldChart)
        delete oldChart;
}

void MainWindow::on_btnExportPDF_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出PDF", "报表.pdf", "PDF文件 (*.pdf)");
    if (filePath.isEmpty())
        return;

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(300);

    QPainter painter(&writer);
    chartView->render(&painter);
    painter.end();

    QMessageBox::information(this, "导出成功", "报表已成功导出至:\n" + filePath);
}

void MainWindow::on_btnExportEmpCSV_clicked()
{
    QString path = QFileDialog::getSaveFileName(this, "导出员工表", "员工信息.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF"; // BOM for Excel
    // 表头
    for (int c = 0; c < empModel->columnCount(); c++) {
        if (c == 6) continue; // 跳过密码列
        out << "\"" << empModel->headerData(c, Qt::Horizontal).toString() << "\"";
        if (c < empModel->columnCount() - 1) out << ",";
    }
    out << "\n";
    // 数据行
    for (int r = 0; r < empModel->rowCount(); r++) {
        for (int c = 0; c < empModel->columnCount(); c++) {
            if (c == 6) continue;
            out << "\"" << empModel->data(empModel->index(r, c)).toString().replace("\"", "\"\"") << "\"";
            if (c < empModel->columnCount() - 1) out << ",";
        }
        out << "\n";
    }
    file.close();
    QMessageBox::information(this, "导出成功", "员工信息已导出至:\n" + path);
}

void MainWindow::refreshDashboard()
{
    QSqlQuery q;
    // 总员工数
    q.exec("SELECT COUNT(*) FROM employees");
    int total = q.next() ? q.value(0).toInt() : 0;
    // 在职/离职
    q.exec("SELECT COUNT(*) FROM employees WHERE status='在职'");
    int active = q.next() ? q.value(0).toInt() : 0;
    q.exec("SELECT COUNT(*) FROM employees WHERE status='离职'");
    int inactive = q.next() ? q.value(0).toInt() : 0;
    // 本月请假
    QString month = QDate::currentDate().toString("yyyy-MM");
    q.prepare("SELECT COUNT(*) FROM leave_requests WHERE start_date LIKE ?");
    q.addBindValue(month + "%");
    q.exec();
    int leaves = q.next() ? q.value(0).toInt() : 0;
    // 待审批
    q.exec("SELECT COUNT(*) FROM leave_requests WHERE status='待审批'");
    int pending = q.next() ? q.value(0).toInt() : 0;
    // 本月薪资总额
    q.prepare("SELECT SUM(net_salary) FROM payroll WHERE month = ?");
    q.addBindValue(month);
    q.exec();
    double salary = q.next() ? q.value(0).toDouble() : 0;

    for (int i = 0; i < 6; i++) {
        auto *label = findChild<QLabel *>("dash_" + QString::number(i));
        if (!label) continue;
        switch (i) {
        case 0: label->setText(QString::number(total)); break;
        case 1: label->setText(QString::number(active)); break;
        case 2: label->setText(QString::number(inactive)); break;
        case 3: label->setText(QString::number(leaves)); break;
        case 4: label->setText(QString::number(pending)); break;
        case 5: label->setText(QString::number(salary, 'f', 2) + " 元"); break;
        }
    }
}

void MainWindow::on_btnExportPayrollCSV_clicked()
{
    QString path = QFileDialog::getSaveFileName(this, "导出工资表", "薪酬数据.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";
    for (int c = 0; c < payrollModel->columnCount(); c++) {
        out << "\"" << payrollModel->headerData(c, Qt::Horizontal).toString() << "\"";
        if (c < payrollModel->columnCount() - 1) out << ",";
    }
    out << "\n";
    for (int r = 0; r < payrollModel->rowCount(); r++) {
        for (int c = 0; c < payrollModel->columnCount(); c++) {
            out << "\"" << payrollModel->data(payrollModel->index(r, c)).toString().replace("\"", "\"\"") << "\"";
            if (c < payrollModel->columnCount() - 1) out << ",";
        }
        out << "\n";
    }
    file.close();
    QMessageBox::information(this, "导出成功", "薪酬数据已导出至:\n" + path);
}

