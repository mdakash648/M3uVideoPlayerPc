#include "PosterFetchService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

namespace Infrastructure {

PosterFetchService::PosterFetchService(SettingsManager* settings, Data::ChannelRepository* channelRepo, QObject* parent)
    : QObject(parent), m_settings(settings), m_channelRepo(channelRepo),
      m_network(new QNetworkAccessManager(this))
{
}

QString PosterFetchService::extractGroupTitle(const QString& raw) {
    QString s = raw;
    s.remove(QRegularExpression("^\\[[^\\]]*\\]"));
    s.remove(QRegularExpression("^\\([^)]*\\)"));
    s = s.trimmed();
    s.replace(QRegularExpression("[._]"), " ");
    s = s.trimmed();

    static const QList<QRegularExpression> cutPatterns = {
        QRegularExpression("\\bS\\d{1,2}E\\d{1,3}\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bS\\d{1,2}\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bE\\d{2,3}\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\b\\d{3,4}[pP]\\b"),
        QRegularExpression("\\b(WEB[-\\s]?DL|HDTV|BluRay|BRRip|WEBRip|AMZN|NF|DSNP|HMAX|MX|SonyLIV|Hotstar|ZEE5)\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\b(AAC|DDP|AC3|DD5|x264|x265|HEVC|AVC|H\\.?264|H\\.?265|mkv|mp4|avi)\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\b(Episode|Ep)\\s*\\d+\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bPart\\s*\\d+\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bVol\\.?\\s*\\d+\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bComplete\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bDual\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bMulti\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bAudio\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bSeason\\s*\\d+\\b", QRegularExpression::CaseInsensitiveOption),
    };

    int cutIndex = s.length();
    for (const auto& pat : cutPatterns) {
        auto m = pat.match(s);
        if (m.hasMatch() && m.capturedStart() < cutIndex) cutIndex = m.capturedStart();
    }

    s = s.left(cutIndex);
    s.remove(QRegularExpression("[-.\\s]+$"));
    s = s.trimmed();

    QString titled = s;
    bool capitalizeNext = true;
    for (int i = 0; i < titled.length(); ++i) {
        const QChar c = titled.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
            if (capitalizeNext) {
                titled[i] = c.toUpper();
                capitalizeNext = false;
            }
        } else {
            capitalizeNext = true;
        }
    }

    return titled.isEmpty() ? raw : titled;
}

PosterFetchService::CleanTitle PosterFetchService::cleanTitleForSearch(const QString& raw) {
    QString s = extractGroupTitle(raw);
    QString year;
    QRegularExpression yearRe("\\b(19\\d{2}|20\\d{2})\\b");
    auto m = yearRe.match(s);
    if (m.hasMatch()) {
        year = m.captured(1);
        s = s.left(m.capturedStart()).trimmed();
    }
    s.remove(QRegularExpression("[\\(\\[\\-.\\s]+$"));
    s = s.trimmed();

    CleanTitle result;
    result.name = s.isEmpty() ? raw : s;
    result.year = year;
    return result;
}

QString PosterFetchService::cleanContentName(QString name) {
    name.remove(QRegularExpression("\\.(m3u8?|mkv|mp4|avi|flv|webm)$", QRegularExpression::CaseInsensitiveOption));
    name.remove(QRegularExpression("\\[.*?\\]"));
    name.remove(QRegularExpression("\\{.*?\\}"));
    name.replace(QRegularExpression("[.]"), " ");
    name.replace(QRegularExpression("-"), " ");
    name.remove(QRegularExpression("\\((?!\\d{4}\\))[^)]*\\)"));
    name = name.simplified();
    return name;
}

void PosterFetchService::fetchPostersForPlaylist(int playlistId, bool onlyMissing) {
    if (!m_channelRepo) return;
    if (m_settings && m_settings->omdbApiKey().trimmed().isEmpty()) {
        m_statusText = QStringLiteral("OMDb API key দেওয়া নেই — Settings-এ গিয়ে key বসান");
        emit progressChanged();
        return;
    }

    auto channels = m_channelRepo->getChannelsByPlaylistId(playlistId);

    QHash<QString, int> keyToIndex;
    QList<SeriesGroup> groups;

    for (const auto& ch : channels) {
        if (ch.type != Domain::ContentType::MOVIE && ch.type != Domain::ContentType::SERIES) continue;
        if (onlyMissing && !ch.logoUrl.trimmed().isEmpty()) continue;

        const CleanTitle clean = cleanTitleForSearch(cleanContentName(ch.name));
        QString key = clean.name.trimmed().toLower();
        if (key.isEmpty()) key = ch.name.trimmed().toLower();

        auto it = keyToIndex.find(key);
        if (it == keyToIndex.end()) {
            SeriesGroup g;
            g.displayTitle = ch.name;
            g.channels.append(ch);
            groups.append(g);
            keyToIndex[key] = groups.size() - 1;
        } else {
            groups[it.value()].channels.append(ch);
        }
    }

    if (groups.isEmpty()) return;

    if (!m_running && m_jobs.isEmpty() && !m_hasCurrentJob) {
        m_totalGroups = 0;
        m_doneGroups = 0;
        m_foundCount = 0;
    }

    Job job;
    job.playlistId = playlistId;
    job.groups = groups;
    m_jobs.enqueue(job);

    m_totalGroups += groups.size();
    emit progressChanged();

    startNextJobIfIdle();
}

void PosterFetchService::fetchPosterByImdbId(int channelId, const QString& imdbId) {
    if (!m_channelRepo || !m_settings) return;
    const QString apiKey = m_settings->omdbApiKey().trimmed();
    const QString id = imdbId.trimmed();
    if (apiKey.isEmpty() || id.isEmpty()) return;

    QUrl url("https://www.omdbapi.com/");
    QUrlQuery q;
    q.addQueryItem("apikey", apiKey);
    q.addQueryItem("i", id);
    url.setQuery(q);

    QNetworkReply* reply = m_network->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, channelId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.value("Response").toString() != "True") return;
        const QString poster = obj.value("Poster").toString();
        if (poster.isEmpty() || poster == "N/A") return;

        auto chOpt = m_channelRepo->getChannelById(channelId);
        if (!chOpt) return;
        Domain::Channel ch = *chOpt;
        ch.logoUrl = poster;
        m_channelRepo->updateChannel(ch);
        emit postersUpdated(ch.playlistId);
    });
}

void PosterFetchService::stop() {
    m_stopRequested = true;
    m_jobs.clear();
}

void PosterFetchService::startNextJobIfIdle() {
    if (m_running) return;
    if (m_hasCurrentJob) return;
    if (m_jobs.isEmpty()) {
        if (m_running) {
            m_running = false;
            emit runningChanged();
        }
        return;
    }

    m_currentJob = m_jobs.dequeue();
    m_hasCurrentJob = true;
    m_stopRequested = false;
    m_running = true;
    emit runningChanged();

    processNextGroup();
}

void PosterFetchService::processNextGroup() {
    if (m_stopRequested) {
        m_hasCurrentJob = false;
        m_running = false;
        m_jobs.clear();
        emit runningChanged();
        return;
    }

    if (!m_hasCurrentJob) {
        startNextJobIfIdle();
        return;
    }

    if (m_currentJob.currentIndex >= m_currentJob.groups.size()) {
        emit batchFinished(m_currentJob.playlistId, m_currentJob.found, m_currentJob.groups.size());
        m_hasCurrentJob = false;
        m_running = false;
        emit runningChanged();
        startNextJobIfIdle();
        return;
    }

    const SeriesGroup& group = m_currentJob.groups[m_currentJob.currentIndex];
    const Domain::Channel& repChannel = group.channels.first();

    const CleanTitle clean = cleanTitleForSearch(cleanContentName(repChannel.name));
    m_attemptName = clean.name;
    m_attemptYear = clean.year;

    const bool isSeries = repChannel.type == Domain::ContentType::SERIES;
    const QString primaryType = isSeries ? "series" : "movie";
    const QString altType = isSeries ? "movie" : "series";

    m_attempts.clear();
    m_attempts.append({primaryType, true});
    m_attempts.append({primaryType, false});
    m_attempts.append({altType, true});
    m_attempts.append({altType, false});
    m_attempts.append({QString(), false});

    m_statusText = QStringLiteral("🔎 খোঁজা হচ্ছে (%1/%2): %3")
                       .arg(m_doneGroups + 1)
                       .arg(m_totalGroups)
                       .arg(group.displayTitle);
    emit progressChanged();

    tryFetchAttempt(0);
}

void PosterFetchService::tryFetchAttempt(int attemptIndex) {
    if (m_stopRequested) {
        processNextGroup();
        return;
    }
    if (attemptIndex >= m_attempts.size()) {
        finishGroup(QString());
        return;
    }

    const Attempt& attempt = m_attempts[attemptIndex];
    const QString apiKey = m_settings ? m_settings->omdbApiKey().trimmed() : QString();
    if (apiKey.isEmpty() || m_attemptName.isEmpty()) {
        finishGroup(QString());
        return;
    }

    QUrl url("https://www.omdbapi.com/");
    QUrlQuery q;
    q.addQueryItem("apikey", apiKey);
    q.addQueryItem("t", m_attemptName);
    if (!attempt.type.isEmpty()) q.addQueryItem("type", attempt.type);
    if (attempt.useYear && !m_attemptYear.isEmpty()) q.addQueryItem("y", m_attemptYear);
    url.setQuery(q);

    QNetworkReply* reply = m_network->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, attemptIndex]() {
        reply->deleteLater();
        if (m_stopRequested) { processNextGroup(); return; }

        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            if (obj.value("Response").toString() == "True") {
                const QString poster = obj.value("Poster").toString();
                if (!poster.isEmpty() && poster != "N/A") {
                    finishGroup(poster);
                    return;
                }
            }
        }
        tryFetchAttempt(attemptIndex + 1);
    });
}

void PosterFetchService::finishGroup(const QString& posterUrl) {
    SeriesGroup& group = m_currentJob.groups[m_currentJob.currentIndex];

    if (!posterUrl.isEmpty()) {
        for (auto& ch : group.channels) {
            ch.logoUrl = posterUrl;
            if (m_channelRepo) m_channelRepo->updateChannel(ch);
        }
        m_currentJob.found++;
        m_foundCount++;
        emit postersUpdated(m_currentJob.playlistId);
    }

    m_doneGroups++;
    m_currentJob.currentIndex++;
    emit progressChanged();

    QTimer::singleShot(400, this, [this]() {
        processNextGroup();
    });
}

} // namespace Infrastructure
