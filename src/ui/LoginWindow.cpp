#include "LoginWindow.h"
#include "ui_LoginWindow.h"
#include "MainWindow.h"
#include "ServerSettingsDialog.h"
#include <QCryptographicHash>
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
    QString driver   = settings.value("Database/Driver",   "MySQL ODBC 9.6 UNICODE Driver").toString();
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
    // SHA-256 哈希密码
    QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    QSqlQuery query;
    query.prepare("SELECT emp_id,name,role FROM employees WHERE (name=:account OR phone=:account) AND password_hash=:password");
    query.bindValue(":account",account);
    query.bindValue(":password",passwordHash);
    if(!query.exec()){
        QMessageBox::critical(this,"数据库错误",query.lastError().text());
        return;
    }
    if(!query.next()){
        QMessageBox::critical(this,"登录失败","账号或密码错误，请重试！");
        return;
    }
    int empId=query.value("emp_id").toInt();
    QString empName=query.value("name").toString();
    QString role=query.value("role").toString();
    QMessageBox::information(this,"登录成功","欢迎回来"+empName+"!\n您的权限级别是:"+role);
    MainWindow *mainWin=new MainWindow(empId,role);
    mainWin->setAttribute(Qt::WA_DeleteOnClose);
    mainWin->show();
    this->close();
}