#include <libavformat/avformat.h>

int transcode(const char* iFilename, const char* oFilename);
int concat(const char* concatFilePath,const char* oFilename);