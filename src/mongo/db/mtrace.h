//#include "mongo/util/concurrency/threadlocal.h"

#pragma once

#define DOING(x) mtrace::Doing __d__(mtrace:: ## x);

#define _DOING(x) mtrace::_Doing<mtrace:: ## x> _d__;

namespace mongo {

    namespace mtrace { 
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
            lock_nest_r, // local and admin are "nestable" locks...
            lock_nest_w,
            lock_db_r, db_read,
            lock_db_w, db_write,
            static_yield,
            early_commit,
            group_commit,
            group_commit_ll,
            sync,
            lock_rwnongreedy, // e.g. LockMongoFilesExclusive
            remapprivateview,
            assembleResponse,
            temp_release, 
            pagefaultexception,
            logop,
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

        template <Act a>
        struct _Doing { 
            Doing d;
            _Doing() : d(a) { }
        };

        void getBmp(string& b, int microsPerSample/*0=use default,-1=max speed*/);

    }

}
