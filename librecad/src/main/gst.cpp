#include "gst.h"
#include "gstcpupipeline.h"

static
const int MAX_CAMERAS = 10;

static
const unsigned int MAX_NAME_LENGTH = 30U;

bool Gst::init() {
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

bool Gst::get_live_video_sources(std::vector<std::string>& cameras) {
    std::string base = "source_enum_element";
    GstElement *source = NULL;
    const char *element_name =
#ifdef Q_OS_WIN
    "ksvideosrc";
#else
    "v4l2src";
#endif
    cameras.clear();

#ifdef Q_OS_WIN
    for (i = 0; i < MAX_CAMERAS; i++) {
        std::string name;
        GstStateChangeReturn state_ret;

        name = base + std::to_string(i);
        source = gst_element_factory_make(element_name, name.c_str());
        if (!source) {
            util_log("Error creating video source element.");
            return false;
        }

        g_object_set(source, "device-index", i, NULL);
#error "implement me"

        state_ret = gst_element_set_state(source, GST_STATE_READY);
        if (state_ret != GST_STATE_CHANGE_FAILURE) {
            gchar *device_name = NULL;

            g_object_get(source, "device-name", &device_name, NULL);
            if (device_name && strlen(device_name))
                cameras.emplace_back(device_name, 0, MAX_NAME_LENGTH);
            else
                cameras.emplace_back("unknown source");
        }
    }
#else
    std::string base_path = "/dev/video";
    for (int i = 0; i < MAX_CAMERAS; i++) {
        std::string name;
        std::string path;
        GstStateChangeReturn state_ret;

        name = base + std::to_string(i);
        source = gst_element_factory_make(element_name, name.c_str());
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
                cameras.emplace_back(device_name, 0, MAX_NAME_LENGTH);
            else
                cameras.emplace_back("unknown source");
        }
    }
#endif

    return true;
}
