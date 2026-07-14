#pragma once

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QString>

namespace Data {

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool init(const QString &dbPath = "m3uplayer.db");
    QSqlDatabase getDatabase() const;

private:
    bool createTables();

    QSqlDatabase m_db;
};

} // namespace Data
