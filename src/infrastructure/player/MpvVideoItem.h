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
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
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

    int duration() const;
    int position() const;
    void setPosition(int position);

    int volume() const;
    void setVolume(int volume);

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
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void userAgentChanged();
    void refererChanged();
    void tracksChanged();
    void onUpdate();

public:
    static void on_mpv_update(void *ctx);
private slots:
    void handleMpvEvent();

private:
    static void on_mpv_events(void *ctx);

    void processMpvEvents();
    void updateTrackList();

    std::shared_ptr<mpv_handle> m_mpv_shared;
    mpv_handle *m_mpv = nullptr;
    std::shared_ptr<MpvCallbackCtx> m_callbackCtx;
    QString m_mediaUrl;
    bool m_playing = false;
    int m_duration = 0;
    int m_position = 0;
    int m_volume = 100;
    QString m_userAgent;
    QString m_referer;

    QVariantList m_videoTracks;
    QVariantList m_audioTracks;
    QVariantList m_subtitleTracks;
};
