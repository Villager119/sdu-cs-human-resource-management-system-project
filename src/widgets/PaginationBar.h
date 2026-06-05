#ifndef PAGINATIONBAR_H
#define PAGINATIONBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

class PaginationBar : public QWidget
{
    Q_OBJECT
public:
    explicit PaginationBar(int pageSize = 20, QWidget *parent = nullptr);
    void setTotalRecords(int total);
    void setCurrentPage(int page);
    int currentPage() const { return m_currentPage; }

signals:
    void pageChanged(int page);

public slots:
    void refresh();

private:
    void updateDisplay();
    int totalPages() const;
    void goToPage(int page);

    QPushButton *m_btnPrev, *m_btnNext;
    QPushButton *m_btnJump;
    QLabel *m_label;
    QSpinBox *m_pageSpin;
    int m_pageSize, m_currentPage, m_totalRecords;
};

#endif
