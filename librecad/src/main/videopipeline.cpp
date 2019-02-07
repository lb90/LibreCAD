#include "videopipeline.h"

VideoPipeline::VideoPipeline(QObject *parent)
 : QObject(parent)
{
}

VideoPipeline::~VideoPipeline() {
    emit Ended();
}

void VideoPipeline::generate_state_changed() {
    if (have_last_known_state) {
        emit StateChanged(last_known_state);
    }
}
