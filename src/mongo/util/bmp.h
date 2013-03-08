// @file bmp.h

#pragma once

namespace mongo { 

    struct Header;

#pragma pack(1)
    class Color {
    public:
        char color[3];

        void x();

        Color(int r, int g, int b) {
            color[0] = b;
            color[1] = g;
            color[2] = r;
        }
        Color() { }
    };
#pragma pack()

    class BMP { 
        Header *p;
    public:
        ~BMP();
        BMP(int width, int height);
        void set(int x, int y, Color color);
        const char* getFile(int& len);
    };

#pragma pack(1)
    struct Header { 
        char B, M;
        int len;
        int zero;
        int ofsPixelData;
        int hdrLen;
        int w, h;
        short nColorPlanes;
        short bitsPerPixel;
        int compressionType;
        int pixelDataSize; // = 3*height*width;
        int a,b,c,d;
        char data[1];

        Header(int W, int H) { 
            B = 'B'; M = 'M';
            w = W;
            h = H;
            pixelDataSize = w * h * 3;
            ofsPixelData = 0x36;
            len = pixelDataSize + ofsPixelData;
            zero = 0;
            hdrLen = 40;
            nColorPlanes=1;
            bitsPerPixel = 24;
            compressionType = 0;
            a = b = 0xec4;
            c = d = 0;
        }        
    };
#pragma pack()

    inline BMP::BMP(int w, int h) { 
        Header hdr(w, h);
        p = (Header *) malloc(hdr.len);
        *p = hdr;
        memset((void*)p->data, 0, hdr.pixelDataSize);
    }
    inline BMP::~BMP() { 
        free(p); p = 0;
    }
    inline void BMP::set(int x, int y, Color c) {
        char *z = &p->data[ (y * p->w + x) * 3 ];
        memcpy(z, c.color, 3);
    }
    inline const char * BMP::getFile(int& len) { 
        len = p->len;
        return (const char *) p;
    }
}
