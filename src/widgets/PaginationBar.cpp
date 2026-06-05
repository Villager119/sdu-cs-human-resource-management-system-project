#include "PaginationBar.h"
#include <QHBoxLayout>
#include <QSignalBlocker>

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
    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setRange(1, 1);
    m_pageSpin->setFixedWidth(70);
    m_btnJump = new QPushButton("跳转");

    l->addWidget(m_label);
    l->addWidget(m_btnPrev);
    l->addWidget(m_btnNext);
    l->addWidget(new QLabel("到第", this));
    l->addWidget(m_pageSpin);
    l->addWidget(new QLabel("页", this));
    l->addWidget(m_btnJump);

    connect(m_btnPrev, &QPushButton::clicked, this, [this]() {
        if (m_currentPage > 1) { m_currentPage--; updateDisplay(); emit pageChanged(m_currentPage); }
    });
    connect(m_btnNext, &QPushButton::clicked, this, [this]() {
        const int pages = totalPages();
        if (m_currentPage < pages) { m_currentPage++; updateDisplay(); emit pageChanged(m_currentPage); }
    });
    connect(m_btnJump, &QPushButton::clicked, this, [this]() {
        goToPage(m_pageSpin->value());
    });
}

void PaginationBar::setTotalRecords(int total)
{
    m_totalRecords = total;
    if (m_currentPage < 1) m_currentPage = 1;
    updateDisplay();
}

void PaginationBar::setCurrentPage(int page)
{
    m_currentPage = qBound(1, page, totalPages());
    updateDisplay();
}

void PaginationBar::refresh()
{
    m_currentPage = 1;
    updateDisplay();
}

void PaginationBar::updateDisplay()
{
    const int pages = totalPages();
    if (m_currentPage > pages) m_currentPage = pages;
    m_label->setText(QString("共 %1 条  第 %2/%3 页").arg(m_totalRecords).arg(m_currentPage).arg(pages));
    m_btnPrev->setEnabled(m_currentPage > 1);
    m_btnNext->setEnabled(m_currentPage < pages);
    m_btnJump->setEnabled(pages > 1);

    const QSignalBlocker blocker(m_pageSpin);
    m_pageSpin->setRange(1, pages);
    m_pageSpin->setValue(m_currentPage);
}

int PaginationBar::totalPages() const
{
    return qMax(1, (m_totalRecords + m_pageSize - 1) / m_pageSize);
}

void PaginationBar::goToPage(int page)
{
    const int targetPage = qBound(1, page, totalPages());
    if (targetPage == m_currentPage) {
        updateDisplay();
        return;
    }

    m_currentPage = targetPage;
    updateDisplay();
    emit pageChanged(m_currentPage);
}
