#ifndef REPORTSTAB_H
#define REPORTSTAB_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QComboBox>

class ReportsTab : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsTab(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onChartTypeChanged(int index);
    void exportPDF();

private:
    void refreshChart();
    QChartView *m_chartView;
    QComboBox *m_combo;
};

#endif
