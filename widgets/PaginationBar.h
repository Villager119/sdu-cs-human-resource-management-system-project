#ifndef PAGINATIONBAR_H
#define PAGINATIONBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

class PaginationBar : public QWidget
{
    Q_OBJECT
public:
    explicit PaginationBar(int pageSize = 20, QWidget *parent = nullptr);
    void setTotalRecords(int total);
    int currentPage() const { return m_currentPage; }

signals:
    void pageChanged(int page);

public slots:
    void refresh();

private:
    void updateDisplay();
    QPushButton *m_btnPrev, *m_btnNext;
    QLabel *m_label;
    int m_pageSize, m_currentPage, m_totalRecords;
};

#endif
