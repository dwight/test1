/** @file mtrace.h

    Indicate you are doing something with the macros. Inline in a code scope:

    void foo() { 
      MTRACE("doing things");
      // do stuff
    }

    Or if an object's lifetime indicates a task, you can use _MTRACE:

    class my_scoped_lock { 
      _MTRACE;
      // ... 
    };

    In this case the description will be the class name in the display.

    Note that these actions have durations, they are not point-in-time events, rather, 
    they will only be noticed and sampled if they take some time.  So do not declare things
    that are done in 1 microsecond always.

    To view the output see 
      http://localhost:28017/_mtrace?micros=<samplinginterval>
    try 1 or 10 for the interval.
*/

#pragma once

#define MTRACE(x) mtrace::Doing __d__(x);

//#define _MTRACE(x) mtrace::_Doing<x> _d__;

#define _MTRACE \
 struct ZZZ { } zzz; \
 mtrace::_Doing<ZZZ> _d__;

namespace mongo {
    
    namespace mtrace { 

        typedef const char *Act;

        struct ThreadTraceState { 
            ThreadTraceState(int n) : nest(n) { }
            ThreadTraceState();
            enum { N = 7 };
            unsigned nest;
            Act acts[N];
            string desc;
        };

        // we don't use TSP_DECLARE here, as the implementation doesn't want a 
        // destruction if the pointed to object at its end.  there is more work to 
        // be done here perhaps including cleanup.
        //TSP_DECLARE(ThreadTraceState, traceState);
#if defined(_WIN32)
        extern __declspec( thread ) ThreadTraceState *traceState;
#else
        extern __thread ThreadTraceState *traceState;
#endif


        inline void begin(Act a) { 
            ThreadTraceState * volatile ts = traceState;
            if( ts->nest < ThreadTraceState::N ) {
                ts->acts[ts->nest] = a;
            }                
            ts->nest++;
        }

        inline void end() { 
            ThreadTraceState * volatile ts = traceState;
            ts->nest--;
        }

        void initThread(const char *desc);

        struct Doing { 
            Doing(Act a) { begin(a); }
            ~Doing() { end(); }
        };

        template <class a>
        struct _Doing { 
            Doing d;
            _Doing() : d(typeid(a).name()) { }
        };

        void getBmp(string& b, int microsPerSample/*0=use default,-1=max speed*/);

    }
    
}
