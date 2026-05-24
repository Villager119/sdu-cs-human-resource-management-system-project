#ifndef DASHBOARDTAB_H
#define DASHBOARDTAB_H

#include <QWidget>
#include <QLabel>

#include <QtCharts/QChartView>

class DashboardTab : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardTab(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    QLabel *m_labels[6];
    QLabel *m_titleLabels[6];
    QLabel *m_iconLabels[6];
    QLabel *m_alertLabel;

    QFrame *m_profileFrame;
    QLabel *m_welcomeLabel;
    QLabel *m_infoEmpId;
    QLabel *m_infoDept;
    QLabel *m_infoPos;
    QLabel *m_infoPhone;
    QLabel *m_infoHireDate;
    QLabel *m_infoEdu;

    QChartView *m_chartView;
    class QComboBox *m_chartCombo;
    class QPushButton *m_exportPdfBtn;
    bool m_isAdmin = false;

private slots:
    void onChartTypeChanged(int index);
    void exportPDF();

private:
    void refreshChart();
};

#endif
