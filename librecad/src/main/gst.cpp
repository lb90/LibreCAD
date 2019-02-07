#include "gst.h"
#include "videopipelinecpu.h"

static
const
int MAX_CAMERAS = 10;

static
const
unsigned int MAX_NAME_LENGTH = 30U;

bool Gst::init() {
    if (!init_gstreamer())
        return false;
    if (!enumerate_camera_sources())
        return false;
    for (unsigned i = 0; i < camera_names.size(); ++i) {
        camera_pipelines.emplace_back();
    }
    return true;
}

bool Gst::init_gstreamer() {
    bool is_ok = true;

    if (!gst_is_initialized()) {
        GError *error = NULL;

        if (!gst_init_check(0, NULL, &error)) {
            std::string log_string = "could not initialize gstreamer";
            if (error && error->message)
                util_log(log_string + ": " + error->message);
            else
                util_log(log_string + ".");

            is_ok = false;
        }

        g_clear_error(&error);
    }

    return is_ok;
}

std::shared_ptr<VideoPipeline> Gst::get_camera_pipeline(int index) {
    if (index < 0 || unsigned(index) >= camera_pipelines.size()) {
        util_log("live camera index is out of range");
        return nullptr;
    }
    /*TODO for C++14: use std::make_shared<>*/
    std::shared_ptr<VideoPipeline> pipeline = camera_pipelines.at(unsigned(index)).lock();
    if (!pipeline) {
        pipeline.reset(new VideoPipelineCpu(nullptr, util_log, util_print));

        if (!pipeline->construct_for_camera(index))
            return nullptr;

        camera_pipelines.at(unsigned(index)) = pipeline;
    }
    assert(pipeline->get_object_state() != VideoPipeline::ObjectState::basic);

    return pipeline;
}

std::shared_ptr<VideoPipeline> Gst::get_file_pipeline(const std::string& path) {
    /*TODO for C++14: use std::make_shared<>*/
    std::shared_ptr<VideoPipeline> pipeline(new VideoPipelineCpu(nullptr, util_log, util_print));
    if (!pipeline->construct_for_file(path))
        return nullptr;

    return pipeline;
}

bool Gst::enumerate_camera_sources() {
    std::string base = "source_enum_element";
    GstElement *source = NULL;

    camera_names.clear();

#ifdef Q_OS_WIN
    for (int i = 0; i < MAX_CAMERAS; i++) {
        std::string name;
        GstStateChangeReturn state_ret;

        name = base + std::to_string(i);
        source = gst_element_factory_make("ksvideosrc", name.c_str());
        if (!source) {
            util_log("Error creating video source element.");
            return false;
        }

        g_object_set(source, "device-index", i, NULL);

        state_ret = gst_element_set_state(source, GST_STATE_READY);
        if (state_ret != GST_STATE_CHANGE_FAILURE) {
            gchar *device_name = NULL;

            g_object_get(source, "device-name", &device_name, NULL);
            if (device_name && strlen(device_name))
                camera_names.emplace_back(device_name, 0, MAX_NAME_LENGTH);
            else
                camera_names.emplace_back("unknown source");
        }
    }
#else
    std::string base_path = "/dev/video";
    for (int i = 0; i < MAX_CAMERAS; i++) {
        std::string name;
        std::string path;
        GstStateChangeReturn state_ret;

        name = base + std::to_string(i);
        source = gst_element_factory_make("v4l2src", name.c_str());
        if (!source) {
            util_log("Error creating video source element.");
            return false;
        }

        path = base_path + std::to_string(i);
        g_object_set(source, "device", path.c_str(), NULL);

        state_ret = gst_element_set_state(source, GST_STATE_READY);
        if (state_ret != GST_STATE_CHANGE_FAILURE) {
            gchar *device_name = NULL;

            g_object_get(source, "device-name", &device_name, NULL);
            if (device_name && strlen(device_name))
                camera_names.emplace_back(device_name, 0, MAX_NAME_LENGTH);
            else
                camera_names.emplace_back("unknown source");
        }
    }
#endif

    return true;
}
