#ifndef REPORTSTAB_H
#define REPORTSTAB_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QComboBox>

class ReportsTab : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsTab(QWidget *parent = nullptr);

private slots:
    void onChartTypeChanged(int index);
    void exportPDF();

private:
    void refreshChart();
    QChartView *m_chartView;
    QComboBox *m_combo;
};

#endif
