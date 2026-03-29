#include "loginwindow.h"
#include "ui_loginwindow.h"
#include"mainwindow.h"
#include<QDebug>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("HRMS - 系统登录");
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::on_btnLogin_clicked()
{
    //获取用户输入的账号和密码
    QString account=ui->lineEdit_account->text();
    QString password=ui->lineEdit_password->text();

    if(account.isEmpty()||password.isEmpty()){
        QMessageBox::warning(this,"提示","账号或密码不能为空！");
        return;
    }
    //员工使用姓名或者手机号登录
    QSqlQuery query;
    query.prepare("SELECT emp_id,name,role FROM employees WHERE (name=:account OR phone = :account)AND password_hash= :password");
    //安全绑定参数，防止SQL注入
    query.bindValue(":account",account);
    query.bindValue(":password",password);
    //执行查询
    if(!query.exec()){
        QMessageBox::critical(this,"数据库错误",query.lastError().text());
        return;
    }
    //判断是否有结果
    if(query.next()){
        //查到了匹配到的账号密码
        int empId=query.value("emp_id").toInt();
        QString empName=query.value("name").toString();
        QString role=query.value("role").toString();
        QMessageBox::information(this,"登录成功","欢迎回来"+empName+"!\n您的权限级别是:"+role);
        //跳转到主界面
        MainWindow *mainWin=new MainWindow(empId,role);
        //当主界面关闭时，自动释放它的内存
        mainWin->setAttribute(Qt::WA_DeleteOnClose);
        mainWin->show();
        this->close();
    }else{
        //没查到数据，账号或者密码错误
        QMessageBox::critical(this,"登录失败","账号或密码错误，请重试！");
    }
}