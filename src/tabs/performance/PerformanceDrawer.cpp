#include "PerformanceDrawer.h"
#include "../../widgets/CommonDelegates.h"
#include "../../utils/Toast.h"
#include "../../core/GlobalEvents.h"
#include "../../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QRegularExpression>

PerformanceDrawer::PerformanceDrawer(QWidget *parent)
    : QWidget(parent), m_isEditMode(false)
{
    setObjectName("drawerPanel");
    setStyleSheet(
        "QWidget#drawerPanel {"
        "  background-color: #ffffff;"
        "  border-left: 1px solid #cbd5e1;"
        "}"
    );
    setFixedWidth(380);

    QVBoxLayout *drawerLayout = new QVBoxLayout(this);
    drawerLayout->setContentsMargins(15, 15, 15, 15);
    drawerLayout->setSpacing(12);

    // Title Row with close button
    QHBoxLayout *titleRow = new QHBoxLayout;
    m_drawerTitleLabel = new QLabel("新增绩效评分", this);
    m_drawerTitleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0f172a;");
    m_btnCloseDrawer = new QPushButton("✕", this);
    m_btnCloseDrawer->setFixedSize(24, 24);
    m_btnCloseDrawer->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  color: #94a3b8;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; color: #475569; }"
    );
    titleRow->addWidget(m_drawerTitleLabel);
    titleRow->addStretch();
    titleRow->addWidget(m_btnCloseDrawer);
    drawerLayout->addLayout(titleRow);

    // Form items
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setSpacing(8);

    m_empCombo = new QComboBox(this);
    m_empCombo->setStyleSheet("QComboBox { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; background: white; }");
    formLayout->addRow("员工:", m_empCombo);

    m_drawerMonthEdit = new QLineEdit(this);
    m_drawerMonthEdit->setReadOnly(true);
    m_drawerMonthEdit->setStyleSheet("QLineEdit { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; background: #f8fafc; color: #64748b; }");
    formLayout->addRow("月份:", m_drawerMonthEdit);
    drawerLayout->addLayout(formLayout);

    // Total score card box
    QFrame *totalCard = new QFrame(this);
    totalCard->setObjectName("totalCard");
    totalCard->setStyleSheet(
        "QFrame#totalCard {"
        "  background-color: #eff6ff;"
        "  border: 1px solid #bfdbfe;"
        "  border-radius: 8px;"
        "  padding: 12px;"
        "}"
    );
    QVBoxLayout *totalCardLayout = new QVBoxLayout(totalCard);
    totalCardLayout->setContentsMargins(0, 0, 0, 0);
    m_totalValLabel = new QLabel("总分: 60", totalCard);
    m_totalValLabel->setObjectName("totalValLabel");
    m_totalValLabel->setStyleSheet("font-size: 26px; font-weight: 800; color: #1e3a8a;");
    m_totalValLabel->setAlignment(Qt::AlignCenter);
    totalCardLayout->addWidget(m_totalValLabel);
    drawerLayout->addWidget(totalCard);

    // 2x2 Grid Layout for scores
    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(10);

    // Setup QSpinBoxes
    m_s1 = new QSpinBox(this); m_s1->setRange(0, 25); m_s1->setAlignment(Qt::AlignCenter); m_s1->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s2 = new QSpinBox(this); m_s2->setRange(0, 25); m_s2->setAlignment(Qt::AlignCenter); m_s2->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s3 = new QSpinBox(this); m_s3->setRange(0, 25); m_s3->setAlignment(Qt::AlignCenter); m_s3->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s4 = new QSpinBox(this); m_s4->setRange(0, 25); m_s4->setAlignment(Qt::AlignCenter); m_s4->setButtonSymbols(QAbstractSpinBox::NoButtons);

    QString spinStyle = 
        "QSpinBox {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 14px;"
        "  background: white;"
        "  font-weight: bold;"
        "  color: #0f172a;"
        "}"
        "QSpinBox:focus { border: 1px solid #3b82f6; }";
    m_s1->setStyleSheet(spinStyle);
    m_s2->setStyleSheet(spinStyle);
    m_s3->setStyleSheet(spinStyle);
    m_s4->setStyleSheet(spinStyle);

    // Add columns with icons and "/ 25分"
    auto makeGridCell = [](const QString &icon, const QString &text, QSpinBox *spin, QWidget *parent) {
        QWidget *w = new QWidget(parent);
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(4);

        QLabel *lblTitle = new QLabel(icon + " " + text, w);
        lblTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569;");
        
        QHBoxLayout *hl = new QHBoxLayout;
        hl->setSpacing(4);
        hl->addWidget(spin);
        QLabel *lblMax = new QLabel("/ 25分", w);
        lblMax->setStyleSheet("color: #64748b; font-size: 12px;");
        hl->addWidget(lblMax);

        vl->addWidget(lblTitle);
        vl->addLayout(hl);
        return w;
    };

    grid->addWidget(makeGridCell("🤝", "工作态度", m_s1, this), 0, 0);
    grid->addWidget(makeGridCell("👥", "团队协作", m_s3, this), 0, 1);
    grid->addWidget(makeGridCell("💡", "创新能力", m_s4, this), 1, 0);
    grid->addWidget(makeGridCell("🛠️", "专业能力", m_s2, this), 1, 1);
    drawerLayout->addLayout(grid);

    // Comment
    m_commentEdit = new QTextEdit(this);
    m_commentEdit->setPlaceholderText("评语(可选)");
    m_commentEdit->setStyleSheet(
        "QTextEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "  background: white;"
        "}"
        "QTextEdit:focus { border: 1px solid #3b82f6; }"
    );
    m_commentEdit->setMaximumHeight(80);
    drawerLayout->addWidget(m_commentEdit);
    drawerLayout->addStretch(1);

    // Action buttons row
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_btnCancel = new QPushButton("取消", this);
    m_btnCancel->setStyleSheet(
        "QPushButton {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px 18px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #e2e8f0; }"
    );
    m_btnSubmit = new QPushButton("提交评分", this);
    m_btnSubmit->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    btnRow->addStretch();
    btnRow->addWidget(m_btnCancel);
    btnRow->addWidget(m_btnSubmit);
    drawerLayout->addLayout(btnRow);

    loadEmployees();

    connect(m_btnCloseDrawer, &QPushButton::clicked, this, &PerformanceDrawer::closeRequested);
    connect(m_btnCancel, &QPushButton::clicked, this, &PerformanceDrawer::closeRequested);
    connect(m_btnSubmit, &QPushButton::clicked, this, &PerformanceDrawer::submitScore);

    connect(m_s1, &QSpinBox::valueChanged, this, &PerformanceDrawer::updateTotal);
    connect(m_s2, &QSpinBox::valueChanged, this, &PerformanceDrawer::updateTotal);
    connect(m_s3, &QSpinBox::valueChanged, this, &PerformanceDrawer::updateTotal);
    connect(m_s4, &QSpinBox::valueChanged, this, &PerformanceDrawer::updateTotal);
}

void PerformanceDrawer::loadEmployees()
{
    m_empCombo->clear();
    {
        QSqlQuery eq("SELECT emp_id, name FROM employees WHERE status='在职'");
        while (eq.next()) {
            m_empCombo->addItem(createAvatarIcon(eq.value(1).toString()), eq.value(1).toString(), eq.value(0).toInt());
        }
    }
}

void PerformanceDrawer::setupAddMode()
{
    m_isEditMode = false;
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    m_drawerTitleLabel->setText("新增绩效评分");
    m_drawerMonthEdit->setText(currentMonth);

    m_empCombo->setEnabled(true);
    if (m_empCombo->count() > 0) m_empCombo->setCurrentIndex(0);

    // Reset scores to default (15 out of 25)
    m_s1->setValue(15);
    m_s2->setValue(15);
    m_s3->setValue(15);
    m_s4->setValue(15);
    m_commentEdit->clear();

    updateTotal();
}

void PerformanceDrawer::setupEditMode(int empId, const QString &month, int a1, int a2, int a3, int a4, const QString &comment)
{
    m_isEditMode = true;
    m_drawerTitleLabel->setText("编辑绩效评分");
    m_drawerMonthEdit->setText(month);

    int empIndex = m_empCombo->findData(empId);
    if (empIndex >= 0) m_empCombo->setCurrentIndex(empIndex);
    m_empCombo->setEnabled(false); // Disable changing employee during editing

    m_s1->setValue(a1);
    m_s2->setValue(a2);
    m_s3->setValue(a3);
    m_s4->setValue(a4);
    m_commentEdit->setPlainText(comment);

    updateTotal();
}

void PerformanceDrawer::updateTotal()
{
    int t = m_s1->value() + m_s2->value() + m_s3->value() + m_s4->value();
    m_totalValLabel->setText(QString("总分: %1").arg(t));
}

void PerformanceDrawer::submitScore()
{
    int eid = m_empCombo->currentData().toInt();
    QString month = m_drawerMonthEdit->text().trimmed();
    int a1 = m_s1->value(), a2 = m_s2->value(), a3 = m_s3->value(), a4 = m_s4->value();
    int total = a1 + a2 + a3 + a4;
    QString comment = m_commentEdit->toPlainText().trimmed();
    
    if (month.isEmpty()) { 
        Toast::show(this, "请输入考核月份", Toast::Warning); 
        return; 
    }

    QRegularExpression re("^\\d{4}-(0[1-9]|1[0-2])$");
    if (!re.match(month).hasMatch()) {
        Toast::show(this, "考核月份格式不正确，请输入 YYYY-MM 格式", Toast::Warning);
        return;
    }

    QString evaluator = SessionManager::instance()->empName;

    bool exists = false;
    {
        // Check if score record already exists for this employee and month
        QSqlQuery ck; 
        ck.prepare("SELECT COUNT(*) FROM performance_scores WHERE emp_id=? AND eval_month=?");
        ck.addBindValue(eid); 
        ck.addBindValue(month); 
        if (ck.exec() && ck.next() && ck.value(0).toInt() > 0) {
            exists = true;
        }
    }
    
    bool queryOk = false;
    QString errText;
    {
        QSqlQuery q;
        if (exists) {
            q.prepare("UPDATE performance_scores SET attitude=?, capability=?, teamwork=?, innovation=?, score=?, comment=?, status='已发布', evaluator=?, created_at=NOW() WHERE emp_id=? AND eval_month=?");
            q.addBindValue(a1); q.addBindValue(a2); q.addBindValue(a3); q.addBindValue(a4);
            q.addBindValue(total); q.addBindValue(comment); q.addBindValue(evaluator);
            q.addBindValue(eid); q.addBindValue(month);
        } else {
            q.prepare("INSERT INTO performance_scores(emp_id, eval_month, attitude, capability, teamwork, innovation, score, comment, status, evaluator) VALUES(?,?,?,?,?,?,?,?,'已发布',?)");
            q.addBindValue(eid); q.addBindValue(month); q.addBindValue(a1); q.addBindValue(a2);
            q.addBindValue(a3); q.addBindValue(a4); q.addBindValue(total); q.addBindValue(comment);
            q.addBindValue(evaluator);
        }

        if (q.exec()) { 
            queryOk = true;
        } else {
            errText = q.lastError().text();
        }
    }

    if (!queryOk) {
        QMessageBox::critical(this, "错误", errText); 
        return; 
    }

    emit logRequested("绩效评分", QString("%1 %2月 态度%3 能力%4 协作%5 创新%6 → 总分%7")
          .arg(m_empCombo->currentText(), month).arg(a1).arg(a2).arg(a3).arg(a4).arg(total));
    Toast::show(this, QString("%1月份绩效已提交，总分: %2").arg(month).arg(total), Toast::Success);
    
    emit scoreSubmitted();
    GlobalEvents::instance()->dataChanged();
}
