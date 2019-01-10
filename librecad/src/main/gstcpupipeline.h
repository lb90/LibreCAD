#ifndef GSTCPUPIPELINE_H
#define GSTCPUPIPELINE_H

#include <QObject>
#include <QMutex>
#include <QAtomicPointer>
#include <QImage>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <string>
#include <list>
#include <functional>

class GstCpuPipeline
 : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(GstCpuPipeline)
public:
    GstCpuPipeline(QObject *parent,
                   std::function<void(const std::string&)> log,
                   std::function<void(const std::string&)> print);

    enum class ObjectState {
        basic,
        constructed,
        deconstructed,
        in_error 
    };

    enum class SourceType {
        LiveCamera = 0,
        File
    };

    enum class PipelineStateNotify {
        stopped,
        paused,
        playing
    };

    bool setup(int index, const std::string& path = "");
    void deconstruct();

    SourceType get_source_type() const { return source_type; }
    ObjectState get_object_state() const { return object_state; }

    void stop();
    void pause();
    void play();

signals:
    void NewFrame();
    void PipelineStateChanged(int);
/*public:
    enum class PipelineState {
        null = 0,
        stopped,
        playing
    };*/

public:
    QImage *frame;
    QMutex frame_lock;
private:
    GstPipeline *pipeline;
    GstElement  *source;
    GstElement  *decoder;
    GstElement  *converter;
    std::list<GstElement*> effects;
    GstElement  *sink;

    guint bus_cb_connection;

private:
    SourceType source_type;
    ObjectState object_state;

    std::function<void(const std::string&)> util_log;
    std::function<void(const std::string&)> util_print;

private:
    gboolean bus_cb(GstBus *bus, GstMessage *msg);
    void pad_added_cb(GstElement *element, GstPad *pad);
    GstFlowReturn new_sample_cb();

/*friends for C callbacks*/
    friend gboolean bus_cb_forward(GstBus *bus,
                                   GstMessage *msg,
                                   gpointer data);
    friend void pad_added_cb_forward(GstElement *element,
                                     GstPad *pad,
                                     gpointer data);
    friend GstFlowReturn new_sample_cb_forward(GstAppSink*,
                                               gpointer data);
};

#endif
