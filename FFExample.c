#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
int main(int argc, char **argv)
{
    AVFormatContext     *inFmtCtx   =   avformat_alloc_context();
    if(avformat_open_input(&inFmtCtx, argv[1], NULL, NULL) != 0){
        return -1;
    }
    if(avformat_find_stream_info(inFmtCtx, NULL) < 0){
        return -2;
    }
    int vInd = -1;
    for (int i = 0; i < inFmtCtx->nb_streams; i++){
        AVCodecParameters *tmp = NULL;
        tmp = inFmtCtx->streams[i]->codecpar;
        if(tmp->codec_type == AVMEDIA_TYPE_VIDEO){
            vInd = i;
        }
    }
    if(vInd == -1){
        return -3;
    }
    AVCodecParameters   *inCodPmt   =   inFmtCtx->streams[vInd]->codecpar;
    AVCodec             *inCod      =   avcodec_find_decoder(inCodPmt->codec_id);
    AVCodecContext      *inCodCtx   =   avcodec_alloc_context3(inCod);
    avcodec_parameters_to_context(inCodCtx, inCodPmt);
    avcodec_open2(inCodCtx, inCod, NULL);
    AVPacket            *inPkt      =   av_packet_alloc();
    AVFrame             *inFrm      =   av_frame_alloc();
    int toProc = 300;
    while(av_read_frame(inFmtCtx, inPkt) >= 0){
        if(inPkt->stream_index == vInd){
            avcodec_send_packet(inCodCtx, inPkt);
            avcodec_receive_frame(inCodCtx, inFrm);
            printf("Frame %d\n", inCodCtx->frame_number);
            if(toProc-- < 0){
                break;
            }
        }
    }
    avformat_close_input(&inFmtCtx);
    return 0;
}