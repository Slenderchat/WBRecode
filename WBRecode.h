#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <stdbool.h>

int transcode(const char* iFilename, const char* oFilename);
int concat(const char* concatFilePath,const char* oFilename);