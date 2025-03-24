#include "LoggerUtil.h"

metacg::MCGLogger& metacg::MCGLogger::instance() {
    static metacg::MCGLogger instance;
    return instance;
}
