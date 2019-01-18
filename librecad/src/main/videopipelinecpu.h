#ifndef GSTCPUPIPELINE_H
#define GSTCPUPIPELINE_H

#include "videopipeline.h"

#include <QObject>
#include <QMutex>
#include <QAtomicPointer>
#include <QImage>

#include <gst/gst.h>

#include <string>
#include <list>
#include <functional>

class VideoPipelineCpu
 : public VideoPipeline
{
Q_DISABLE_COPY(VideoPipelineCpu)
public:
    VideoPipelineCpu(QObject *parent,
                     std::function<void(const std::string&)> log,
                     std::function<void(const std::string&)> print);
    ~VideoPipelineCpu() override;

    enum class Use {
        unique,
        shared
    };

    bool construct_for_camera(int index) override;
    bool construct_for_file(const std::string& path) override;

    void pause() override;
    void play() override;

public:
    QImage *frame {nullptr};
    QMutex frame_lock;

private:
    GstPipeline *pipeline {nullptr};
    GstElement  *source;
    GstElement  *decoder;
    GstElement  *converter;
    std::list<GstElement*> effects;
    GstElement  *sink;

    guint bus_cb_connection {0};

    Use use {Use::unique};
    int playing_count {0};
private:
    std::function<void(const std::string&)> util_log;
    std::function<void(const std::string&)> util_print;

private:
    bool construct_common_priv(
            const std::string& source_element_name,
            std::function<void(GstElement*)> setup_source);

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
    friend GstFlowReturn new_sample_cb_forward(GstElement*,
                                               gpointer data);
};

#endif
