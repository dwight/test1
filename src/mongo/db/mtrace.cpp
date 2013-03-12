#include "pch.h"
#include "mongo/util/bmp.h"
#include "mongo/util/mtrace.h"
#include "mongo/util/timer.h"
#include "mongo/util/mongoutils/str.h"

using namespace mongoutils;

namespace mongo { 
    namespace mtrace {

    unsigned font[] = {
        0x6999996,/*0*/   0x1111111,        0x699248f,        0x788e887,
        0xaaaf222,        0xf88f11f,        0xf88f99f,        0xf111111,
        0x6996996,        0x6997111,        0x0020200,        0,0,0,0,0,0,
        0xf99f999,//A
        0xE99E99E,        0x7888887,        0xE99999E,        0xF88E88F,
        0xF88E888,        0x788F997,        0x999f999,        0xF44444F,
        0x111999F,        0x9AC8CA9,        0x888888F,        0x99ff999,
        0x9DDBBB9,        0x6999996,        0xE99E888,        0x6999771,
        0xE99CA99,        0x788f11e,        0xf444444,        0x999999f,
        0x999aa66,        0x99bbff9,        0x9966699,        0x999f222,
        0x9112449,        0,0,0,0,          0x000000f,/*_*/   0x00999f8,//μ
        0x0079997,//a
        0x888e99e,        0x0078887,        0x1179997,        0x0069f86,//e
        0x0699e88,        0x0069717,        0x088f999,        0x0202222,
        0x0111117,        0x089aca9,        0x2222222,        0x00ff999,
        0x00f9999,        0x0069996,        0x00e99e8,        0x0079971,
        0x0078888,        0x00f8f1f,        0x022f223,        0x0099997,
        0x0009966,        0x0099bff,        0x0099699,        0x0009648,
        0x000f24f
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

    //TSP_DEFINE(ThreadTraceState, traceState);
#if defined(_WIN32)
    __declspec( thread ) ThreadTraceState *traceState = 0;
#else
    __thread ThreadTraceState *traceState = 0;
#endif

    struct Canvas; 

    namespace {
        const int MaxThreads = 50;
        int nThreads = 0;
        SimpleMutex m("mtrace");
        ThreadTraceState states[MaxThreads];
        int z = 0;
        Color clr(0,0,0);
        const int graph_topmargin = 1000-30;
        struct ActInfo {
            string desc;
            Color c;
            int ord;
            int y() { 
                return graph_topmargin-nThreads*10-30-7-10*ord/*-((ord-1)/4)*6*/;
            }
            ActInfo(const char *d) {
                {
                    string s = d;
                    if( str::endsWith(s, "ZZZ") ) { 
                        log() << "actinfo " << s << endl;
                        s = str::before(s, "::ZZZ");      
                        if( str::startsWith(s, "class") )
                            s = s.substr(5);
                        if( str::startsWith(s,"struct") )
                            s = s.substr(6);
                        s = str::ltrim(s);
                        if( str::startsWith(s,"mongo::") )
                            s = s.substr(7);
                    }
                    desc = s;
                }

                ord = z;
                int x = (z++)%3;
                clr.color[x] = (((unsigned)clr.color[x]) + 103 + z) % 256;
                c = clr;
                //log() << "actinfo " << d << " ord:" << ord << endl;
            }
            /*ActInfo(const char *d, int r, int g, int b) : desc(d), c(r,g,b) {
                ord = z++;
            }*/
            Color color() { return c; }
        };
        map<const char *,ActInfo*> actInfo;

        ActInfo& getAct(Act a) { 
            ActInfo*& ai = actInfo[a];
            if( ai == 0 ) { 
                ai = new ActInfo(a);
            }
            return *ai;
        }

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
                    volatile ThreadTraceState& ts = states[i];
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
                    for( unsigned n = 0; n < ts.nest; n++ ) { 
                        ActInfo& a = getAct( ts.acts[n] );
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
            {
                for( map<const char*,ActInfo*>::iterator i = actInfo.begin(); i != actInfo.end(); i++ ) {
                    ActInfo& a = *i->second;
                    c.moveTo(10, a.y());
                    c.color = a.color();
                    c.print(a.desc.c_str(),true);
                }
            }
            log() << "time: " << t.millis() << ' ' << Samples << endl;
        }
    }

    ThreadTraceState::ThreadTraceState() { 
        nest = 0;
        for( int i = 0; i < N; i++ ) acts[i] = "";
    }

    void initThread(const char *desc) {
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
