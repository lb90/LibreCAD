#ifndef VIDEOPIPELINEMONIKER_H
#define VIDEOPIPELINEMONIKER_H

#include <QObject>
#include <QImage>

#include <memory>

class VideoPipeline;

class VideoPipelineMoniker
 : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(VideoPipelineMoniker)
public:
    virtual ~VideoPipelineMoniker();

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

private:
};

class VideoPipelineMonikerShared final
 : public VideoPipelineMoniker
{
Q_DISABLE_COPY(VideoPipelineMonikerShared)
public:
    ~VideoPipelineMonikerShared() override;

    void play() override;
    void pause() override;
    void stop() override;

private:
    bool paused {false};
    QImage paused_frame;
    std::shared_ptr<VideoPipeline> pipeline;
};

class VideoPipelineMonikerUnique final
 : public VideoPipelineMoniker
{
Q_DISABLE_COPY(VideoPipelineMonikerUnique)
public:
    ~VideoPipelineMonikerUnique() override;

    void play() override;
    void pause() override;
    void stop() override;

private:
    std::unique_ptr<VideoPipeline> pipeline;
};

#endif // VIDEOPIPELINEMONIKER_H
