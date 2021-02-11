#include "timer.h"

using namespace utils;

std::vector<Timer> Timer::_timers;

unsigned Timer::start() {
    for (unsigned i = 0; i < _timers.size(); i++) {
        if (!_timers[i].active()) {
            _timers[i].active(true);
            _timers[i]._time = std::chrono::steady_clock::now();
            return i;
        }
    }
    _timers.push_back(Timer());
    _timers.back()._time = std::chrono::steady_clock::now();
    _timers.back().active(true);
    return _timers.size() - 1;
}

double Timer::time(unsigned id) {
    if (!_timers[id].active()) {
        return 0.0;
    }
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - _timers[id]._time).count();
}

double Timer::reset(unsigned id) {
    if (!_timers[id].active()) {
        return 0.0;
    }
    auto now = std::chrono::steady_clock::now();
    auto old = _timers[id]._time;
    _timers[id]._time = now;
    return std::chrono::duration<double>(now - old).count();
}

double Timer::stop(unsigned id) {
    if (!_timers[id].active()) {
        return 0.0;
    }
    double diff = time(id);
    _timers[id].active(false);
    return diff;
}

std::string Timer::getCurTimeStr(bool long_mode) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    if (long_mode) {
        ss << std::put_time(std::localtime(&in_time_t), "%Y_%m_%d_%H_%M_%S");
    } else {
        ss << std::put_time(std::localtime(&in_time_t), "%H_%M_%S");
    }
    return ss.str();
}