#ifndef DASHBOARDTAB_H
#define DASHBOARDTAB_H

#include <QWidget>
#include <QLabel>

class DashboardTab : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardTab(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    QLabel *m_labels[6];
    QLabel *m_alertLabel;
};

#endif
