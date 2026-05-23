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
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
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
    m_combo->addItems({
        "部门人数分布",
        "在职/离职比例",
        "学历分布",
        "婚姻状况比例",
        "岗位分布",
        "入职年份分布",
        "工资区间统计",
        "各部门平均薪资",
        "月度请假统计",
        "月度薪资趋势"
    });
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
    chart->setTheme(QChart::ChartThemeLight);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    QSqlQuery q;

    switch (type) {
    case 0: {
        q.exec("SELECT department, COUNT(*) FROM employees WHERE status='在职' GROUP BY department");
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
        q.exec("SELECT education, COUNT(*) FROM employees WHERE status='在职' GROUP BY education");
        auto *s = new QPieSeries;
        while (q.next()) {
            QString label = q.value(0).toString().trimmed();
            if (label.isEmpty()) label = "未填写";
            s->append(label, q.value(1).toInt());
        }
        chart->addSeries(s);
        chart->setTitle("学历分布");
        break;
    }
    case 3: {
        q.exec("SELECT marital_status, COUNT(*) FROM employees WHERE status='在职' GROUP BY marital_status");
        auto *s = new QPieSeries;
        while (q.next()) {
            QString label = q.value(0).toString().trimmed();
            if (label.isEmpty()) label = "未填写";
            s->append(label, q.value(1).toInt());
        }
        chart->addSeries(s);
        chart->setTitle("婚姻状况比例");
        break;
    }
    case 4: {
        q.exec("SELECT position, COUNT(*) FROM employees WHERE status='在职' GROUP BY position");
        auto *s = new QPieSeries;
        while (q.next()) {
            QString label = q.value(0).toString().trimmed();
            if (label.isEmpty()) label = "未指定";
            s->append(label, q.value(1).toInt());
        }
        chart->addSeries(s);
        chart->setTitle("岗位分布");
        break;
    }
    case 5: {
        q.exec("SELECT DATE_FORMAT(hire_date, '%Y'), COUNT(*) FROM employees WHERE hire_date IS NOT NULL GROUP BY 1 ORDER BY 1");
        auto *set = new QBarSet("入职人数");
        QStringList cats;
        while (q.next()) {
            cats << q.value(0).toString() + "年";
            *set << q.value(1).toInt();
        }
        auto *s = new QBarSeries; s->append(set);
        chart->addSeries(s);
        auto *ax = new QBarCategoryAxis; ax->append(cats);
        chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
        auto *ay = new QValueAxis; ay->setTitleText("人数");
        chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
        chart->setTitle("入职年份分布");
        break;
    }
    case 6: {
        q.exec("SELECT "
               "SUM(CASE WHEN base_salary < 5000 THEN 1 ELSE 0 END), "
               "SUM(CASE WHEN base_salary >= 5000 AND base_salary < 10000 THEN 1 ELSE 0 END), "
               "SUM(CASE WHEN base_salary >= 10000 AND base_salary < 20000 THEN 1 ELSE 0 END), "
               "SUM(CASE WHEN base_salary >= 20000 THEN 1 ELSE 0 END) "
               "FROM employees WHERE status='在职'");
        auto *set = new QBarSet("员工人数");
        QStringList cats = {"<5000元", "5000-10000元", "10000-20000元", ">=20000元"};
        if (q.next()) {
            *set << q.value(0).toInt()
                 << q.value(1).toInt()
                 << q.value(2).toInt()
                 << q.value(3).toInt();
        } else {
            *set << 0 << 0 << 0 << 0;
        }
        auto *s = new QBarSeries; s->append(set);
        chart->addSeries(s);
        auto *ax = new QBarCategoryAxis; ax->append(cats);
        chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
        auto *ay = new QValueAxis; ay->setTitleText("人数");
        chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
        chart->setTitle("员工工资区间分布");
        break;
    }
    case 7: {
        q.exec("SELECT department, AVG(base_salary) FROM employees WHERE base_salary>0 AND status='在职' GROUP BY department");
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
    case 8: {
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
    case 9: {
        q.exec("SELECT month, SUM(net_salary) FROM payroll GROUP BY month ORDER BY month");
        auto *s = new QLineSeries;
        s->setName("薪资总额");
        QStringList cats;
        while (q.next()) { cats << q.value(0).toString(); s->append(cats.size()-1, q.value(1).toDouble()); }
        chart->addSeries(s);
        auto *ax = new QBarCategoryAxis; ax->append(cats);
        chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
        auto *ay = new QValueAxis; ay->setTitleText("元");
        chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
        chart->setTitle("月度薪资趋势");
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

void ReportsTab::refresh()
{
    refreshChart();
}
