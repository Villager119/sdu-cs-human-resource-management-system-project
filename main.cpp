#include <QApplication>
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include"loginwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 从 config.ini 读取数据库连接配置（多路径查找）
    QString exeDir = QApplication::applicationDirPath();
    QStringList searchPaths = {
        exeDir + "/config.ini",            // exe 同目录（部署）
        exeDir + "/../../config.ini",      // build/xxx/ 上两级到项目根（Qt Creator）
        exeDir + "/../config.ini",         // build/ 上一级
        "config.ini"                       // 当前工作目录
    };
    QString configPath;
    for (const auto &path : searchPaths) {
        if (QFileInfo::exists(path)) {
            configPath = path;
            break;
        }
    }
    if (configPath.isEmpty())
        configPath = searchPaths.first();

    QSettings settings(configPath, QSettings::IniFormat);

    QString driver = settings.value("Database/Driver", "MySQL ODBC 9.6 UNICODE Driver").toString();
    QString server = settings.value("Database/Server", "127.0.0.1").toString();
    int port = settings.value("Database/Port", 3306).toInt();
    QString database = settings.value("Database/Database", "hrms_db").toString();
    QString uid = settings.value("Database/UID", "root").toString();
    QString pwd = settings.value("Database/PWD", "").toString();

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");

    QString dsn = QString("DRIVER={%1};"
                          "SERVER=%2;"
                          "PORT=%3;"
                          "DATABASE=%4;"
                          "UID=%5;"
                          "PWD=%6;")
                      .arg(driver, server)
                      .arg(port)
                      .arg(database, uid, pwd);

    db.setDatabaseName(dsn);

    if (!db.open()) {
        qDebug() << "数据库连接失败！" << db.lastError().text();
        return -1;
    }
    LoginWindow loginWin;
    loginWin.show();

    return app.exec();
}