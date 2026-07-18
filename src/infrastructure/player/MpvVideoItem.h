#pragma once

#include <QQuickFramebufferObject>
#include <QString>
#include <QVariantList>
#include <mpv/client.h>
#include <memory>
#include <mutex>

class MpvVideoItem;

// Thread-safe context shared between MpvVideoItem and MpvRenderer.
// The mutex protects the item pointer so mpv's render callback thread
// never accesses a destroyed MpvVideoItem.
struct MpvCallbackCtx {
    std::mutex mutex;
    MpvVideoItem *item = nullptr;
};

class MpvVideoItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString mediaUrl READ mediaUrl WRITE setMediaUrl NOTIFY mediaUrlChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int startPosition READ startPosition WRITE setStartPosition NOTIFY startPositionChanged)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent NOTIFY userAgentChanged)
    Q_PROPERTY(QString referer READ referer WRITE setReferer NOTIFY refererChanged)
    
    Q_PROPERTY(QVariantList videoTracks READ videoTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY tracksChanged)

public:
    explicit MpvVideoItem(QQuickItem *parent = nullptr);
    ~MpvVideoItem() override;

    Renderer *createRenderer() const override;

    QString mediaUrl() const;
    void setMediaUrl(const QString &url);

    bool isPlaying() const;
    void setPlaying(bool playing);

    bool isLoading() const;

    int duration() const;
    int position() const;
    void setPosition(int position);

    int volume() const;
    void setVolume(int volume);

    // Seconds to seek to once the next file is loaded (one-shot: applied on
    // FILE_LOADED and then reset so later reloads start from the beginning).
    int startPosition() const;
    void setStartPosition(int seconds);

    QString userAgent() const;
    void setUserAgent(const QString &userAgent);

    QString referer() const;
    void setReferer(const QString &referer);

    QVariantList videoTracks() const;
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;

    std::shared_ptr<mpv_handle> mpvHandle() const { return m_mpv_shared; }
    std::shared_ptr<MpvCallbackCtx> callbackCtx() const { return m_callbackCtx; }

public slots:
    void command(const QVariantList &args);
    void setProperty(const QString &name, const QVariant &value);
    void setTrack(const QString &type, int id);

signals:
    void mediaUrlChanged();
    void playingChanged();
    void loadingChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void startPositionChanged();
    void userAgentChanged();
    void refererChanged();
    void tracksChanged();
    void onUpdate();
    // Emitted when a file fails to load/play (network error, 403, bad URL...).
    void playbackFailed(const QString &reason);
    // Emitted when playback reaches the natural end of the file.
    void endReached();

public:
    static void on_mpv_update(void *ctx);
private slots:
    void handleMpvEvent();
    // Invoked (queued) by MpvRenderer once the mpv GL render context exists.
    // Flushes a deferred loadfile — see setMediaUrl().
    void onRenderContextReady();

private:
    static void on_mpv_events(void *ctx);

    void processMpvEvents();
    void updateTrackList();
    void updateHttpHeaders();
    void loadCurrentUrl();

    std::shared_ptr<mpv_handle> m_mpv_shared;
    mpv_handle *m_mpv = nullptr;
    std::shared_ptr<MpvCallbackCtx> m_callbackCtx;
    QString m_mediaUrl;
    // vo=libmpv can't initialize until MpvRenderer creates the render
    // context; loads requested before that are deferred (see setMediaUrl).
    bool m_renderContextReady = false;
    bool m_pendingLoad = false;
    bool m_playing = false;
    bool m_loading = false;
    int m_duration = 0;
    int m_position = 0;
    int m_volume = 100;
    int m_startPosition = 0;
    QString m_userAgent;
    QString m_referer;

    QVariantList m_videoTracks;
    QVariantList m_audioTracks;
    QVariantList m_subtitleTracks;
};
