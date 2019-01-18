#ifndef VIDEOPIPELINE_H
#define VIDEOPIPELINE_H

#include <QObject>

class VideoPipeline
 : public QObject
{
Q_DISABLE_COPY(VideoPipeline)
public:
    VideoPipeline(QObject *parent);
    virtual ~VideoPipeline();

    enum class ObjectState {
        basic,
        constructed, /* constructed means that it is ready to play */
        ended_ok,
        ended_error
    };

    enum class SourceType {
        camera,
        file
    };

    enum class StateNotify {
        playing,
        paused
        /* there is no stop, for that you have to
         * use the special signal Ended() */
    };

    virtual bool construct_for_camera(int index) = 0;
    virtual bool construct_for_file(const std::string& path) = 0;

    virtual void play() = 0;
    virtual void pause() = 0;

    SourceType get_source_type() const { return source_type; }
    ObjectState get_object_state() const { return object_state; }

    void generate_state_changed();
    int camera_index {-1};
    std::string file_path {};
signals:
    void NewFrame();
    void StateChanged(StateNotify);
    void Ended();

protected:
    SourceType source_type;
    ObjectState object_state;
};

#endif // VIDEOPIPELINE_H
