//#include "mongo/util/concurrency/threadlocal.h"

namespace mongo {

    enum Act { 
        None = 0,
        SockR,
        SockW,
        qlock_r,
        qlock_R,
        qlock_w,
        qlock_W,
        qlock_Other,
        qlock_X,
        act_touch,
        lock_nest_r,
        lock_nest_w,
        lock_db_r,
        lock_db_w,
        NActs
    };

    struct TraceState { 
        TraceState();
        enum { N = 7 };
        int nest;
        Act acts[N];
        string desc;
    };

    extern __declspec( thread ) TraceState *traceState;

    //TSP_DECLARE(TraceState, traceState);

    struct Doing { 
        static void initThread(const char *desc);
        Doing(Act a) { 
            TraceState * volatile ts = traceState;
            if( ts->nest < TraceState::N ) {
                ts->acts[ts->nest] = a;
            }                
            ts->nest++;
        }
        ~Doing() {
            TraceState * volatile ts = traceState;
            ts->nest--;
        }
    };

    void getBmp(string& b);

}
