#ifndef GSTCPUPIPELINE_H
#define GSTCPUPIPELINE_H

#include <QObject>
#include <gst/gst.h>

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
public:
    gboolean bus_cb(GstBus *bus, GstMessage *msg);
    void pad_added_cb(GstElement *element, GstPad *pad);
    GstFlowReturn new_sample_cb();
};

#endif
