#ifndef GSTCPUPIPELINE_H
#define GSTCPUPIPELINE_H

#include <QObject>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <string>
#include <list>
#include <functional>

class GstCpuPipeline
 : public QObject
{
Q_OBJECT
public:
    GstCpuPipeline(QObject *parent,
                   std::function<void(const std::string&)> log,
                   std::function<void(const std::string&)> print);
    GstCpuPipeline(const GstCpuPipeline&) = delete;
    void operator=(const GstCpuPipeline&) = delete;

    enum class State {
        basic,
        in_error 
    };

    bool setup();
    void deconstruct();

    State get_state() const { return state; }

signals:
    void NewFrame();
    void StateChanged();

private:
    GstPipeline *pipeline;
    GstElement  *source;
    GstElement  *decoder;
    GstElement  *converter;
    std::list<GstElement*> effects;
    GstElement  *sink;

    guint bus_cb_connection;

    std::function<void(const std::string&)> util_log;
    std::function<void(const std::string&)> util_print;

    State state;

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
