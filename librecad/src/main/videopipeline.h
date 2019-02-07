#ifndef VIDEOPIPELINE_H
#define VIDEOPIPELINE_H

#include <QObject>
#include <QMutex>
#include <QImage>

class VideoPipeline
 : public QObject
{
Q_OBJECT
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

public:
    int camera_index {-1};
    std::string file_path {};
    QImage *image {nullptr};
    QMutex image_lock {};

signals:
    void NewFrame();
    void StateChanged(StateNotify);
    void Ended();

protected:
    SourceType source_type { SourceType::file };
    ObjectState object_state { ObjectState::basic };
    StateNotify last_known_state;
    bool        have_last_known_state {false};
};

#endif // VIDEOPIPELINE_H
