#include "SettingsManager.h"
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <algorithm>

namespace Infrastructure {

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent),
      // Default QSettings location — same org/app scope as the QML Settings
      // items (organization/name are set in main.cpp).
      m_network(new QNetworkAccessManager(this))
{
    m_gridColumns = m_store.value("Appearance/gridColumns", 0).toInt();
    m_posterColumns = m_store.value("Appearance/posterColumns", 0).toInt();
    m_controllerTimeout = m_store.value("Player/controllerTimeout", 3.0).toDouble();
    m_timeZoneAuto = m_store.value("TimeZone/auto", true).toBool();
    m_timeZoneId = m_store.value("TimeZone/id", QString()).toString();
    m_omdbApiKey = m_store.value("Poster/omdbApiKey", QString()).toString();
    m_autoPosterFetchEnabled = m_store.value("Poster/autoFetchEnabled", true).toBool();
}

void SettingsManager::setGridColumns(int columns) {
    columns = std::clamp(columns, 0, 8);
    if (columns == m_gridColumns) return;
    m_gridColumns = columns;
    m_store.setValue("Appearance/gridColumns", columns);
    emit gridColumnsChanged();
}

void SettingsManager::setPosterColumns(int columns) {
    columns = std::clamp(columns, 0, 8);
    if (columns == m_posterColumns) return;
    m_posterColumns = columns;
    m_store.setValue("Appearance/posterColumns", columns);
    emit posterColumnsChanged();
}

void SettingsManager::setControllerTimeout(double seconds) {
    // Range 0.5s .. 10s in 0.5s steps
    seconds = std::clamp(seconds, 0.5, 10.0);
    seconds = qRound(seconds * 2.0) / 2.0;
    if (qFuzzyCompare(seconds, m_controllerTimeout)) return;
    m_controllerTimeout = seconds;
    m_store.setValue("Player/controllerTimeout", seconds);
    emit controllerTimeoutChanged();
}

void SettingsManager::setTimeZoneAuto(bool automatic) {
    if (automatic == m_timeZoneAuto) return;
    m_timeZoneAuto = automatic;
    m_store.setValue("TimeZone/auto", automatic);
    emit timeZoneChanged();
}

void SettingsManager::setTimeZoneId(const QString& ianaId) {
    if (ianaId == m_timeZoneId) return;
    if (!ianaId.isEmpty() && !QTimeZone(ianaId.toUtf8()).isValid()) return;
    m_timeZoneId = ianaId;
    m_store.setValue("TimeZone/id", ianaId);
    emit timeZoneChanged();
}

void SettingsManager::setOmdbApiKey(const QString& key) {
    // Users sometimes paste the full OMDb URL instead of just the key.
    QString cleaned = key.trimmed();
    const QRegularExpression re("[?&]apikey=([^&\\s]+)", QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(cleaned);
    if (match.hasMatch()) cleaned = match.captured(1);
    cleaned = cleaned.trimmed();

    if (cleaned == m_omdbApiKey) return;
    m_omdbApiKey = cleaned;
    m_store.setValue("Poster/omdbApiKey", cleaned);
    emit omdbApiKeyChanged();
}

void SettingsManager::setAutoPosterFetchEnabled(bool enabled) {
    if (enabled == m_autoPosterFetchEnabled) return;
    m_autoPosterFetchEnabled = enabled;
    m_store.setValue("Poster/autoFetchEnabled", enabled);
    emit autoPosterFetchEnabledChanged();
}

QTimeZone SettingsManager::effectiveTimeZone() const {
    if (!m_timeZoneAuto && !m_timeZoneId.isEmpty()) {
        QTimeZone tz(m_timeZoneId.toUtf8());
        if (tz.isValid()) return tz;
    }
    return QTimeZone::systemTimeZone();
}

QString SettingsManager::offsetLabel(const QTimeZone& tz) {
    const int secs = tz.offsetFromUtc(QDateTime::currentDateTimeUtc());
    const int totalMin = qAbs(secs) / 60;
    return QString("UTC%1%2:%3")
        .arg(secs < 0 ? "-" : "+")
        .arg(totalMin / 60, 2, 10, QChar('0'))
        .arg(totalMin % 60, 2, 10, QChar('0'));
}

QString SettingsManager::timeZoneLabel() const {
    const QTimeZone tz = effectiveTimeZone();
    const QString base = offsetLabel(tz) + QStringLiteral(" — ") + QString::fromUtf8(tz.id());
    return m_timeZoneAuto ? base + QStringLiteral(" (Auto)") : base;
}

QVariantList SettingsManager::availableTimeZones() const {
    struct Entry { int offset; QString id; QString label; };
    QList<Entry> entries;

    const auto ids = QTimeZone::availableTimeZoneIds();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    entries.reserve(ids.size());
    for (const QByteArray& id : ids) {
        QTimeZone tz(id);
        if (!tz.isValid()) continue;
        Entry e;
        e.offset = tz.offsetFromUtc(now);
        e.id = QString::fromUtf8(id);
        e.label = offsetLabel(tz) + QStringLiteral(" — ") + e.id;
        entries.push_back(e);
    }

    // Sort by UTC offset first (so "utc-10, utc-9, ..." scan naturally), then name
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.offset != b.offset) return a.offset < b.offset;
        return a.id < b.id;
    });

    QVariantList list;
    list.reserve(entries.size());
    for (const auto& e : entries) {
        QVariantMap m;
        m["id"] = e.id;
        m["label"] = e.label;
        list.push_back(m);
    }
    return list;
}

void SettingsManager::autoDetectTimeZone() {
    // IP-based geolocation: independent of the PC's (possibly wrong) clock
    // and time zone settings.
    QNetworkRequest request(QUrl("http://ip-api.com/json/?fields=status,timezone"));
    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit autoDetectFinished(false, QString());
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString zone = obj.value("timezone").toString();
        if (obj.value("status").toString() != "success" || zone.isEmpty()
            || !QTimeZone(zone.toUtf8()).isValid()) {
            emit autoDetectFinished(false, QString());
            return;
        }
        setTimeZoneAuto(false);
        setTimeZoneId(zone);
        emit autoDetectFinished(true, zone);
    });
}

QVariantMap SettingsManager::toMap() const {
    QVariantMap m;
    m["gridColumns"] = m_gridColumns;
    m["posterColumns"] = m_posterColumns;
    m["controllerTimeout"] = m_controllerTimeout;
    m["timeZoneAuto"] = m_timeZoneAuto;
    m["timeZoneId"] = m_timeZoneId;
    m["autoPosterFetchEnabled"] = m_autoPosterFetchEnabled;
    return m;
}

void SettingsManager::applyMap(const QVariantMap& map) {
    if (map.contains("gridColumns")) setGridColumns(map["gridColumns"].toInt());
    if (map.contains("posterColumns")) setPosterColumns(map["posterColumns"].toInt());
    if (map.contains("controllerTimeout")) setControllerTimeout(map["controllerTimeout"].toDouble());
    if (map.contains("timeZoneId")) setTimeZoneId(map["timeZoneId"].toString());
    if (map.contains("timeZoneAuto")) setTimeZoneAuto(map["timeZoneAuto"].toBool());
    if (map.contains("autoPosterFetchEnabled")) setAutoPosterFetchEnabled(map["autoPosterFetchEnabled"].toBool());
}

} // namespace Infrastructure
