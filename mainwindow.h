#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QSqlTableModel>
#include<QSqlRelationalTableModel>
#include<QSqlRelation>
#include<QtCharts/QChartView>
#include<QtCharts/QPieSeries>
#include<QtCharts/QBarSeries>
#include<QtCharts/QBarSet>
#include<QtCharts/QBarCategoryAxis>
#include<QtCharts/QValueAxis>

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

    void on_btnCalculatePayroll_clicked();

    void on_actionChangePassword_triggered();

    void on_btnToggleStatus_clicked();

    void on_btnSearch_clicked();
    void on_btnResetFilter_clicked();

    void on_actionLogout_triggered();

    void on_comboChartType_currentIndexChanged(int index);
    void on_btnExportPDF_clicked();

private:
    void logAction(const QString &action, const QString &target = QString());
    void refreshChart();

    Ui::MainWindow *ui;
    QSqlTableModel *empModel;
    QSqlRelationalTableModel *leaveModel;
    QSqlRelationalTableModel *payrollModel;
    QSqlTableModel *auditLogModel;
    QChartView *chartView;
    QChart *chart;
    int currentEmpId;
    QString currentRole;
    QString currentEmpName;
};

#endif // MAINWINDOW_H
