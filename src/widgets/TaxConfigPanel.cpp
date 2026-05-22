#include "TaxConfigPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

TaxConfigPanel::TaxConfigPanel(std::function<void(const QString&, const QString&)> logFn,
                               QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 全局薪资参数配置
    auto *settingsGroup = new QGroupBox("全局薪酬核算参数");
    auto *formLayout = new QFormLayout(settingsGroup);

    m_workDaysEdit = new QLineEdit;
    m_workDaysEdit->setPlaceholderText("21.75");
    formLayout->addRow("月度标准工作天数:", m_workDaysEdit);

    m_taxThresholdEdit = new QLineEdit;
    m_taxThresholdEdit->setPlaceholderText("5000");
    formLayout->addRow("个人所得税起征点 (元):", m_taxThresholdEdit);

    // 加载初始值
    QSqlQuery q;
    q.exec("SELECT value FROM system_settings WHERE key_name='work_days_per_month'");
    if (q.next()) {
        m_workDaysEdit->setText(q.value(0).toString());
    } else {
        m_workDaysEdit->setText("21.75");
    }

    q.exec("SELECT value FROM system_settings WHERE key_name='tax_threshold'");
    if (q.next()) {
        m_taxThresholdEdit->setText(q.value(0).toString());
    } else {
        m_taxThresholdEdit->setText("5000");
    }

    // 社保公积金比例模型
    m_model = new QSqlTableModel(this);
    m_model->setTable("salary_config");
    m_model->setHeaderData(1, Qt::Horizontal, "保险项目");
    m_model->setHeaderData(2, Qt::Horizontal, "个人比例");
    m_model->setHeaderData(3, Qt::Horizontal, "启用");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->select();

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->hideColumn(0);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_btnSave = new QPushButton("保存配置");
    m_btnSave->setStyleSheet("QPushButton{background:#1a2233;color:#fff;} QPushButton:hover{background:#2a3a55;}");

    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(settingsGroup);
    l->addWidget(new QLabel("社保公积金比例配置（双击表格项修改，保存后下次核算生效）"));
    l->addWidget(m_table, 1);
    
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(m_btnSave);
    l->addLayout(row);

    connect(m_btnSave, &QPushButton::clicked, this, &TaxConfigPanel::save);
}

void TaxConfigPanel::save()
{
    // 输入校验
    bool ok1 = false, ok2 = false;
    double wd = m_workDaysEdit->text().toDouble(&ok1);
    double threshold = m_taxThresholdEdit->text().toDouble(&ok2);
    if (!ok1 || wd <= 0) {
        QMessageBox::warning(this, "提示", "请输入有效的工作天数！");
        return;
    }
    if (!ok2 || threshold < 0) {
        QMessageBox::warning(this, "提示", "请输入有效的个税起征点！");
        return;
    }

    // 保存到 system_settings 表
    QSqlQuery q;
    q.prepare("UPDATE system_settings SET value=? WHERE key_name='work_days_per_month'");
    q.addBindValue(m_workDaysEdit->text().trimmed());
    if (!q.exec()) {
        QMessageBox::critical(this, "失败", "保存月工作天数失败: " + q.lastError().text());
        return;
    }

    q.prepare("UPDATE system_settings SET value=? WHERE key_name='tax_threshold'");
    q.addBindValue(m_taxThresholdEdit->text().trimmed());
    if (!q.exec()) {
        QMessageBox::critical(this, "失败", "保存个税起征点失败: " + q.lastError().text());
        return;
    }

    // 保存表格中的社保比例模型
    if (m_model->submitAll()) {
        QMessageBox::information(this, "成功", "薪资及社保比例配置已保存");
        m_log("修改薪薪资及社保配置", QString("月天数:%1, 起征点:%2").arg(wd).arg(threshold));
    } else {
        QMessageBox::critical(this, "失败", "社保比例保存失败: " + m_model->lastError().text());
    }
}
