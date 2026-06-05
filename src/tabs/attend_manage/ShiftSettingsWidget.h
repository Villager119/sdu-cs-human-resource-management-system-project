#ifndef SHIFTSETTINGSWIDGET_H
#define SHIFTSETTINGSWIDGET_H

#include <QWidget>
#include <QTimeEdit>
#include <QSqlTableModel>
#include <QTableView>
#include "../../utils/UnsavedChangesGuard.h"

class ShiftSettingsWidget : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    explicit ShiftSettingsWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override;
    QString unsavedChangesMessage() const override { return "班次设置还有未保存的修改，是否先保存再离开？"; }

signals:
    void logRequested(const QString &action, const QString &details);

private slots:
    void saveShiftSettings();
    void addShift();
    void removeSelectedShift();
    void saveShiftTable();

private:
    int m_empId;
    QString m_role;

    QTimeEdit *m_shiftStartEdit;
    QTimeEdit *m_shiftEndEdit;
    QSqlTableModel *m_shiftModel = nullptr;
    QTableView *m_shiftTable = nullptr;
    QTime m_savedStart;
    QTime m_savedEnd;
    bool saveInternal(bool showMessage);
    bool validateShiftRows(QString *errorText) const;
};

#endif // SHIFTSETTINGSWIDGET_H
