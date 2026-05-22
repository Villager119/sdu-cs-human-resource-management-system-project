#include "CsvExport.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QWidget>

void exportModelToCSV(QAbstractItemModel *model, const QString &filePath,
                      const QList<int> &skipCols)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "导出失败", "无法写入文件:\n" + filePath);
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";

    const int cc = model->columnCount();
    for (int c = 0; c < cc; c++) {
        if (skipCols.contains(c)) continue;
        out << "\"" << model->headerData(c, Qt::Horizontal).toString() << "\""
            << (c < cc - 1 ? "," : "\n");
    }
    for (int r = 0; r < model->rowCount(); r++) {
        for (int c = 0; c < cc; c++) {
            if (skipCols.contains(c)) continue;
            out << "\"" << model->data(model->index(r, c)).toString().replace("\"", "\"\"") << "\""
                << (c < cc - 1 ? "," : "\n");
        }
    }
    f.close();
    QMessageBox::information(nullptr, "导出成功", "数据已导出至:\n" + filePath);
}
