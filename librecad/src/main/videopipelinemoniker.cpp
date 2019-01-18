#include "videopipelinemoniker.h"
#include "videopipeline.h"

VideoPipelineMoniker::~VideoPipelineMoniker() = default;
VideoPipelineMonikerUnique::~VideoPipelineMonikerUnique() = default;
VideoPipelineMonikerShared::~VideoPipelineMonikerShared() = default;

void VideoPipelineMonikerShared::play() {
    if (paused) {
        pipeline->play();
        paused = false;
    }
}

void VideoPipelineMonikerShared::pause() {
    if (!paused) {
        pipeline->pause();
        paused = true;
    }
}

void VideoPipelineMonikerShared::stop() {
    pipeline.reset();
}

void VideoPipelineMonikerUnique::play() {
    pipeline->play();
}

void VideoPipelineMonikerUnique::pause() {
    pipeline->pause();
}

void VideoPipelineMonikerUnique::stop() {
    pipeline.reset();
}
