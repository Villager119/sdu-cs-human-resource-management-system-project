#include "PerformanceTab.h"
#include <QVBoxLayout>
#include "../core/SessionManager.h"
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

PerformanceTab::PerformanceTab(int empId, const QString &role,
                               std::function<void(const QString&, const QString&)> logFn,
                               QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn)
{
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS performance_scores ("
           "score_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL, eval_month VARCHAR(7) NOT NULL,"
           "attitude DECIMAL(5,2) DEFAULT 0, capability DECIMAL(5,2) DEFAULT 0,"
           "teamwork DECIMAL(5,2) DEFAULT 0, innovation DECIMAL(5,2) DEFAULT 0,"
           "score DECIMAL(5,2) NOT NULL, comment TEXT,"
           "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY(emp_id) REFERENCES employees(emp_id)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    for (auto &col : {"attitude", "capability", "teamwork", "innovation"}) {
        q.exec(QString("SHOW COLUMNS FROM performance_scores LIKE '%1'").arg(col));
        if (!q.next())
            q.exec(QString("ALTER TABLE performance_scores ADD COLUMN %1 DECIMAL(5,2) DEFAULT 0 AFTER eval_month").arg(col));
    }

    m_model = new QSqlTableModel(this);
    m_model->setTable("performance_scores");
    m_model->setHeaderData(0, Qt::Horizontal, "编号");
    m_model->setHeaderData(1, Qt::Horizontal, "员工ID");
    m_model->setHeaderData(2, Qt::Horizontal, "月份");
    m_model->setHeaderData(3, Qt::Horizontal, "态度(25)");
    m_model->setHeaderData(4, Qt::Horizontal, "能力(25)");
    m_model->setHeaderData(5, Qt::Horizontal, "协作(25)");
    m_model->setHeaderData(6, Qt::Horizontal, "创新(25)");
    m_model->setHeaderData(7, Qt::Horizontal, "总分");
    m_model->setHeaderData(8, Qt::Horizontal, "评语");
    m_model->setHeaderData(9, Qt::Horizontal, "时间");
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) m_model->setFilter(QString("emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->hideColumn(1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table, 1);

    m_scorePanel = new QGroupBox("绩效评分 (4维度 × 25分 = 100分)");
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) m_scorePanel->setVisible(false);
    auto *form = new QFormLayout(m_scorePanel);

    m_empCombo = new QComboBox;
    QSqlQuery eq("SELECT emp_id, name FROM employees");
    while (eq.next()) m_empCombo->addItem(eq.value(1).toString(), eq.value(0).toInt());
    form->addRow("员工:", m_empCombo);

    m_monthCombo = new QComboBox;
    m_monthCombo->addItem(QDate::currentDate().toString("yyyy-MM"));
    m_monthCombo->addItem(QDate::currentDate().addMonths(-1).toString("yyyy-MM"));
    m_monthCombo->setEditable(true);
    form->addRow("月份:", m_monthCombo);

    auto *grid = new QGridLayout;
    m_s1 = new QSpinBox; m_s1->setRange(0, 25); m_s1->setSuffix("分");
    m_s2 = new QSpinBox; m_s2->setRange(0, 25); m_s2->setSuffix("分");
    m_s3 = new QSpinBox; m_s3->setRange(0, 25); m_s3->setSuffix("分");
    m_s4 = new QSpinBox; m_s4->setRange(0, 25); m_s4->setSuffix("分");
    grid->addWidget(new QLabel("工作态度"), 0, 0); grid->addWidget(m_s1, 0, 1);
    grid->addWidget(new QLabel("专业能力"), 0, 2); grid->addWidget(m_s2, 0, 3);
    grid->addWidget(new QLabel("团队协作"), 1, 0); grid->addWidget(m_s3, 1, 1);
    grid->addWidget(new QLabel("创新能力"), 1, 2); grid->addWidget(m_s4, 1, 3);
    form->addRow(grid);

    m_totalLabel = new QLabel("总分: 0");
    m_totalLabel->setStyleSheet("font-size:18px;font-weight:700;color:#1976d2;");
    form->addRow(m_totalLabel);

    m_commentEdit = new QTextEdit; m_commentEdit->setMaximumHeight(50);
    m_commentEdit->setPlaceholderText("评语(可选)");
    form->addRow("评语:", m_commentEdit);

    auto *btn = new QPushButton("提交评分");
    btn->setStyleSheet("QPushButton{background:#1a2233;color:#fff;font-size:13px;padding:8px;} QPushButton:hover{background:#2a3a55;}");
    form->addRow(btn);

    layout->addWidget(m_scorePanel);
    connect(btn, &QPushButton::clicked, this, &PerformanceTab::submitScore);
    connect(m_s1, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s2, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s3, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s4, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    m_model->select();
}

void PerformanceTab::updateTotal()
{
    int t = m_s1->value() + m_s2->value() + m_s3->value() + m_s4->value();
    m_totalLabel->setText(QString("总分: %1").arg(t));
}

void PerformanceTab::submitScore()
{
    int eid = m_empCombo->currentData().toInt();
    QString month = m_monthCombo->currentText().trimmed();
    int a1 = m_s1->value(), a2 = m_s2->value(), a3 = m_s3->value(), a4 = m_s4->value();
    int total = a1 + a2 + a3 + a4;
    QString comment = m_commentEdit->toPlainText().trimmed();
    if (month.isEmpty()) { QMessageBox::warning(this, "提示", "请输入考核月份"); return; }

    QSqlQuery ck; ck.prepare("SELECT COUNT(*) FROM performance_scores WHERE emp_id=? AND eval_month=?");
    ck.addBindValue(eid); ck.addBindValue(month); ck.exec();
    QSqlQuery q;
    if (ck.next() && ck.value(0).toInt() > 0) {
        q.prepare("UPDATE performance_scores SET attitude=?,capability=?,teamwork=?,innovation=?,score=?,comment=?,created_at=NOW() WHERE emp_id=? AND eval_month=?");
        q.addBindValue(a1); q.addBindValue(a2); q.addBindValue(a3); q.addBindValue(a4);
        q.addBindValue(total); q.addBindValue(comment); q.addBindValue(eid); q.addBindValue(month);
    } else {
        q.prepare("INSERT INTO performance_scores(emp_id,eval_month,attitude,capability,teamwork,innovation,score,comment) VALUES(?,?,?,?,?,?,?,?)");
        q.addBindValue(eid); q.addBindValue(month); q.addBindValue(a1); q.addBindValue(a2);
        q.addBindValue(a3); q.addBindValue(a4); q.addBindValue(total); q.addBindValue(comment);
    }
    if (!q.exec()) { QMessageBox::critical(this, "错误", q.lastError().text()); return; }
    m_log("绩效评分", QString("%1 %2月 态度%3 能力%4 协作%5 创新%6 → 总分%7")
          .arg(m_empCombo->currentText(), month).arg(a1).arg(a2).arg(a3).arg(a4).arg(total));
    QMessageBox::information(this, "成功", QString("已提交 %1 月份绩效: 总分 %2").arg(month).arg(total));
    m_model->select();
}

void PerformanceTab::refresh() { m_model->select(); }
