#include "pch.h"
#include "util/bmp.h"
#include "mtrace.h"
#include "util/timer.h"

namespace mongo { 

    unsigned font[] = {
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
        0x9112449};

    struct Canvas {
        BMP& bmp;
        int x, y;
        Color color;
        void moveTo(int a, int b) { x = a; y = b; }
        Canvas(BMP& b) : bmp(b),color(127,0,0) { x = y = 50; }
        void right(int n=1) { x+=n; }
        void down(int n=1) { y+=n; }
        void paint() { 
            bmp.set(x,y,color);
        }
        void print(const char *p, bool lc) { 
            while( *p ) { 
                print(*p++,lc);
            }
        }
        void print(int ch, bool lc) { 
            if( ch == ' ' ) {
                right(6);
                return;
            }
            int was = ch;
            ch = tolower(ch);
            if( ch < 'a' || ch > 'z' ) 
                return;
            unsigned z = font[ch-'a'];
            if( lc ) {
                if( was == 'r' ) { 
                    z = 0x00f8888;
                }
                if( was == 'w' ) { 
                    z = 0x0099bff;
                }
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
        struct ActInfo {
            const char *desc;
            Color c;
            int ord;
            int y() { return 1000-nThreads*10-20-20-7-10*ord; }
            ActInfo(const char *,int,int,int);
            ActInfo(const char *d) : desc(d) {
                ord = z;
                int x = z++%3;
                clr.color[x] = clr.color[x] + 77;
                c = clr;
                log() << "actinfo " << d << " ord:" << ord << endl;
            }
            Color color() { return c; }
        };
        ActInfo ai[] = { 
            ActInfo("none"),
            ActInfo("SockR"),
            ActInfo("SockW"),
            ActInfo("qlock_r"),
            ActInfo("qlock_R"),
            ActInfo("qlock_w"),
            ActInfo("qlock_W"),
            ActInfo("qlock_OTHER"),
            ActInfo("qlock_X"),
            ActInfo("touch"),
            ActInfo("lknestr"),
            ActInfo("lknestw"),
            ActInfo("lkdbr"),
            ActInfo("lkdbw")
        };

        void gen(BMP& b, Canvas& c) { 
            int Samples = 900;
            unsigned long long last = 0;
            Timer t;
            int y=0;
            for( int s = 0; s < Samples; s++ ) { 
                int x = 80+s;
                for( int i = 0; i < nThreads; i++ ) {
                    volatile TraceState& ts = states[i];
                    y = 1000 - i*10 - 20;
                    if( s == 0 ) { 
                        c.moveTo(10, y);
                        c.print(states[i].desc.c_str(),false);
                    }
                    y += 6;
                    b.set(x, y, Color(16,16,16));
                    for( int n = 0; n < ts.nest; n++ ) { 
                        ActInfo& a = ai[ ts.acts[n] ];
                        Color c = a.color();
                        b.set(x, y-n, c);
                        b.set(x, a.y()+3, c);
                    }             
                }
                y -= 10;
                unsigned long long tick = curTimeMicros64();
                if( tick-last>10 ) { 
                    b.set(x,y-2,Color(128,128,128));
                    b.set(x,y-3,Color(128,128,128));
                    last = tick;
                }
                //sleepmicros(10);
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

    void getBmp(string& b) { 
        BMP bmp(1000, 1000);
        bmp.set(0, 0, Color(255,0,0));
        bmp.set(0, 2, Color(0,255,0));
        bmp.set(0, 4, Color(255,0,255));
        {
            Canvas c(bmp);
            //c.print("abcdefghijklmnopqrstuvwxyz");
            gen(bmp,c);
        }
        int len = 0;
        const char *p = bmp.getFile(len);
        b = string(p, len);
    }

}
