#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <stdbool.h>

bool transcode(const char* iFilename, const char* oFilename);
bool concat(const char* concatFilePath,const char* oFilename);