#include "PerformanceTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

PerformanceTab::PerformanceTab(int empId, const QString &role,
                               std::function<void(const QString&, const QString&)> logFn,
                               QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn)
{
    // 建表
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS performance_scores ("
           "score_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL,"
           "eval_month VARCHAR(7) NOT NULL,"
           "score DECIMAL(5,2) NOT NULL,"
           "comment TEXT DEFAULT '',"
           "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    m_model = new QSqlTableModel(this);
    m_model->setTable("performance_scores");
    m_model->setHeaderData(0, Qt::Horizontal, "编号");
    m_model->setHeaderData(1, Qt::Horizontal, "员工ID");
    m_model->setHeaderData(2, Qt::Horizontal, "考核月份");
    m_model->setHeaderData(3, Qt::Horizontal, "评分(0-100)");
    m_model->setHeaderData(4, Qt::Horizontal, "评语");
    m_model->setHeaderData(5, Qt::Horizontal, "提交时间");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if (m_role == "user")
        m_model->setFilter(QString("emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->hideColumn(1); // 隐藏 emp_id

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table, 1);

    // 打分面板（仅 admin）
    m_scorePanel = new QGroupBox("绩效评分");
    if (m_role == "user") m_scorePanel->setVisible(false);
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

    m_scoreSpin = new QSpinBox;
    m_scoreSpin->setRange(0, 100);
    m_scoreSpin->setValue(80);
    m_scoreSpin->setSuffix(" 分");
    form->addRow("评分:", m_scoreSpin);

    m_commentEdit = new QTextEdit;
    m_commentEdit->setMaximumHeight(60);
    m_commentEdit->setPlaceholderText("可选评语...");
    form->addRow("评语:", m_commentEdit);

    auto *submitBtn = new QPushButton("提交评分");
    submitBtn->setStyleSheet("QPushButton { background: #1e3a5f; color: #fff; } QPushButton:hover { background: #2a5080; }");
    form->addRow(submitBtn);
    connect(submitBtn, &QPushButton::clicked, this, &PerformanceTab::submitScore);

    layout->addWidget(m_scorePanel);
    m_model->select();
}

void PerformanceTab::submitScore()
{
    int eid = m_empCombo->currentData().toInt();
    QString month = m_monthCombo->currentText().trimmed();
    int score = m_scoreSpin->value();
    QString comment = m_commentEdit->toPlainText().trimmed();

    if (month.isEmpty()) { QMessageBox::warning(this, "提示", "请输入考核月份"); return; }

    // 若已存在则更新，否则插入
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM performance_scores WHERE emp_id=? AND eval_month=?");
    check.addBindValue(eid); check.addBindValue(month); check.exec();
    if (check.next() && check.value(0).toInt() > 0) {
        QSqlQuery upd;
        upd.prepare("UPDATE performance_scores SET score=?, comment=?, created_at=NOW() WHERE emp_id=? AND eval_month=?");
        upd.addBindValue(score); upd.addBindValue(comment); upd.addBindValue(eid); upd.addBindValue(month);
        if (!upd.exec()) { QMessageBox::critical(this, "错误", upd.lastError().text()); return; }
    } else {
        QSqlQuery ins;
        ins.prepare("INSERT INTO performance_scores(emp_id,eval_month,score,comment) VALUES(?,?,?,?)");
        ins.addBindValue(eid); ins.addBindValue(month); ins.addBindValue(score); ins.addBindValue(comment);
        if (!ins.exec()) { QMessageBox::critical(this, "错误", ins.lastError().text()); return; }
    }

    QString empName = m_empCombo->currentText();
    m_log("绩效评分", QString("%1 %2月 → %3分").arg(empName, month).arg(score));
    QMessageBox::information(this, "成功", QString("已为 %1 提交 %2 月绩效评分 %3 分").arg(empName, month).arg(score));
    m_commentEdit->clear();
    m_model->select();
}

void PerformanceTab::refresh()
{
    m_model->select();
}
