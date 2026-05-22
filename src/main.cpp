#include <QApplication>
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include "ui/LoginWindow.h"
#include "utils/DbUtils.h"
#include "core/Logger.h"

static QString findFile(const QString &name)
{
    QString exeDir = QApplication::applicationDirPath();
    QStringList paths = {
        exeDir + "/" + name,
        exeDir + "/../../" + name,
        exeDir + "/../" + name,
        name
    };
    for (const auto &p : paths)
        if (QFileInfo::exists(p)) return p;
    return paths.first();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 初始化日志
    Logger::init(QApplication::applicationDirPath() + "/hrms.log");
    LOG_INFO("HRMS 启动");

    // 加载全局样式表
    QString qssPath = findFile("style.qss");
    QFile qssFile(qssPath);
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QTextStream(&qssFile).readAll());
        qssFile.close();
    }
    // 从 config.ini 读取数据库连接配置
    QString configPath = findFile("config.ini");
    QSettings settings(configPath, QSettings::IniFormat);

    QString driver = settings.value("Database/Driver", "MySQL ODBC 9.6 UNICODE Driver").toString();
    QString server = settings.value("Database/Server", "127.0.0.1").toString();
    int port = settings.value("Database/Port", 3306).toInt();
    QString database = settings.value("Database/Database", "hrms_db").toString();
    QString uid = settings.value("Database/UID", "root").toString();
    // Base64 解码密码（兼容明文：解码失败则使用原值）
    QString pwdEnc = settings.value("Database/PWD", "").toString();
    QByteArray pwdDec = QByteArray::fromBase64(pwdEnc.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    QString pwd = pwdDec.isEmpty() ? pwdEnc : QString::fromUtf8(pwdDec);

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName(buildDsn(driver, server, port, database, uid, pwd));

    bool dbOk = db.open();
    if (!dbOk)
        qDebug() << "数据库连接失败！" << db.lastError().text();

    LoginWindow loginWin;
    loginWin.setConfigPath(configPath);
    loginWin.setDbConnected(dbOk);
    loginWin.show();

    return app.exec();
}