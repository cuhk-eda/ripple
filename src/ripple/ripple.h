#ifndef _RIPPLE_RIPPLE_H_
#define _RIPPLE_RIPPLE_H_

#include <string>

namespace ripple {

class ProcessStatus {
public:
    enum RunStatus {
        NotStarted = 0,
        Prepared = 1,
        Initializing = 2,
        Running = 3,
        Aborting = 4,
        Aborted = 5,
        Finishing = 6,
        Finished = 7
    };
    std::string process;
    RunStatus status;
    int step;
    ProcessStatus(std::string process)
    {
        this->process = process;
        this->status = NotStarted;
        this->step = 0;
    }
};

class Ripple {
private:
    static Ripple* _instance;
    ProcessStatus* currentStatus = nullptr;

    Ripple() {}
    ~Ripple() {}

    int _run(int argc, char** argv);
    int _abort();

public:
    static Ripple* get();

    static inline int run(int argc, char** argv) { return get()->_run(argc, argv); }
    static inline int abort() { return get()->_abort(); }

    ProcessStatus::RunStatus getStatus();
    int getArgs(int argc, char** argv);
};
}

#endif
