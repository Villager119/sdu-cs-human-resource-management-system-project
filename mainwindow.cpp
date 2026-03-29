#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QMessageBox>
#include<QSqlError>
#include<QDate>
#include<QSqlQuery>

MainWindow::MainWindow(int empId,QString role,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentEmpId(empId)
    , currentRole(role)

{
    ui->setupUi(this);
    this->setWindowTitle("HRMS-员工信息管理");
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

    empModel->select();
    //绑定table views
    ui->tableView_employees->setModel(empModel);
    //隐藏密码那一列
    ui->tableView_employees->hideColumn(6);
    //双击修改
    ui->tableView_employees->setEditTriggers(QAbstractItemView::DoubleClicked);
    //修改存到内存里，手都确认后完成同步
    empModel->setEditStrategy(QSqlTableModel::OnManualSubmit);


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
    leaveModel->select();
    //将管家拉取的数据绑定到ui界面的表格上
    ui->tableView_leaves->setModel(leaveModel);
    //设置只读，整行选中
    ui->tableView_leaves->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_leaves->setSelectionBehavior(QAbstractItemView::SelectRows);


    //===============权限隔离=================
    if(currentRole=="user"){
        ui->btnApprove->setVisible(false);
        ui->btnReject->setVisible(false);
        ui->tabWidget->setTabVisible(0,false);
    }
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
}


void MainWindow::on_btnDelete_clicked()
{
    //获取当前鼠标选中的那一行
    int currentRow=ui->tableView_employees->currentIndex().row();
    if(currentRow>=0){
        //界面上删除该行，但数据库里还在
        empModel->removeRow(currentRow);
    }else{
        QMessageBox::warning(this,"提示","请先在表格中选中要删除的员工！");
    }
}


void MainWindow::on_binSave_clicked()
{
    //将所有修改一次性提交给MySQL
    if(empModel->submitAll()){
        QMessageBox::critical(this,"成功","所有数据修改已成功");
    }else{
        QMessageBox::critical(this,"失败","保存失败: \n"+empModel->lastError().text());
    }
}


void MainWindow::on_binRevert_clicked()
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
        QMessageBox::warning(this,"填写错误","结束日期大于开始日期，何意味");
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
    }
    //获取请假单号
    int requestID=leaveModel->data(leaveModel->index(currentRow,0)).toInt();
    //执行SQL语句
    QSqlQuery query;
    query.prepare("UPDATE leave_requests SET status = '已同意' WHERE request_id=?");
    query.addBindValue(requestID);
    if(query.exec()){
        QMessageBox::information(this,"审批成功","已成功批改请假申请");
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
    }
    //获取请假单号
    int requestID=leaveModel->data(leaveModel->index(currentRow,0)).toInt();
    //执行SQL语句
    QSqlQuery query;
    query.prepare("UPDATE leave_requests SET status = '已拒绝' WHERE request_id=?");
    query.addBindValue(requestID);
    if(query.exec()){
        QMessageBox::information(this,"审批成功","已拒绝请假申请");
        leaveModel->select();
    }else{
        QMessageBox::critical(this,"审批失败","审批失败: \n"+query.lastError().text());
    }
}

