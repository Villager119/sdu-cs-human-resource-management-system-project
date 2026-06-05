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
#include <QHeaderView>
#include <QItemSelectionModel>
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

    QLabel *tableTitle = new QLabel("班次列表（员工管理中填写对应班次ID）", this);
    tableTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #1d4ed8;");
    l->addWidget(tableTitle);

    m_shiftModel = new QSqlTableModel(this);
    m_shiftModel->setTable("shifts");
    m_shiftModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_shiftModel->setHeaderData(0, Qt::Horizontal, "班次ID");
    m_shiftModel->setHeaderData(1, Qt::Horizontal, "班次名称");
    m_shiftModel->setHeaderData(2, Qt::Horizontal, "上班时间");
    m_shiftModel->setHeaderData(3, Qt::Horizontal, "下班时间");

    m_shiftTable = new QTableView(this);
    m_shiftTable->setModel(m_shiftModel);
    m_shiftTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_shiftTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_shiftTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    l->addWidget(m_shiftTable, 1);

    auto *tableBtnRow = new QHBoxLayout;
    tableBtnRow->addStretch();
    QPushButton *btnAddShift = new QPushButton("新增班次", this);
    QPushButton *btnRemoveShift = new QPushButton("删除班次", this);
    QPushButton *btnSaveTable = new QPushButton("保存班次列表", this);
    tableBtnRow->addWidget(btnAddShift);
    tableBtnRow->addWidget(btnRemoveShift);
    tableBtnRow->addWidget(btnSaveTable);
    l->addLayout(tableBtnRow);

    connect(btnAddShift, &QPushButton::clicked, this, &ShiftSettingsWidget::addShift);
    connect(btnRemoveShift, &QPushButton::clicked, this, &ShiftSettingsWidget::removeSelectedShift);
    connect(btnSaveTable, &QPushButton::clicked, this, &ShiftSettingsWidget::saveShiftTable);

    l->addStretch(1);

    refresh();
}

void ShiftSettingsWidget::refresh()
{
    if (m_shiftModel) {
        m_shiftModel->select();
    }

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
    return (m_shiftModel && m_shiftModel->isDirty())
        || (m_shiftStartEdit && m_shiftEndEdit
            && (m_shiftStartEdit->time() != m_savedStart || m_shiftEndEdit->time() != m_savedEnd));
}

bool ShiftSettingsWidget::saveChanges()
{
    if (m_shiftModel && m_shiftModel->isDirty()) {
        QString errorText;
        if (!validateShiftRows(&errorText)) {
            Toast::show(this, errorText, Toast::Warning);
            return false;
        }
        if (!m_shiftModel->submitAll()) {
            Toast::show(this, "保存班次列表失败：" + m_shiftModel->lastError().text(), Toast::Error);
            return false;
        }
    }
    if (!saveInternal(false)) {
        return false;
    }
    refresh();
    GlobalEvents::instance()->dataChanged();
    return true;
}

void ShiftSettingsWidget::discardChanges()
{
    if (m_shiftModel) {
        m_shiftModel->revertAll();
    }
    refresh();
}

void ShiftSettingsWidget::addShift()
{
    if (!m_shiftModel) return;
    const int row = m_shiftModel->rowCount();
    m_shiftModel->insertRow(row);
    m_shiftModel->setData(m_shiftModel->index(row, 1), QString("新班次"));
    m_shiftModel->setData(m_shiftModel->index(row, 2), QString("09:00:00"));
    m_shiftModel->setData(m_shiftModel->index(row, 3), QString("18:00:00"));
    if (m_shiftTable) {
        m_shiftTable->selectRow(row);
    }
}

void ShiftSettingsWidget::removeSelectedShift()
{
    if (!m_shiftModel || !m_shiftTable || !m_shiftTable->selectionModel()) return;
    const auto rows = m_shiftTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        Toast::show(this, "请先选择要删除的班次", Toast::Warning);
        return;
    }

    const int row = rows.first().row();
    const int shiftId = m_shiftModel->data(m_shiftModel->index(row, 0)).toInt();
    if (shiftId == 1) {
        Toast::show(this, "标准班不能删除", Toast::Warning);
        return;
    }
    if (shiftId > 0) {
        QSqlQuery usedQuery;
        usedQuery.prepare("SELECT COUNT(*) FROM employees WHERE shift_id=?");
        usedQuery.addBindValue(shiftId);
        if (!usedQuery.exec() || !usedQuery.next()) {
            Toast::show(this, "检查班次占用失败：" + usedQuery.lastError().text(), Toast::Error);
            usedQuery.finish();
            return;
        }
        const int usedCount = usedQuery.value(0).toInt();
        usedQuery.finish();
        if (usedCount > 0) {
            Toast::show(this, QString("该班次已被 %1 名员工使用，请先调整员工班次").arg(usedCount), Toast::Warning);
            return;
        }
    }
    m_shiftModel->removeRow(row);
}

void ShiftSettingsWidget::saveShiftTable()
{
    if (!m_shiftModel) return;
    QString errorText;
    if (!validateShiftRows(&errorText)) {
        Toast::show(this, errorText, Toast::Warning);
        return;
    }

    if (!m_shiftModel->submitAll()) {
        Toast::show(this, "保存班次列表失败：" + m_shiftModel->lastError().text(), Toast::Error);
        return;
    }

    refresh();
    emit logRequested("保存班次列表", "更新班次定义");
    GlobalEvents::instance()->dataChanged();
    Toast::show(this, "班次列表已保存", Toast::Success);
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
        if (m_shiftModel && !m_shiftModel->isDirty()) {
            m_shiftModel->select();
        }
        GlobalEvents::instance()->dataChanged();
        return true;
    } else {
        Toast::show(this, "保存失败：" + err, Toast::Error);
        return false;
    }
}

bool ShiftSettingsWidget::validateShiftRows(QString *errorText) const
{
    if (!m_shiftModel) return true;
    for (int row = 0; row < m_shiftModel->rowCount(); ++row) {
        if (m_shiftModel->headerData(row, Qt::Vertical).toString() == "!") {
            continue;
        }
        const QString name = m_shiftModel->data(m_shiftModel->index(row, 1)).toString().trimmed();
        const QTime start = parseShiftTime(m_shiftModel->data(m_shiftModel->index(row, 2)));
        const QTime end = parseShiftTime(m_shiftModel->data(m_shiftModel->index(row, 3)));
        if (name.isEmpty()) {
            if (errorText) *errorText = QString("第 %1 行：班次名称不能为空").arg(row + 1);
            return false;
        }
        if (!start.isValid() || !end.isValid() || end <= start) {
            if (errorText) *errorText = QString("第 %1 行：班次时间无效").arg(row + 1);
            return false;
        }
    }
    return true;
}
