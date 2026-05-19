#include "ReportsTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QMessageBox>
#include <QSqlQuery>
#include <QChart>

ReportsTab::ReportsTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_chartView, 1);

    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel("图表类型:"));
    m_combo = new QComboBox;
    m_combo->addItems({"部门人数分布", "在职/离职比例", "各部门平均薪资", "月度请假统计"});
    row->addWidget(m_combo);
    row->addStretch();
    auto *btn = new QPushButton("导出PDF");
    row->addWidget(btn);
    layout->addLayout(row);

    connect(m_combo, &QComboBox::currentIndexChanged, this, &ReportsTab::onChartTypeChanged);
    connect(btn, &QPushButton::clicked, this, &ReportsTab::exportPDF);
    refreshChart();
}

void ReportsTab::onChartTypeChanged(int) { refreshChart(); }

void ReportsTab::refreshChart()
{
    int type = m_combo->currentIndex();
    auto *old = m_chartView->chart();
    auto *chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);
    QSqlQuery q;

    switch (type) {
    case 0: {
        q.exec("SELECT department, COUNT(*) FROM employees GROUP BY department");
        auto *s = new QPieSeries;
        while (q.next()) {
            QString d = q.value(0).toString();
            if (d.isEmpty()) d = "未分配";
            s->append(d, q.value(1).toInt());
        }
        chart->addSeries(s);
        chart->setTitle("部门人数分布");
        break;
    }
    case 1: {
        q.exec("SELECT status, COUNT(*) FROM employees GROUP BY status");
        auto *s = new QPieSeries;
        while (q.next()) s->append(q.value(0).toString(), q.value(1).toInt());
        chart->addSeries(s);
        chart->setTitle("在职/离职比例");
        break;
    }
    case 2: {
        q.exec("SELECT department, AVG(base_salary) FROM employees WHERE base_salary>0 GROUP BY department");
        auto *set = new QBarSet("平均薪资");
        QStringList cats;
        while (q.next()) { cats << q.value(0).toString(); *set << q.value(1).toDouble(); }
        auto *s = new QBarSeries; s->append(set);
        chart->addSeries(s);
        auto *ax = new QBarCategoryAxis; ax->append(cats);
        chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
        auto *ay = new QValueAxis; ay->setTitleText("元");
        chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
        chart->setTitle("各部门平均薪资");
        break;
    }
    case 3: {
        q.exec("SELECT DATE_FORMAT(start_date,'%Y-%m'), COUNT(*) FROM leave_requests GROUP BY 1 ORDER BY 1");
        auto *set = new QBarSet("请假人次");
        QStringList cats;
        while (q.next()) { cats << q.value(0).toString(); *set << q.value(1).toInt(); }
        auto *s = new QBarSeries; s->append(set);
        chart->addSeries(s);
        auto *ax = new QBarCategoryAxis; ax->append(cats);
        chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
        auto *ay = new QValueAxis; ay->setTitleText("人次");
        chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
        chart->setTitle("月度请假统计");
        break;
    }
    }
    m_chartView->setChart(chart);
    delete old;
}

void ReportsTab::exportPDF()
{
    QString path = QFileDialog::getSaveFileName(this, "导出PDF", "报表.pdf", "PDF文件 (*.pdf)");
    if (path.isEmpty()) return;
    QPdfWriter w(path);
    w.setPageSize(QPageSize(QPageSize::A4));
    w.setPageOrientation(QPageLayout::Landscape);
    w.setResolution(300);
    QPainter p(&w);
    m_chartView->render(&p);
    p.end();
    QMessageBox::information(this, "导出成功", "报表已导出至:\n" + path);
}
