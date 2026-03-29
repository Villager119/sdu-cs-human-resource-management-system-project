#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QSqlTableModel>
#include<QSqlRelationalTableModel>
#include<QSqlRelation>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int empId,QString role,QWidget *parent=nullptr);
    ~MainWindow();

private slots:
    void on_btnAdd_clicked();

    void on_btnDelete_clicked();

    void on_binSave_clicked();

    void on_binRevert_clicked();

    void on_btnApplyLeave_clicked();

    void on_btnApprove_clicked();

    void on_btnReject_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *empModel;
    QSqlRelationalTableModel *leaveModel;
    int currentEmpId;
    QString currentRole;
};

#endif // MAINWINDOW_H
