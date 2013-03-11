#include "pch.h"
#include "util/bmp.h"
#include "mtrace.h"
#include "util/timer.h"

namespace mongo { 
    namespace mtrace {

    unsigned font[] = {
        0x6999996,
        0x1111111,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xf99f999,
        0xE99E99E,
        0x7888887,
        0xE99999E,
        0xF88E88F,
        0xF88E888,
        0xF88F99F,
        0x999f999,//h
        0xF44444F,
        0x111999F,
        0x9AC8CA9,
        0x888888F,
        0x99ff999,//M
        0x9DDBBB9,
        0x6999996,
        0xE99E888,
        0x6999771,
        0xE99CA99,
        0xf88f11f,
        0xf444444,
        0x999999f,
        0x999aa66,
        0x99bbff9,//W
        0x9966699,
        0x999f222,
        0x9112449,
        0,0,0,0,
        0x000000f,
        0x00999f8,// mu
        0x007999f, // a
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x00f8888,
        0x0f8f1f0, // s
        0,0,0,
        0x0099bff,0,0,0
    };

    struct Canvas {
        BMP& bmp;
        int x, y;
        Color color;
        void moveTo(int a, int b) { x = a; y = b; }
        Canvas(BMP& b) : bmp(b),color(127,127,127) { x = y = 50; }
        void right(int n=1) { x+=n; }
        void down(int n=1) { y+=n; }
        void paint() { 
            bmp.set(x,y,color);
        }
        void print(const char *p, bool lc=true) { 
            while( *p ) { 
                print(*p++,lc);
            }
        }
        void print(int ch, bool lc_allowed=true) { 
            if( ch == ' ' ) {
                right(5);
                return;
            }
            int ofs = ch-'0';
            if( ofs < 0 || ofs > 58+17 ) 
                return;
            if( !lc_allowed && ch > 'Z' ) {
                ofs -= 32;
            }
            unsigned z = font[ofs];
            if( z == 0 && ofs >= 32 ) { 
                z = font[ofs-32];
            }
            for( int i = 0; i < 7; i++ ) {
                unsigned line = z & 0xf;
                for( int y = 3; y >= 0; y-- ) { 
                    if( z&(1<<y) ) { 
                        paint(); 
                    }
                    right();
                }
                right(-4);
                down();
                z = (z >> 4);
            }
            down(-7);
            right(6);
        }
    };

    //TSP_DEFINE(TraceState, traceState);

    __declspec( thread ) TraceState *traceState = 0;

    struct Canvas; 

    namespace {
        const int MaxThreads = 50;
        int nThreads = 0;
        SimpleMutex m("mtrace");
        TraceState states[MaxThreads];
        int z = 0;
        Color clr(0,0,0);
        const int graph_topmargin = 1000-30;
        struct ActInfo {
            const char *desc;
            Color c;
            int ord;
            int y() { 
                return graph_topmargin-nThreads*10-30-7-10*ord-((ord-1)/4)*6;
            }
            ActInfo(const char *d) : desc(d) {
                ord = z;
                int x = (z++)%3;
                clr.color[x] = (((unsigned)clr.color[x]) + 103 + z) % 256;
                c = clr;
                //log() << "actinfo " << d << " ord:" << ord << endl;
            }
            ActInfo(const char *d, int r, int g, int b) : desc(d), c(r,g,b) {
                ord = z++;
            }
            Color color() { return c; }
        };
        ActInfo ai[] = { 
            ActInfo("none"),
            ActInfo("SockR",0,0,88),
            ActInfo("SockW",104,155,155),
            ActInfo("GET_QLOCK_r"),
            ActInfo("GET_QLOCK_R"),
            ActInfo("GET_QLOCK_w"),
            ActInfo("GET_QLOCK_W"),
            ActInfo("GET_QLOCK_OTHER"),
            ActInfo("GET_QLOCK_X"),
            ActInfo("TOUCH",245,245,245),
            ActInfo("lkNESTR"),
            ActInfo("lkNESTW"),
            ActInfo("GET_LOCK_DBR",102,204,0),
            ActInfo("DBREAD",       51,102,0),
            ActInfo("GET_LOCK_DBW",204,204,0),
            ActInfo("DBWRITE",     102,102,0),
            ActInfo("STATICYIELD"),
            ActInfo("EARLYCOMMIT"),
            ActInfo("GROUPCOMMIT"),
            ActInfo("GROUPCOMMITLL"),
            ActInfo("SYNC"),
            ActInfo("LOCKRWNONGREEDY"),
            ActInfo("REMAPPRIVATEVIEW",200,0,0),
            ActInfo("ASSEMBLERESPONSE",44,44,44),
            ActInfo("TEMPRELEASE",127,0,255),
            ActInfo("PAGEFAULTEXCEPTION"),
            ActInfo("LOGOP",204,102,0),
            ActInfo("SLEEP",0,96,0)
        };

        void gen(BMP& b, Canvas& c, int micros) { 
            const int graph_lmargin = 140;
            const int tick_interval = 10; // microsecs
            Color light_grey(32,32,32);
            const int Samples = 1000;

            unsigned long long last = 0;
            Timer t;
            unsigned long long lastReport = 0;
            unsigned long long lastMilli = 0;
            int y=0;
            for( int s = 0; s < Samples; s++ ) { 
                unsigned long long goal = micros*s;
                //log() << "goal:" << goal << endl;
                while( 1 ) { 
                    unsigned long long m = t.micros();
                    //log() << "micros: " << m << endl;
                    if( m >= goal )
                        break;
                }

                int x = graph_lmargin+s;
                for( int i = 0; i < nThreads; i++ ) {
                    volatile TraceState& ts = states[i];
                    y = graph_topmargin - i*10;
                    if( s == 0 ) { 
                        c.color = Color(96,96,96);
                        c.moveTo(10, y);
                        c.print(states[i].desc.c_str(),false);
                    }
                    y += 6;
                    if( i % 2 == 1 && x % 2 == 0 ) {
                        b.set(x, y, light_grey);
                    }
                    for( int n = 0; n < ts.nest; n++ ) { 
                        ActInfo& a = ai[ ts.acts[n] ];
                        Color c = a.color();
                        b.set(x, y-n, c);
                        b.set(x, a.y()+3, c);
                    }             
                }
                
                unsigned long long passed = t.micros();
                if( passed >= lastReport+100 ) {
                    Color g(64,64,64);
                    b.set(x,graph_topmargin+17,g);
                    b.set(x,graph_topmargin+18,g);
                    if( passed >= lastMilli+1000) {
                        b.set(x,graph_topmargin+19,g);
                        b.set(x,graph_topmargin+20,g);
                        b.set(x,graph_topmargin+21,g);
                        b.set(x,graph_topmargin+22,g);
                        lastMilli = (passed/1000)*1000;
                    }
                    lastReport = (passed/100)*100;
                    /*if( last == 0 ) { 
                        c.color = Color(96,96,96);
                        c.moveTo(graph_lmargin-2, graph_topmargin+21);
                        c.print("10",true);
                        c.print('a'-1,true);
                        c.print('s',true);
                    }*/
                }
            }
            for( int i = 1; i < NActs; i++ ) { 
                c.moveTo(10, ai[i].y());
                c.color = ai[i].color();
                c.print(ai[i].desc,true);
            }
            log() << "time: " << t.millis() << ' ' << Samples << endl;
        }
    }

    TraceState::TraceState() { 
        nest = 0;
        for( int i = 0; i < N; i++ ) acts[i] = None;
    }

    void Doing::initThread(const char *desc) {
        if( traceState == 0 ) {
            SimpleMutex::scoped_lock lk(m);
            verify( nThreads < MaxThreads );
            traceState = &states[nThreads++];
            traceState->desc = desc;
        }
    }

    void getBmp(string& b, int micros) { 
        if(0){
            unsigned long long tick = curTimeMicros64();
            for( int x = 0; x < 10; x++ ) {
                sleepmicros(x);
                unsigned long long t = curTimeMicros64();
                log() << "slept " << x << ' ' << t-tick << endl;
                tick = curTimeMicros64();
            }
        }
        BMP bmp(1200, 1000);
        {
            Canvas c(bmp);
            gen(bmp,c,micros);
        }
        int len = 0;
        const char *p = bmp.getFile(len);
        b = string(p, len);
    }

}
}
