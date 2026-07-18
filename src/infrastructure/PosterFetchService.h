#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQueue>
#include <QHash>
#include <QNetworkAccessManager>
#include "../domain/Channel.h"
#include "../data/ChannelRepository.h"
#include "SettingsManager.h"

namespace Infrastructure {

class PosterFetchService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY progressChanged)
    Q_PROPERTY(int doneCount READ doneCount NOTIFY progressChanged)
    Q_PROPERTY(int foundCount READ foundCount NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)

public:
    explicit PosterFetchService(SettingsManager* settings, Data::ChannelRepository* channelRepo, QObject* parent = nullptr);

    bool running() const { return m_running; }
    int totalCount() const { return m_totalGroups; }
    int doneCount() const { return m_doneGroups; }
    int foundCount() const { return m_foundCount; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void fetchPostersForPlaylist(int playlistId, bool onlyMissing = true);
    Q_INVOKABLE void fetchPosterByImdbId(int channelId, const QString& imdbId);
    Q_INVOKABLE void stop();

signals:
    void runningChanged();
    void progressChanged();
    void postersUpdated(int playlistId);
    void batchFinished(int playlistId, int found, int total);

private:
    struct SeriesGroup {
        QString displayTitle;
        QList<Domain::Channel> channels;
    };
    struct Job {
        int playlistId = 0;
        QList<SeriesGroup> groups;
        int currentIndex = 0;
        int found = 0;
    };
    struct CleanTitle {
        QString name;
        QString year;
    };
    struct Attempt {
        QString type;
        bool useYear;
    };

    static QString extractGroupTitle(const QString& raw);
    static CleanTitle cleanTitleForSearch(const QString& raw);
    static QString cleanContentName(QString name);

    void startNextJobIfIdle();
    void processNextGroup();
    void tryFetchAttempt(int attemptIndex);
    void finishGroup(const QString& posterUrl);

    SettingsManager* m_settings;
    Data::ChannelRepository* m_channelRepo;
    QNetworkAccessManager* m_network;

    QQueue<Job> m_jobs;
    bool m_hasCurrentJob = false;
    Job m_currentJob;

    bool m_running = false;
    bool m_stopRequested = false;
    int m_totalGroups = 0;
    int m_doneGroups = 0;
    int m_foundCount = 0;
    QString m_statusText;

    QList<Attempt> m_attempts;
    QString m_attemptName;
    QString m_attemptYear;
};

} // namespace Infrastructure
