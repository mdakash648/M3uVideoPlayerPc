#include "MpvVideoItem.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QThread>
#include <mpv/render_gl.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <QVariantMap>
#include <QSet>

namespace {
// Fallback UA for playlist entries that don't specify one. Many CDNs return
// 403 for mpv's default "libmpv" user agent, so pretend to be a browser.
constexpr const char *kDefaultUserAgent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";

void *get_proc_address_mpv(void *fn_ctx, const char *name) {
    Q_UNUSED(fn_ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return nullptr;
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}
} // namespace

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    MpvRenderer(MpvVideoItem *item)
        : m_window(item->window())
        , m_mpv_shared(item->mpvHandle())
        , m_callbackCtx(item->callbackCtx())
        , m_mpv_gl(nullptr)
    {
        mpv_opengl_init_params gl_init_params{get_proc_address_mpv, nullptr};
        int advanced_control = 1;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
            {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        if (mpv_render_context_create(&m_mpv_gl, m_mpv_shared.get(), params) < 0) {
            std::cerr << "Failed to initialize mpv GL context" << std::endl;
        }

        // The update callback fires from mpv's render thread.
        // We pass the shared MpvCallbackCtx which is protected by a mutex.
        mpv_render_context_set_update_callback(m_mpv_gl, MpvVideoItem::on_mpv_update, m_callbackCtx.get());

        // Tell the item (on the GUI thread) that the render context now
        // exists, so a deferred loadfile can be issued. Starting playback
        // BEFORE this point makes mpv's VO init fail ("No render context
        // set") and the file plays audio-only with a black screen.
        std::lock_guard<std::mutex> lock(m_callbackCtx->mutex);
        if (m_callbackCtx->item) {
            QMetaObject::invokeMethod(m_callbackCtx->item, "onRenderContextReady", Qt::QueuedConnection);
        }
    }

    ~MpvRenderer() override {
        if (m_mpv_gl) {
            mpv_render_context_free(m_mpv_gl);
        }
    }

    void render() override {
        if (!m_mpv_gl) return;

        QOpenGLFramebufferObject *fbo = framebufferObject();
        if (!fbo) return;

        m_window->beginExternalCommands();

        mpv_opengl_fbo mpfbo;
        mpfbo.fbo = fbo->handle();
        mpfbo.w = fbo->width();
        mpfbo.h = fbo->height();
        mpfbo.internal_format = 0;

        int flip_y = 0;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_mpv_gl, params);
        m_window->endExternalCommands();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    QQuickWindow *m_window;
    std::shared_ptr<mpv_handle> m_mpv_shared;
    std::shared_ptr<MpvCallbackCtx> m_callbackCtx;
    mpv_render_context *m_mpv_gl;
};

MpvVideoItem::MpvVideoItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    mpv_handle *mpv = mpv_create();
    if (!mpv) {
        throw std::runtime_error("could not create mpv context");
    }
    m_mpv_shared = std::shared_ptr<mpv_handle>(mpv, [](mpv_handle *h) {
        mpv_terminate_destroy(h);
    });
    m_mpv = m_mpv_shared.get();

    // Create the thread-safe callback context
    m_callbackCtx = std::make_shared<MpvCallbackCtx>();
    m_callbackCtx->item = this;

    // Default configuration for better performance and Qt compatibility.
    // hwdec MUST be a copy-back mode ("auto-copy-safe"), not "auto": plain
    // auto picks D3D11VA on Windows, whose frames can't be shown through the
    // desktop-OpenGL mpv_render_context we use — decoding "works" but the
    // video stays black (audio only, and subtitles die with the video since
    // they're drawn onto the frames). Copy-back decodes on the GPU, then
    // copies frames to system memory where the GL renderer can always use
    // them.
    mpv_set_option_string(m_mpv, "hwdec", "auto-copy-safe");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "audio-channels", "auto-safe");

    // Precise seeking: without this, relative seeks (+10s) snap to the
    // nearest keyframe, which in files with sparse keyframes can land right
    // back at the pre-seek position ("+10s jumps back" bug).
    mpv_set_option_string(m_mpv, "hr-seek", "yes");

    // Network cache: lets mpv seek within already-downloaded data instantly
    // and keeps recently played data for backwards seeks.
    mpv_set_option_string(m_mpv, "cache", "yes");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", "256MiB");
    mpv_set_option_string(m_mpv, "demuxer-max-back-bytes", "128MiB");

    // Allow volume up to 200% (default is 130)
    mpv_set_option_string(m_mpv, "volume-max", "200");

    if (mpv_initialize(m_mpv) < 0) {
        throw std::runtime_error("could not initialize mpv context");
    }

    // Observe properties
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "track-list", MPV_FORMAT_NONE);
    mpv_observe_property(m_mpv, 0, "paused-for-cache", MPV_FORMAT_FLAG);

    mpv_set_wakeup_callback(m_mpv, on_mpv_events, this);

    connect(this, &MpvVideoItem::onUpdate, this, [this]() {
        update();
    }, Qt::QueuedConnection);
}

MpvVideoItem::~MpvVideoItem() {
    // FIRST: Atomically invalidate the callback context so mpv's render thread
    // can never touch 'this' again after this lock is released.
    {
        std::lock_guard<std::mutex> lock(m_callbackCtx->mutex);
        m_callbackCtx->item = nullptr;
    }

    if (m_mpv) {
        // Stop the wakeup callback
        mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);

        // Stop playback so mpv stops generating new events/frames
        const char *cmd[] = {"stop", nullptr};
        mpv_command(m_mpv, cmd);

        // Drain any already-queued mpv events
        while (true) {
            mpv_event *ev = mpv_wait_event(m_mpv, 0);
            if (ev->event_id == MPV_EVENT_NONE)
                break;
        }

        m_mpv = nullptr;
    }
}

QQuickFramebufferObject::Renderer *MpvVideoItem::createRenderer() const {
    window()->setPersistentSceneGraph(true);
    return new MpvRenderer(const_cast<MpvVideoItem *>(this));
}

void MpvVideoItem::on_mpv_events(void *ctx) {
    QMetaObject::invokeMethod(static_cast<MpvVideoItem *>(ctx), "handleMpvEvent", Qt::QueuedConnection);
}

// Called from mpv's render thread. Uses mutex-protected context to safely
// emit the onUpdate signal only if the MpvVideoItem is still alive.
void MpvVideoItem::on_mpv_update(void *ctx) {
    auto *cbCtx = static_cast<MpvCallbackCtx *>(ctx);
    std::lock_guard<std::mutex> lock(cbCtx->mutex);
    if (cbCtx->item) {
        emit cbCtx->item->onUpdate();
    }
}

void MpvVideoItem::handleMpvEvent() {
    while (m_mpv) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;

        switch (event->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property *prop = static_cast<mpv_event_property *>(event->data);
                if (QString(prop->name) == "time-pos" && prop->format == MPV_FORMAT_DOUBLE) {
                    m_position = static_cast<int>(*static_cast<double *>(prop->data));
                    emit positionChanged();
                } else if (QString(prop->name) == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
                    m_duration = static_cast<int>(*static_cast<double *>(prop->data));
                    emit durationChanged();
                } else if (QString(prop->name) == "pause" && prop->format == MPV_FORMAT_FLAG) {
                    m_playing = !(*static_cast<int *>(prop->data));
                    emit playingChanged();
                } else if (QString(prop->name) == "volume" && prop->format == MPV_FORMAT_DOUBLE) {
                    m_volume = static_cast<int>(*static_cast<double *>(prop->data));
                    emit volumeChanged();
                } else if (QString(prop->name) == "track-list") {
                    updateTrackList();
                } else if (QString(prop->name) == "paused-for-cache" && prop->format == MPV_FORMAT_FLAG) {
                    bool buffering = (*static_cast<int *>(prop->data)) != 0;
                    if (m_loading != buffering) {
                        m_loading = buffering;
                        emit loadingChanged();
                    }
                }
                break;
            }
            case MPV_EVENT_START_FILE: {
                if (!m_loading) {
                    m_loading = true;
                    emit loadingChanged();
                }
                break;
            }
            case MPV_EVENT_FILE_LOADED:
                // One-shot resume seek: applied only when the file is fully
                // loaded (seeking earlier via time-pos is silently dropped).
                if (m_startPosition > 0) {
                    double val = m_startPosition;
                    mpv_set_property(m_mpv, "time-pos", MPV_FORMAT_DOUBLE, &val);
                    m_startPosition = 0;
                    emit startPositionChanged();
                }
                Q_FALLTHROUGH();
            case MPV_EVENT_PLAYBACK_RESTART: {
                if (m_loading) {
                    m_loading = false;
                    emit loadingChanged();
                }
                break;
            }
            case MPV_EVENT_END_FILE: {
                mpv_event_end_file *ef = static_cast<mpv_event_end_file *>(event->data);
                if (m_loading) {
                    m_loading = false;
                    emit loadingChanged();
                }
                // MPV_END_FILE_REASON_ERROR covers network failures (403,
                // timeouts, unsupported format...). Let QML decide fallback.
                if (ef && ef->reason == MPV_END_FILE_REASON_ERROR) {
                    emit playbackFailed(QString::fromUtf8(mpv_error_string(ef->error)));
                } else if (ef && ef->reason == MPV_END_FILE_REASON_EOF) {
                    // Natural end of the video — used by the resume system to
                    // clear the saved position and auto-play the next item.
                    emit endReached();
                }
                break;
            }
            default:
                break;
        }
    }
}

QString MpvVideoItem::mediaUrl() const {
    return m_mediaUrl;
}

void MpvVideoItem::setMediaUrl(const QString &url) {
    if (m_mediaUrl != url) {
        m_mediaUrl = url;

        // Apply Referer / User-Agent on the mpv handle BEFORE loading the
        // file. Per-file loadfile options are unreliable across mpv versions
        // (>= 0.38 changed the positional arguments), and header values may
        // contain commas which break the options list syntax.
        updateHttpHeaders();

        if (m_renderContextReady) {
            loadCurrentUrl();
        } else {
            // The QML scene graph hasn't created the MpvRenderer (and thus
            // the mpv_render_context) yet. Issuing loadfile now would make
            // vo=libmpv fail to initialize ("No render context set") and the
            // whole file would play audio-only on a black screen. This bites
            // local files (mkv especially) because they open faster than the
            // first render pass; the load is flushed in onRenderContextReady.
            m_pendingLoad = true;
        }

        emit mediaUrlChanged();
    }
}

void MpvVideoItem::loadCurrentUrl() {
    // Keep the QByteArray alive for the duration of the command —
    // url.toUtf8().constData() inline would leave a dangling pointer.
    const QByteArray urlUtf8 = m_mediaUrl.toUtf8();
    const char *args[] = {"loadfile", urlUtf8.constData(), nullptr};
    mpv_command(m_mpv, args);
}

void MpvVideoItem::onRenderContextReady() {
    m_renderContextReady = true;
    if (m_pendingLoad) {
        m_pendingLoad = false;
        if (!m_mediaUrl.isEmpty()) {
            loadCurrentUrl();
        }
    }
}

bool MpvVideoItem::isPlaying() const {
    return m_playing;
}

void MpvVideoItem::setPlaying(bool playing) {
    if (m_playing != playing) {
        int val = playing ? 0 : 1;
        mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &val);
    }
}

bool MpvVideoItem::isLoading() const {
    return m_loading;
}

int MpvVideoItem::duration() const {
    return m_duration;
}

int MpvVideoItem::position() const {
    return m_position;
}

void MpvVideoItem::setPosition(int position) {
    double val = position;
    mpv_set_property(m_mpv, "time-pos", MPV_FORMAT_DOUBLE, &val);
}

int MpvVideoItem::volume() const {
    return m_volume;
}

void MpvVideoItem::setVolume(int volume) {
    double val = volume;
    mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &val);
}

int MpvVideoItem::startPosition() const {
    return m_startPosition;
}

void MpvVideoItem::setStartPosition(int seconds) {
    if (m_startPosition != seconds) {
        m_startPosition = seconds > 0 ? seconds : 0;
        emit startPositionChanged();
    }
}

QString MpvVideoItem::userAgent() const {
    return m_userAgent;
}

void MpvVideoItem::setUserAgent(const QString &userAgent) {
    if (m_userAgent != userAgent) {
        m_userAgent = userAgent;
        updateHttpHeaders();
        emit userAgentChanged();
    }
}

QString MpvVideoItem::referer() const {
    return m_referer;
}

void MpvVideoItem::setReferer(const QString &referer) {
    if (m_referer != referer) {
        m_referer = referer;
        updateHttpHeaders();
        emit refererChanged();
    }
}

void MpvVideoItem::updateHttpHeaders() {
    // Effective UA: playlist-provided, or a browser-like fallback so hosts
    // that reject mpv's default "libmpv" agent still serve the stream.
    const QString ua = m_userAgent.isEmpty() ? QString::fromLatin1(kDefaultUserAgent)
                                             : m_userAgent;

    // "user-agent" and "referrer" are the canonical mpv options and are also
    // forwarded to yt-dlp/ffmpeg by mpv itself, so no ytdl-raw-options needed.
    mpv_set_property_string(m_mpv, "user-agent", ua.toUtf8().constData());
    mpv_set_property_string(m_mpv, "referrer", m_referer.toUtf8().constData());

    // Also set the raw header fields — some demuxer paths (e.g. HLS segment
    // requests through ffmpeg) only pick headers up from here.
    QStringList headers;
    if (!m_referer.isEmpty()) {
        headers << QString("Referer: %1").arg(m_referer);
    }
    headers << QString("User-Agent: %1").arg(ua);

    // Use the MPV_FORMAT_NODE list form instead of a comma-joined string:
    // UA strings contain commas ("KHTML, like Gecko") which would otherwise
    // be split into separate (broken) header entries.
    QList<QByteArray> headerBytes;
    std::vector<mpv_node> items;
    headerBytes.reserve(headers.size());
    items.reserve(headers.size());
    for (const QString &h : headers) {
        headerBytes.append(h.toUtf8());
        mpv_node item;
        item.format = MPV_FORMAT_STRING;
        item.u.string = const_cast<char *>(headerBytes.last().constData());
        items.push_back(item);
    }

    mpv_node_list list;
    list.num = static_cast<int>(items.size());
    list.values = items.data();
    list.keys = nullptr;

    mpv_node node;
    node.format = MPV_FORMAT_NODE_ARRAY;
    node.u.list = &list;

    mpv_set_property(m_mpv, "http-header-fields", MPV_FORMAT_NODE, &node);
}

QVariantList MpvVideoItem::videoTracks() const { return m_videoTracks; }
QVariantList MpvVideoItem::audioTracks() const { return m_audioTracks; }
QVariantList MpvVideoItem::subtitleTracks() const { return m_subtitleTracks; }

void MpvVideoItem::setTrack(const QString &type, int id) {
    QString optName;
    if (type == "video") optName = "vid";
    else if (type == "audio") optName = "aid";
    else if (type == "sub" || type == "subtitle") optName = "sid";
    else optName = type;

    if (id < 0) {
        mpv_set_property_string(m_mpv, optName.toUtf8().constData(), "no");
    } else {
        int64_t val = id;
        mpv_set_property(m_mpv, optName.toUtf8().constData(), MPV_FORMAT_INT64, &val);
    }
}

void MpvVideoItem::updateTrackList() {
    int count = 0;
    if (mpv_get_property(m_mpv, "track-list/count", MPV_FORMAT_INT64, &count) < 0) {
        return;
    }

    QVariantList videoTracks, audioTracks, subtitleTracks;

    for (int i = 0; i < count; ++i) {
        QString prefix = QString("track-list/%1/").arg(i);
        
        char *type_str = nullptr;
        mpv_get_property(m_mpv, (prefix + "type").toUtf8().constData(), MPV_FORMAT_STRING, &type_str);
        
        int64_t id = -1;
        mpv_get_property(m_mpv, (prefix + "id").toUtf8().constData(), MPV_FORMAT_INT64, &id);
        
        char *title_str = nullptr;
        mpv_get_property(m_mpv, (prefix + "title").toUtf8().constData(), MPV_FORMAT_STRING, &title_str);
        
        char *lang_str = nullptr;
        mpv_get_property(m_mpv, (prefix + "lang").toUtf8().constData(), MPV_FORMAT_STRING, &lang_str);

        QVariantMap track;
        track["id"] = static_cast<int>(id);
        QString tType = type_str ? QString(type_str) : QString();
        track["type"] = tType;
        track["title"] = title_str ? QString(title_str) : QString("Track %1").arg(id);
        track["lang"] = lang_str ? QString(lang_str) : QString();

        if (tType == "video") videoTracks.append(track);
        else if (tType == "audio") audioTracks.append(track);
        else if (tType == "sub") subtitleTracks.append(track);

        if (type_str) mpv_free(type_str);
        if (title_str) mpv_free(title_str);
        if (lang_str) mpv_free(lang_str);
    }

    m_videoTracks = videoTracks;
    m_audioTracks = audioTracks;
    m_subtitleTracks = subtitleTracks;
    emit tracksChanged();
}

void MpvVideoItem::command(const QVariantList &args) {
    QList<QByteArray> bytesList;
    std::vector<const char *> ptrArgs;
    
    for (const QVariant &arg : args) {
        bytesList.append(arg.toString().toUtf8());
        ptrArgs.push_back(bytesList.last().constData());
    }
    ptrArgs.push_back(nullptr);
    
    mpv_command(m_mpv, ptrArgs.data());
}

void MpvVideoItem::setProperty(const QString &name, const QVariant &value) {
    // mpv properties that are always double-typed, even when QML hands us a
    // whole number (e.g. speed=2.0 arrives as QMetaType::Int since it has no
    // fractional part). Sending those as MPV_FORMAT_INT64 is a type mismatch
    // that mpv rejects, so the call silently does nothing.
    static const QSet<QString> doubleOnlyProps = { "speed", "volume", "time-pos" };

    if (doubleOnlyProps.contains(name)) {
        double val = value.toDouble();
        mpv_set_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_DOUBLE, &val);
        return;
    }

    if (value.typeId() == QMetaType::Bool) {
        int val = value.toBool() ? 1 : 0;
        mpv_set_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_FLAG, &val);
    } else if (value.typeId() == QMetaType::Int) {
        int val = value.toInt();
        mpv_set_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_INT64, &val);
    } else if (value.typeId() == QMetaType::Double) {
        double val = value.toDouble();
        mpv_set_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_DOUBLE, &val);
    } else if (value.typeId() == QMetaType::QString) {
        QByteArray bytes = value.toString().toUtf8();
        const char *val = bytes.constData();
        mpv_set_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_STRING, &val);
    }
}
