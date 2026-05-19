#include "PaginationBar.h"
#include <QHBoxLayout>

PaginationBar::PaginationBar(int pageSize, QWidget *parent)
    : QWidget(parent), m_pageSize(pageSize), m_currentPage(1), m_totalRecords(0)
{
    auto *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 4, 0, 4);
    l->addStretch();

    m_btnPrev = new QPushButton("<< 上一页");
    m_label = new QLabel("共 0 条  第 1/1 页");
    m_label->setStyleSheet("color: #666; font-size: 12px;");
    m_btnNext = new QPushButton("下一页 >>");

    l->addWidget(m_label);
    l->addWidget(m_btnPrev);
    l->addWidget(m_btnNext);

    connect(m_btnPrev, &QPushButton::clicked, this, [this]() {
        if (m_currentPage > 1) { m_currentPage--; updateDisplay(); emit pageChanged(m_currentPage); }
    });
    connect(m_btnNext, &QPushButton::clicked, this, [this]() {
        int totalPages = qMax(1, (m_totalRecords + m_pageSize - 1) / m_pageSize);
        if (m_currentPage < totalPages) { m_currentPage++; updateDisplay(); emit pageChanged(m_currentPage); }
    });
}

void PaginationBar::setTotalRecords(int total)
{
    m_totalRecords = total;
    if (m_currentPage < 1) m_currentPage = 1;
    updateDisplay();
}

void PaginationBar::refresh()
{
    m_currentPage = 1;
    updateDisplay();
}

void PaginationBar::updateDisplay()
{
    int totalPages = qMax(1, (m_totalRecords + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    m_label->setText(QString("共 %1 条  第 %2/%3 页").arg(m_totalRecords).arg(m_currentPage).arg(totalPages));
    m_btnPrev->setEnabled(m_currentPage > 1);
    m_btnNext->setEnabled(m_currentPage < totalPages);
}
