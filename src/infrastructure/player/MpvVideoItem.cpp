#include "MpvVideoItem.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QThread>
#include <mpv/render_gl.h>
#include <stdexcept>
#include <iostream>
#include <QVariantMap>

namespace {
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
        : m_item(item)
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

        if (mpv_render_context_create(&m_mpv_gl, item->mpvHandle(), params) < 0) {
            std::cerr << "Failed to initialize mpv GL context" << std::endl;
        }

        mpv_render_context_set_update_callback(m_mpv_gl, MpvVideoItem::on_mpv_update, item);
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

        m_item->window()->beginExternalCommands();

        mpv_opengl_fbo mpfbo;
        mpfbo.fbo = fbo->handle();
        mpfbo.w = fbo->width();
        mpfbo.h = fbo->height();
        mpfbo.internal_format = 0;

        int flip_y = 1;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_mpv_gl, params);
        m_item->window()->endExternalCommands();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    MpvVideoItem *m_item;
    mpv_render_context *m_mpv_gl;
};

MpvVideoItem::MpvVideoItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        throw std::runtime_error("could not create mpv context");
    }

    // Default configuration for better performance and Qt compatibility
    mpv_set_option_string(m_mpv, "hwdec", "auto");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "audio-channels", "auto-safe");

    if (mpv_initialize(m_mpv) < 0) {
        throw std::runtime_error("could not initialize mpv context");
    }

    // Observe properties
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "track-list", MPV_FORMAT_NONE);

    mpv_set_wakeup_callback(m_mpv, on_mpv_events, this);

    connect(this, &MpvVideoItem::onUpdate, this, [this]() {
        update();
    }, Qt::QueuedConnection);
}

MpvVideoItem::~MpvVideoItem() {
    if (m_mpv) {
        mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
        mpv_terminate_destroy(m_mpv);
    }
}

QQuickFramebufferObject::Renderer *MpvVideoItem::createRenderer() const {
    window()->setPersistentSceneGraph(true);
    return new MpvRenderer(const_cast<MpvVideoItem *>(this));
}

void MpvVideoItem::on_mpv_events(void *ctx) {
    QMetaObject::invokeMethod(static_cast<MpvVideoItem *>(ctx), "handleMpvEvent", Qt::QueuedConnection);
}

void MpvVideoItem::on_mpv_update(void *ctx) {
    auto *item = static_cast<MpvVideoItem *>(ctx);
    emit item->onUpdate();
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
        const char *args[] = {"loadfile", url.toUtf8().constData(), nullptr};
        mpv_command(m_mpv, args);
        emit mediaUrlChanged();
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

QString MpvVideoItem::userAgent() const {
    return m_userAgent;
}

void MpvVideoItem::setUserAgent(const QString &userAgent) {
    if (m_userAgent != userAgent) {
        m_userAgent = userAgent;
        mpv_set_property_string(m_mpv, "user-agent", userAgent.toUtf8().constData());
        emit userAgentChanged();
    }
}

QString MpvVideoItem::referer() const {
    return m_referer;
}

void MpvVideoItem::setReferer(const QString &referer) {
    if (m_referer != referer) {
        m_referer = referer;
        mpv_set_property_string(m_mpv, "referrer", referer.toUtf8().constData());
        emit refererChanged();
    }
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
