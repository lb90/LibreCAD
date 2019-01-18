#ifndef GST_H
#define GST_H

#include <QObject>

#include <functional>
#include <vector>
#include <string>
#include <memory>

class VideoPipeline;

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

public:
    std::shared_ptr<VideoPipeline> get_camera_pipeline(int index);
    std::shared_ptr<VideoPipeline> get_file_pipeline(const std::string& path);

public:
    bool enumerate_camera_sources(std::vector<std::string>& cameras);

private:
    std::vector<std::weak_ptr<VideoPipeline>> camera_pipelines;

    std::function<void(const std::string&)> util_log;
    std::function<void(const std::string&)> util_print;
};

#endif
