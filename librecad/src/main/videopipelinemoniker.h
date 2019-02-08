#ifndef VIDEOPIPELINEMONIKER_H
#define VIDEOPIPELINEMONIKER_H

#include "videopipeline.h"

#include <QObject>
#include <QImage>

#include <memory>

class QWidget;
class Gst;

class VideoPipelineMoniker final
 : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(VideoPipelineMoniker)
public:
    VideoPipelineMoniker(QWidget *view);
    ~VideoPipelineMoniker();

    enum class Use {
        unique, shared
    };

    void play();
    void pause();

    QImage* get_image();
    void release_image();

    bool wrap_file_pipeline(const std::string& path);
    bool wrap_camera_pipeline(int index);

    void reset();

    bool active() const { return bool(pipeline); }
    void generate_state_changed();

    VideoPipeline* get_pipeline() { return pipeline.get(); }
signals:
    void StateChanged(int); /* 0: play
                               1: pause
                               2: ended */

private slots:
    void OnStateChanged(VideoPipeline::StateNotify);
    void OnPipelineEnded();

private:
    std::shared_ptr<VideoPipeline> pipeline {nullptr};

    Use use {Use::unique};
    bool started {false};
    bool paused {false};
    QImage paused_image {};

    Gst *gst;
    QWidget *graphicview;
};

#endif // VIDEOPIPELINEMONIKER_H
