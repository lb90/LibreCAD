#ifndef GST_H
#define GST_H

#include <QObject>

#include <vector>
#include <string>
#include <functional>

class GstCpuPipeline;

class Gst
 : public QObject
{
Q_OBJECT
public:
    Gst(std::function<void(const std::string&)> log,
        std::function<void(const std::string&)> print)
     : util_log(log),
       util_print(print)
    {}
    bool init();

    bool create_camera_pipeline(int index,
                                GstCpuPipeline** pipeline);
    bool create_file_pipeline(const std::string& path,
                              GstCpuPipeline** pipeline);

    bool get_live_video_sources(std::vector<std::string>& cameras);
private:
    std::vector<GstCpuPipeline*> camera_pipelines;

    std::function<void(const std::string&)> util_log;
    std::function<void(const std::string&)> util_print;
};

#endif
