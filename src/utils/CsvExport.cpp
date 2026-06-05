#include "CsvExport.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QStringConverter>

struct CsvData {
    QStringList headers;
    QList<QStringList> rows;
};

static CsvData serializeModelData(QAbstractItemModel *model, const QList<int> &skipCols)
{
    CsvData data;
    if (!model) return data;

    // Collect active columns to export
    QList<int> exportCols;
    const int cc = model->columnCount();
    for (int c = 0; c < cc; c++) {
        if (!skipCols.contains(c)) {
            exportCols.append(c);
        }
    }

    // Read headers
    for (int c : exportCols) {
        data.headers.append(model->headerData(c, Qt::Horizontal).toString());
    }

    // Read rows
    const int rc = model->rowCount();
    for (int r = 0; r < rc; r++) {
        QStringList rowData;
        for (int c : exportCols) {
            rowData.append(model->data(model->index(r, c)).toString());
        }
        data.rows.append(rowData);
    }

    return data;
}

static bool writeCsvDataToFile(const CsvData &data, const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    if (f.write("\xEF\xBB\xBF", 3) != 3) {
        f.close();
        return false;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    // Write headers
    for (int i = 0; i < data.headers.size(); i++) {
        QString item = data.headers[i];
        out << "\"" << item.replace("\"", "\"\"") << "\"";
        if (i < data.headers.size() - 1) {
            out << ",";
        } else {
            out << "\n";
        }
    }

    // Write rows
    for (const auto &row : data.rows) {
        for (int i = 0; i < row.size(); i++) {
            QString item = row[i];
            out << "\"" << item.replace("\"", "\"\"") << "\"";
            if (i < row.size() - 1) {
                out << ",";
            } else {
                out << "\n";
            }
        }
    }
    f.close();
    return true;
}

void exportModelToCSV(QAbstractItemModel *model, const QString &filePath,
                      const QList<int> &skipCols)
{
    CsvData data = serializeModelData(model, skipCols);
    if (writeCsvDataToFile(data, filePath)) {
        QMessageBox::information(nullptr, "导出成功", "数据已导出至:\n" + filePath);
    } else {
        QMessageBox::warning(nullptr, "导出失败", "无法写入文件:\n" + filePath);
    }
}

void exportModelToCSVAsync(QAbstractItemModel *model, const QString &filePath,
                           QWidget *parent, const QList<int> &skipCols)
{
    if (filePath.isEmpty() || !model) return;

    CsvData csvData = serializeModelData(model, skipCols);
    exportRowsToCSVAsync(csvData.headers, csvData.rows, filePath, parent);
}

void exportRowsToCSVAsync(const QStringList &headers, const QList<QStringList> &rows,
                          const QString &filePath, QWidget *parent)
{
    if (filePath.isEmpty()) return;

    CsvData csvData;
    csvData.headers = headers;
    csvData.rows = rows;

    auto *progress = new QProgressDialog("正在导出数据，请稍候...", QString(), 0, 0, parent);
    progress->setWindowTitle("正在导出");
    progress->setWindowModality(Qt::WindowModal);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    auto *watcher = new QFutureWatcher<bool>(parent);

    QObject::connect(watcher, &QFutureWatcher<bool>::finished, [watcher, progress, filePath, parent]() {
        progress->close();
        bool success = watcher->result();
        if (success) {
            QMessageBox::information(parent, "导出成功", "数据已成功导出至:\n" + filePath);
        } else {
            QMessageBox::critical(parent, "导出失败", "写入文件失败:\n" + filePath);
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([csvData, filePath]() {
        return writeCsvDataToFile(csvData, filePath);
    });
    watcher->setFuture(future);
}
