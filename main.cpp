#include <QApplication>
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include"loginwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    //使用 QODBC 驱动
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");

    //配置 ODBC 连接字符串
    QString dsn = "DRIVER={MySQL ODBC 9.6 UNICODE Driver};"
                  "SERVER=127.0.0.1;"
                  "PORT=3306;"
                  "DATABASE=hrms_db;"
                  "UID=root;"
                  "PWD=Sunshine2021.;";

    db.setDatabaseName(dsn);


    if (!db.open()) {
        qDebug() << "数据库连接失败！";
        return -1;
    }
    LoginWindow loginWin;
    loginWin.show();

    return app.exec();
}