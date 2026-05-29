#include "ShiftSettingsWidget.h"
#include "../../utils/Toast.h"
#include "../../core/GlobalEvents.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QTime>

static QTime parseShiftTime(const QVariant &value)
{
    QTime parsed = value.toTime();
    if (parsed.isValid()) {
        return parsed;
    }

    QString text = value.toString();
    if (text.size() >= 8) {
        text = text.left(8);
    }
    parsed = QTime::fromString(text, "HH:mm:ss");
    if (parsed.isValid()) {
        return parsed;
    }
    return QTime::fromString(text, "HH:mm");
}

ShiftSettingsWidget::ShiftSettingsWidget(int empId, const QString &role, QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(15, 15, 15, 15);
    l->setSpacing(15);

    QFrame *panel = new QFrame(this);
    panel->setObjectName("shiftSettingsPanel");
    panel->setStyleSheet(
        "QFrame#shiftSettingsPanel {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}"
    );
    panel->setFixedWidth(400);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 20, 20, 20);
    panelLayout->setSpacing(15);

    QLabel *title = new QLabel("标准班次工作时间设置", panel);
    title->setStyleSheet("font-size: 15px; font-weight: bold; color: #1d4ed8; border: none; background: transparent;");
    panelLayout->addWidget(title);

    panelLayout->addWidget(new QLabel("上班打卡时间:", panel));
    m_shiftStartEdit = new QTimeEdit(panel);
    m_shiftStartEdit->setDisplayFormat("HH:mm");
    m_shiftStartEdit->setStyleSheet(
        "QTimeEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "}"
        "QTimeEdit:focus { border: 1px solid #3b82f6; }"
    );
    panelLayout->addWidget(m_shiftStartEdit);

    panelLayout->addWidget(new QLabel("下班打卡时间:", panel));
    m_shiftEndEdit = new QTimeEdit(panel);
    m_shiftEndEdit->setDisplayFormat("HH:mm");
    m_shiftEndEdit->setStyleSheet(
        "QTimeEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "}"
        "QTimeEdit:focus { border: 1px solid #3b82f6; }"
    );
    panelLayout->addWidget(m_shiftEndEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton *btnSave = new QPushButton("保存设置", panel);
    btnSave->setStyleSheet(
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
    btnRow->addWidget(btnSave);
    panelLayout->addLayout(btnRow);

    connect(btnSave, &QPushButton::clicked, this, &ShiftSettingsWidget::saveShiftSettings);

    l->addWidget(panel);
    l->addStretch(1);

    refresh();
}

void ShiftSettingsWidget::refresh()
{
    // Load shift times for the shift configuration tab
    {
        QSqlQuery sq;
        if (sq.exec("SELECT start_time, end_time FROM shifts WHERE shift_id = 1") && sq.next()) {
            QTime start = parseShiftTime(sq.value(0));
            QTime end = parseShiftTime(sq.value(1));
            if (!start.isValid()) start = QTime(9, 0);
            if (!end.isValid()) end = QTime(18, 0);
            if (m_shiftStartEdit) {
                m_shiftStartEdit->setTime(start);
                m_savedStart = m_shiftStartEdit->time();
            }
            if (m_shiftEndEdit) {
                m_shiftEndEdit->setTime(end);
                m_savedEnd = m_shiftEndEdit->time();
            }
        }
        sq.finish();
    }
}

void ShiftSettingsWidget::saveShiftSettings()
{
    saveInternal(true);
}

bool ShiftSettingsWidget::hasUnsavedChanges() const
{
    return m_shiftStartEdit && m_shiftEndEdit
        && (m_shiftStartEdit->time() != m_savedStart || m_shiftEndEdit->time() != m_savedEnd);
}

bool ShiftSettingsWidget::saveChanges()
{
    return saveInternal(true);
}

void ShiftSettingsWidget::discardChanges()
{
    refresh();
}

bool ShiftSettingsWidget::saveInternal(bool showMessage)
{
    QTime start = m_shiftStartEdit->time();
    QTime end = m_shiftEndEdit->time();

    if (end <= start) {
        Toast::show(this, "下班时间不能早于或等于上班时间", Toast::Warning);
        return false;
    }

    bool ok = false;
    QString err;
    {
        QSqlQuery q;
        q.prepare("UPDATE shifts SET start_time = ?, end_time = ? WHERE shift_id = 1");
        q.addBindValue(start.toString("HH:mm:ss"));
        q.addBindValue(end.toString("HH:mm:ss"));
        ok = q.exec();
        if (!ok) err = q.lastError().text();
        q.finish();
    }

    if (ok) {
        m_savedStart = start;
        m_savedEnd = end;
        if (showMessage) {
            Toast::show(this, "班次时间设置已保存", Toast::Success);
        }
        emit logRequested("修改班次时间", QString("上班: %1, 下班: %2").arg(start.toString("HH:mm"), end.toString("HH:mm")));
        GlobalEvents::instance()->dataChanged();
        return true;
    } else {
        Toast::show(this, "保存失败：" + err, Toast::Error);
        return false;
    }
}
