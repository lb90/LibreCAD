#include "gstcpupipeline.h"

#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "fmt/format.h"

#include <QImage>

/**
  GstCpuPipeline is a pipeline suitable for all-software processing, from
  decoding to effects to rendering. It is a safe mechanism for displaying
  video.
*/

typedef GstCpuPipeline Me;

GstCpuPipeline::GstCpuPipeline(QObject *parent,
                               std::function<void(const std::string&)> log,
                               std::function<void(const std::string&)> print)
 : QObject(parent)
 , util_log(log)
 , util_print(print)
{}

GstFlowReturn
new_sample_cb_forward(GstAppSink*,
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
set_filesource(GstElement *source) {
    g_object_set(source, "location", "", NULL); /*TODO*/
}
static void
set_capsfilter(GstElement *capsfilter) {
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                    "format", G_TYPE_STRING, "rgb",
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
bool GstCpuPipeline::setup() {
    if (pipeline)
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

    source = gst_element_factory_make("filesrc", "source");
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

    set_filesource(source);
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

GstFlowReturn GstCpuPipeline::new_sample_cb() {
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
                unsigned char *src = info.data;
                gint width = video_info->width;
                gint height = video_info->height;

                QImage *qimage = new QImage(width, height, QImage::Format_RGB32);
                unsigned char *dst = qimage->bits();
                

                ok = true;
                /* memory barrier */
                emit NewFrame();

                gst_video_info_free(video_info);
            }

            gst_buffer_unmap(buffer, &info);
        }

        gst_sample_unref(sample);
    }

    return ok ? GST_FLOW_OK
              : GST_FLOW_ERROR;
}

gboolean GstCpuPipeline::bus_cb(GstBus *, GstMessage *msg) {
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
    }
    return TRUE;
}

void GstCpuPipeline::pad_added_cb(GstElement*, GstPad *pad) {
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
                this->deconstruct();
                this->state = State::in_error;
            }
        }
        gst_caps_unref(new_pad_caps);
    }
    gst_object_unref(sink_pad);
}

/**
  Please, do not re-setup a deconstructed pipeline. Just create a new object.
  Calling setup() on a deconstructed pipeline yelds an error.
*/
void GstCpuPipeline::deconstruct() {
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_cb_connection);
}
