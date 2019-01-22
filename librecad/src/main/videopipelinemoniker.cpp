#include "videopipelinemoniker.h"
#include "videopipeline.h"
#include "gst.h"
#include "lc_application.h"

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
        if (paused) {
            pipeline->play();
            paused = false;
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

    /* disconnect all connected signals */
    disconnect(pipeline.get(), &VideoPipeline::Ended, this, &VideoPipelineMoniker::OnPipelineEnded);
    disconnect(pipeline.get(), &VideoPipeline::StateChanged, this, &VideoPipelineMoniker::OnStateChanged);

    pipeline.reset(); /* drop pipeline */
}

bool VideoPipelineMoniker::wrap_file_pipeline(const std::string& path) {
    reset();
    pipeline = gst->get_file_pipeline(path);
    if (!pipeline)
        return false;

    /* connect signals */
    connect(pipeline.get(), &VideoPipeline::StateChanged, this, &VideoPipelineMoniker::OnStateChanged);
    connect(pipeline.get(), &VideoPipeline::Ended, this, &VideoPipelineMoniker::OnPipelineEnded);

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

    use = Use::shared;
    return true;
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
