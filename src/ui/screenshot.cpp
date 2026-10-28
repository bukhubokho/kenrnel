// language: C++17, target: Android arm64
// glReadPixels → BMP → /sdcard/DCIM/Screenshots/  → media scan broadcast
#include "screenshot.hpp"
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sys/stat.h>

namespace screenshot {

static void write_bmp(const char* path, int w, int h, const uint8_t* rgba) {
    // BMP stores BGR bottom-up; flip rows and convert RGBA→BGR
    int rowBytes = w * 3;
    int pad      = (4 - (rowBytes % 4)) % 4;
    int stride   = rowBytes + pad;
    int dataSize = stride * h;
    int fileSize = 54 + dataSize;

    uint8_t fh[14] = {};
    fh[0]='B'; fh[1]='M';
    fh[2]=(fileSize)&0xff; fh[3]=(fileSize>>8)&0xff;
    fh[4]=(fileSize>>16)&0xff; fh[5]=(fileSize>>24)&0xff;
    fh[10]=54;

    uint8_t ih[40] = {};
    ih[0]=40;
    ih[4]=(w)&0xff;      ih[5]=(w>>8)&0xff;
    ih[8]=(h)&0xff;      ih[9]=(h>>8)&0xff;   // positive = bottom-up
    ih[12]=1; ih[14]=24;
    ih[20]=(dataSize)&0xff; ih[21]=(dataSize>>8)&0xff;
    ih[22]=(dataSize>>16)&0xff; ih[23]=(dataSize>>24)&0xff;

    FILE* f = fopen(path, "wb");
    if (!f) return;
    fwrite(fh, 1, 14, f);
    fwrite(ih, 1, 40, f);

    std::vector<uint8_t> row(stride, 0);
    for (int y = 0; y < h; y++) {
        // GL bottom-left origin → row 0 is bottom; BMP bottom-up so same order
        const uint8_t* src = rgba + (size_t)y * w * 4;
        for (int x = 0; x < w; x++) {
            row[x*3+0] = src[x*4+2]; // B
            row[x*3+1] = src[x*4+1]; // G
            row[x*3+2] = src[x*4+0]; // R
        }
        fwrite(row.data(), 1, stride, f);
    }
    fclose(f);
}

bool take(int width, int height) {
    if (width <= 0 || height <= 0) return false;

    std::vector<uint8_t> pixels((size_t)width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Ensure output directory exists
    const char* dir = "/sdcard/DCIM/Screenshots";
    mkdir(dir, 0755);

    // Timestamp filename
    time_t t = time(nullptr);
    struct tm tm{}; localtime_r(&t, &tm);
    char path[256];
    snprintf(path, sizeof(path),
        "%s/claudepro_%04d%02d%02d_%02d%02d%02d.bmp",
        dir,
        tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    write_bmp(path, width, height, pixels.data());

    // Trigger media scanner so gallery picks it up immediately
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE "
        "-d 'file://%s' >/dev/null 2>&1 &", path);
    system(cmd);

    return true;
}

} // namespace screenshot
