#include "videopipelinecpu.h"

#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "fmt/format.h"

#include <cstring> /*for memcpy*/

/**
  GstCpuPipeline is a pipeline suitable for all-software processing, from
  decoding to effects to rendering. It is a safe mechanism for displaying
  video.
*/

typedef VideoPipelineCpu Me;

VideoPipelineCpu::VideoPipelineCpu(QObject *parent,
                                   std::function<void(const std::string&)> log,
                                   std::function<void(const std::string&)> print)
 : VideoPipeline(parent)
 , util_log(log)
 , util_print(print)
{}

VideoPipelineCpu::~VideoPipelineCpu() {
    if (pipeline) {
        gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
        g_source_remove(bus_cb_connection);

        pipeline = nullptr;
        bus_cb_connection = 0;
    }
}

GstFlowReturn
new_sample_cb_forward(GstElement*,
                      gpointer data)
{
    Me *inst = static_cast<Me*>(data);
    if (!inst) /*TODO throw from C callback */
        throw std::runtime_error("NULL instance pointer.");
    return inst->new_sample_cb();
}

gboolean
bus_cb_forward(GstBus *bus,
               GstMessage *msg,
               gpointer data)
{
    Me *inst = static_cast<Me*>(data);
    if (!inst) /*TODO throw from C callback */
        throw std::runtime_error("NULL instance pointer.");
    return inst->bus_cb(bus, msg);
}

void
pad_added_cb_forward(GstElement *element,
                     GstPad *pad,
                     gpointer data)
{
    Me *inst = static_cast<Me*>(data);
    if (!inst) /*TODO throw from C callback */
        throw std::runtime_error("NULL instance pointer.");
    inst->pad_added_cb(element, pad);
}

static void
set_camerasource(GstElement *source, int index) {
#ifdef Q_OS_WIN
    g_object_set(source, "device-index", index, NULL);
#else
    std::string base = "/dev/video";
    std::string path = base + std::to_string(index);
    g_object_set(source, "device", path.c_str(), NULL);
#endif
}
static void
set_filesource(GstElement *source, const std::string& path) {
    g_object_set(source, "location", path.c_str(), NULL);
}
static void
set_capsfilter(GstElement *capsfilter) {
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                    "format", G_TYPE_STRING, "xrgb",
                    NULL);

    g_object_set(capsfilter, "caps", caps, NULL);

    gst_object_unref(caps);
}
static void
set_appsink(GstElement *appsink) {
    g_object_set(appsink, "emit-signals", TRUE, NULL);
    /* Drop old buffers when the buffer queue is filled. */
    g_object_set(appsink, "drop", TRUE, NULL);
}

bool VideoPipelineCpu::construct_for_camera(int index) {
    if (object_state != ObjectState::basic || pipeline)
        return false;

#ifdef Q_OS_WIN
    std::string source_element_name = "ksvideosrc";
#else
    std::string source_element_name = "v4l2src";
#endif

    using namespace std::placeholders;
    if (!construct_common_priv(source_element_name,
                               std::bind(&set_camerasource, _1, index))) {
        object_state = ObjectState::ended_error;
        return false;
    }
    source_type = SourceType::camera;
    use = Use::shared;
    object_state = ObjectState::constructed;
    return true;
}

bool VideoPipelineCpu::construct_for_file(const std::string& path) {
    if (object_state != ObjectState::basic || pipeline)
        return false;

    std::string source_element_name = "filesrc";

    using namespace std::placeholders;
    if (!construct_common_priv(source_element_name.c_str(),
                               std::bind(&set_filesource, _1, path))) {
        object_state = ObjectState::ended_error;
        return false;
    }
    source_type = SourceType::file;
    use = Use::unique;
    object_state = ObjectState::constructed;
    return true;
}

bool VideoPipelineCpu::construct_common_priv(
        const std::string& source_element_name,
        std::function<void(GstElement*)> set_source)
{
    if (object_state != ObjectState::basic || pipeline)
        return false;

    if (!gst_is_initialized()) {
        util_log("gstreamer is not initialized, cannot build pipeline.");
        return false;
    }

    pipeline = GST_PIPELINE(gst_pipeline_new("cpu_pipeline"));
    if (!pipeline) {
        util_log("could not create GstPipeline object.");
        return false;
    }

    GstBus *bus = gst_pipeline_get_bus(pipeline);
    bus_cb_connection = gst_bus_add_watch(
                          bus,
                          bus_cb_forward,
                          gpointer(this));
    gst_object_unref(bus);

    source = gst_element_factory_make(source_element_name.c_str(), "source");
    if (!source) {
        util_log("could not create source element.");
        return false;
    }

    decoder = gst_element_factory_make("decodebin", "decoder");
    if (!decoder) {
        util_log("could not create decoder element.");
        return false;
    }

    converter = gst_element_factory_make("videoconvert", "converter");
    if (!converter) {
        util_log("could not create convert element.");
        return false;
    }

    sink = gst_element_factory_make("appsink", "sink");
    if (!converter) {
        util_log("could not create sink element.");
        return false;
    }

    set_source(source);
    set_capsfilter(sink);
    set_appsink(sink);
    g_signal_connect(sink, "new-sample",
                     G_CALLBACK(new_sample_cb_forward),
                     gpointer(this));

    gst_bin_add_many(GST_BIN(pipeline), source, decoder, converter, sink, NULL);

    if (!gst_element_link(source, decoder)) {
        util_log("could not link gstreamer pipeline elements"
                 "(source <-> decoder).");
        return false;
    }
    if (!gst_element_link(converter, sink)) {
        util_log("could not link gstreamer pipeline elements"
                 "(converter <-> sink).");
        return false;
    }
    g_signal_connect(decoder, "pad-added", G_CALLBACK(pad_added_cb_forward),
                     gpointer(this));

    return true;
}

GstFlowReturn VideoPipelineCpu::new_sample_cb() {
    GstSample *sample = NULL;
    bool ok = false;

    g_signal_emit_by_name(sink, "pull-sample", &sample, NULL);

    if (sample) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo info; /* contains the actual image */

        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            GstVideoInfo *video_info = gst_video_info_new();

            if (gst_video_info_from_caps(video_info,
                     gst_sample_get_caps(sample)))
            {
                if (video_info->finfo->format == GST_VIDEO_FORMAT_xRGB) {
                    unsigned char *src = info.data;
                    gint width = video_info->width;
                    gint height = video_info->height;

                    if (src && width > 0 && height > 0) {
                        QImage *new_image = new QImage(width, height, QImage::Format_RGB32);
                        if (new_image) {
                            uchar *dst = new_image->bits();
                            if (dst) {
                                size_t sz = 4U * unsigned(width) * unsigned(height); /*TODO check for overflow */
                                memcpy(dst, src, sz);

                                QImage *old_image = frame;
                                    frame_lock.lock();
                                    frame = new_image;
                                    frame_lock.unlock();
                                emit NewFrame();
                                delete old_image;

                                ok = true;
                            }
                        }
                    }
                }

                gst_video_info_free(video_info);
            }

            gst_buffer_unmap(buffer, &info);
        }

        gst_sample_unref(sample);
    }

    return ok ? GST_FLOW_OK
              : GST_FLOW_ERROR;
}

gboolean VideoPipelineCpu::bus_cb(GstBus *, GstMessage *msg) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            util_print(fmt::format("error: {}", error->message).c_str());

            g_clear_error(&error);
        }
        break;
        case GST_MESSAGE_EOS: {
        }
        break;
        case GST_MESSAGE_STATE_CHANGED: {
            if (msg->src == (GstObject*)pipeline) {
                GstState state = GST_STATE_NULL;
                gst_element_get_state(GST_ELEMENT(pipeline), &state, NULL, 0);
                switch (state) {
                    case GST_STATE_PLAYING:
                        { emit StateChanged(StateNotify::playing); }
                    break;
                    case GST_STATE_PAUSED:
                        { emit StateChanged(StateNotify::paused); }
                    break;
                    /*default:
                        { emit PipelineStateChanged(PipelineStateNotify::transitioning); }*/
                }
            }
        }
        break;
    }
    return TRUE;
}

void VideoPipelineCpu::pad_added_cb(GstElement*, GstPad *pad) {
    /* if videoconvert pad is already linked do nothing. */
    GstPad *sink_pad = gst_element_get_static_pad (converter, "sink");

    if (!gst_pad_is_linked(sink_pad)) {

        /* we have to skip the audio pad, check if it is the video pad
           and link. */
        GstCaps *new_pad_caps;
        GstStructure *new_pad_struct = NULL;
        const gchar *new_pad_type = NULL;

        new_pad_caps = gst_pad_get_current_caps(pad);
        new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
        new_pad_type = gst_structure_get_name(new_pad_struct);

        if (g_str_has_prefix(new_pad_type, "video/x-raw")) {

            if (!gst_element_link(decoder, converter)) {

                util_log("could not link gstreamer pipeline elements"
                         "(decoder <-> converter).");
                object_state = ObjectState::ended_error;
#error "implement me!" /* signal that pipeline ended! */
            }
        }
        gst_caps_unref(new_pad_caps);
    }
    gst_object_unref(sink_pad);
}

void VideoPipelineCpu::pause() {
    if (object_state != ObjectState::constructed)
        return;

    switch (use) {
    case Use::unique: {
        gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED);
    }
    break;
    case Use::shared: {
        playing_count--;
        if (!playing_count) {
            gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED);
        }
    }
    break;
    }
}

void VideoPipelineCpu::play() {
    if (object_state != ObjectState::constructed)
        return;

    switch (use) {
    case Use::unique: {
        gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
    }
    break;
    case Use::shared: {
        if (!playing_count) {
            gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
        }
        playing_count++;
    }
    break;
    }
}


