#ifndef SHIFTSETTINGSWIDGET_H
#define SHIFTSETTINGSWIDGET_H

#include <QWidget>
#include <QTimeEdit>

class ShiftSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ShiftSettingsWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();

signals:
    void logRequested(const QString &action, const QString &details);

private slots:
    void saveShiftSettings();

private:
    int m_empId;
    QString m_role;

    QTimeEdit *m_shiftStartEdit;
    QTimeEdit *m_shiftEndEdit;
};

#endif // SHIFTSETTINGSWIDGET_H
