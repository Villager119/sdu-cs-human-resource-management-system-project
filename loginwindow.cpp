#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "mainwindow.h"
#include "serversettingsdialog.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSettings>
#include <QFileInfo>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("HRMS - 系统登录");
    ui->label_status->setVisible(false);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::setConfigPath(const QString &path)
{
    m_configPath = path;
}

void LoginWindow::setDbConnected(bool ok)
{
    m_dbConnected = ok;
    if (!ok) {
        ui->label_status->setText("数据库未连接 - 请点击\"服务器设置\"配置连接");
        ui->label_status->setStyleSheet("color: red;");
        ui->label_status->setVisible(true);
    }
}

void LoginWindow::on_btnServerSettings_clicked()
{
    ServerSettingsDialog dlg(m_configPath, this);
    if (dlg.exec() == QDialog::Accepted)
        tryReconnect();
}

void LoginWindow::tryReconnect()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    QString driver   = settings.value("Database/Driver",   "MySQL ODBC 9.2 Unicode Driver").toString();
    QString server   = settings.value("Database/Server",   "127.0.0.1").toString();
    int     port     = settings.value("Database/Port",     3306).toInt();
    QString database = settings.value("Database/Database", "hrms_db").toString();
    QString uid      = settings.value("Database/UID",      "root").toString();
    QString pwd      = settings.value("Database/PWD",      "").toString();

    QSqlDatabase db = QSqlDatabase::database();
    db.close();

    QString dsn = QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
                      .arg(driver, server)
                      .arg(port)
                      .arg(database, uid, pwd);
    db.setDatabaseName(dsn);

    if (db.open()) {
        m_dbConnected = true;
        ui->label_status->setVisible(false);
        QMessageBox::information(this, "提示", "数据库连接成功！");
    } else {
        m_dbConnected = false;
        ui->label_status->setText("数据库连接失败: " + db.lastError().text());
        ui->label_status->setStyleSheet("color: red;");
        ui->label_status->setVisible(true);
    }
}

void LoginWindow::on_btnLogin_clicked()
{
    if (!m_dbConnected) {
        QMessageBox::warning(this, "提示", "数据库未连接，请先配置服务器！");
        return;
    }

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