#include "videopipelinemoniker.h"
#include "videopipeline.h"
#include "gst.h"
#include "lc_application.h"

#include <QWidget>

VideoPipelineMoniker::VideoPipelineMoniker(QWidget *view) {
    gst = ((LC_Application*)qApp)->gst();
    if (!gst)
        throw std::runtime_error("NULL gst pointer in VideoPipelineMoniker");
    if (!view)
        throw std::runtime_error("NULL view passed to VideoPipelineMoniker");
    graphicview = view;
}

VideoPipelineMoniker::~VideoPipelineMoniker() {
    reset();
}

void VideoPipelineMoniker::play() {
    if (!pipeline)
        return;
    if (use == Use::shared) {
        if (!started || paused) {
            pipeline->play();
            paused = false;
            started = true;
        }
    }
    else pipeline->play();
}

void VideoPipelineMoniker::pause() {
    if (!pipeline)
        return;
    if (use == Use::shared) {
        if (!paused) {
            pipeline->pause();
            pipeline->image_lock.lock();
            paused_image = pipeline->image->copy();
            pipeline->image_lock.unlock();
            paused = true;
        }
    }
    else pipeline->pause();
}

void VideoPipelineMoniker::reset() {
    if (!pipeline)
        return;
    if (use == Use::shared)
        pause(); /* be sure to pause */

    emit StateChanged(2);
    /* disconnect all connected signals */
    disconnect(pipeline.get(), &VideoPipeline::Ended, this, &VideoPipelineMoniker::OnPipelineEnded);
    disconnect(pipeline.get(), &VideoPipeline::StateChanged, this, &VideoPipelineMoniker::OnStateChanged);
    void (QWidget::* update_slot)() = &QWidget::update;
    disconnect(pipeline.get(), &VideoPipeline::NewFrame, graphicview, update_slot);

    pipeline.reset(); /* drop pipeline */

    graphicview->update();
}

bool VideoPipelineMoniker::wrap_file_pipeline(const std::string& path) {
    reset();
    pipeline = gst->get_file_pipeline(path);
    if (!pipeline)
        return false;

    /* connect signals */
    connect(pipeline.get(), &VideoPipeline::StateChanged, this, &VideoPipelineMoniker::OnStateChanged);
    connect(pipeline.get(), &VideoPipeline::Ended, this, &VideoPipelineMoniker::OnPipelineEnded);
    void (QWidget::* update_slot)() = &QWidget::update;
    connect(pipeline.get(), &VideoPipeline::NewFrame, graphicview, update_slot);

    use = Use::unique;
    return true;
}

bool VideoPipelineMoniker::wrap_camera_pipeline(int index) {
    reset();
    pipeline = gst->get_camera_pipeline(index);
    if (!pipeline)
        return false;

    /* connect signals */
    connect(pipeline.get(), &VideoPipeline::StateChanged, this, &VideoPipelineMoniker::OnStateChanged);
    connect(pipeline.get(), &VideoPipeline::Ended, this, &VideoPipelineMoniker::OnPipelineEnded);
    void (QWidget::* update_slot)() = &QWidget::update;
    connect(pipeline.get(), &VideoPipeline::NewFrame, graphicview, update_slot);

    use = Use::shared;
    return true;
}

QImage* VideoPipelineMoniker::get_image(){
    if (use == Use::shared) {
        if (paused) {
            return &paused_image;
        }
    }
    pipeline->image_lock.lock();
    return pipeline->image;
}

void VideoPipelineMoniker::release_image() {
    if (use == Use::shared)
        if (paused)
            return;
    pipeline->image_lock.unlock();
}

void VideoPipelineMoniker::OnPipelineEnded() {
    reset();
    emit StateChanged(2);
}

void VideoPipelineMoniker::OnStateChanged(VideoPipeline::StateNotify new_state) {
    switch (new_state) {
    case VideoPipeline::StateNotify::playing:
        emit StateChanged(0);
        break;
    case VideoPipeline::StateNotify::paused:
        emit StateChanged(1);
        break;
    }
}
