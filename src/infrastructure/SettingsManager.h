#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTimeZone>
#include <QNetworkAccessManager>

namespace Infrastructure {

// Central, QSettings-backed app settings exposed to QML through
// AppController.settings. Every setter persists immediately so the values
// survive restarts (stored under the app's organization/name registry keys).
class SettingsManager : public QObject {
    Q_OBJECT
    // 0 = Auto (responsive default of 4 per row); 2..8 = fixed column count
    Q_PROPERTY(int gridColumns READ gridColumns WRITE setGridColumns NOTIFY gridColumnsChanged)
    // 0 = Auto (default 4 per row in poster view); 2..8 = fixed column count
    Q_PROPERTY(int posterColumns READ posterColumns WRITE setPosterColumns NOTIFY posterColumnsChanged)
    // Seconds the player controls stay visible before auto-hiding (0.5 .. 10, step 0.5)
    Q_PROPERTY(double controllerTimeout READ controllerTimeout WRITE setControllerTimeout NOTIFY controllerTimeoutChanged)
    // true = follow auto-detected/PC time zone, false = manual IANA id below
    Q_PROPERTY(bool timeZoneAuto READ timeZoneAuto WRITE setTimeZoneAuto NOTIFY timeZoneChanged)
    Q_PROPERTY(QString timeZoneId READ timeZoneId WRITE setTimeZoneId NOTIFY timeZoneChanged)
    // Human readable label of the effective zone, e.g. "UTC+06:00 — Asia/Dhaka"
    Q_PROPERTY(QString timeZoneLabel READ timeZoneLabel NOTIFY timeZoneChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    int gridColumns() const { return m_gridColumns; }
    void setGridColumns(int columns);

    int posterColumns() const { return m_posterColumns; }
    void setPosterColumns(int columns);

    double controllerTimeout() const { return m_controllerTimeout; }
    void setControllerTimeout(double seconds);

    bool timeZoneAuto() const { return m_timeZoneAuto; }
    void setTimeZoneAuto(bool automatic);

    QString timeZoneId() const { return m_timeZoneId; }
    void setTimeZoneId(const QString& ianaId);

    QString timeZoneLabel() const;

    // The zone all app time calculations should use (manual pick, or the
    // system zone while in auto mode).
    QTimeZone effectiveTimeZone() const;

    // Full IANA zone list with UTC offsets for the picker:
    // [{ id: "Asia/Dhaka", label: "UTC+06:00 — Asia/Dhaka" }, ...]
    Q_INVOKABLE QVariantList availableTimeZones() const;

    // Looks the zone up from the public IP's geolocation (works even when the
    // PC's own zone/clock is wrong). Answers via autoDetectFinished().
    Q_INVOKABLE void autoDetectTimeZone();

    // Everything as a JSON-ready map — used by Backup & Restore.
    QVariantMap toMap() const;
    void applyMap(const QVariantMap& map);

signals:
    void gridColumnsChanged();
    void posterColumnsChanged();
    void controllerTimeoutChanged();
    void timeZoneChanged();
    void autoDetectFinished(bool success, const QString& zoneId);

private:
    static QString offsetLabel(const QTimeZone& tz);

    QSettings m_store;
    QNetworkAccessManager* m_network;

    int m_gridColumns = 0;          // 0 = auto
    int m_posterColumns = 0;        // 0 = auto
    double m_controllerTimeout = 3.0;
    bool m_timeZoneAuto = true;
    QString m_timeZoneId;
};

} // namespace Infrastructure
