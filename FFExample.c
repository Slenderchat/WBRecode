#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
int main(int argc, char **argv)
{
    AVFormatContext     *inFmtCtx   =   avformat_alloc_context();
    avformat_open_input(&inFmtCtx, "bbb_sunflower_1080p_30fps_normal.mp4", NULL, NULL);
    avformat_find_stream_info(inFmtCtx, NULL);
    int vInd = -1;
    for (int i = 0; i < inFmtCtx->nb_streams; i++){
        AVCodecParameters *tmp = NULL;
        tmp = inFmtCtx->streams[i]->codecpar;
        if(tmp->codec_type == AVMEDIA_TYPE_VIDEO){
            vInd = i;
        }
    }
    AVCodecParameters   *inCodPmt   =   inFmtCtx->streams[vInd]->codecpar;
    /*
    AVCodec             *inCod      =   avcodec_find_decoder(inCodPmt->codec_id);
    AVCodecContext      *inCodCtx   =   avcodec_alloc_context3(inCod);
    avcodec_parameters_to_context(inCodCtx, inCodPmt);
    avcodec_open2(inCodCtx, inCod, NULL);
    */
    AVPacket            *inPkt      =   av_packet_alloc();
    AVFrame             *inFrm      =   av_frame_alloc();
    AVFormatContext     *outFmtCtx  =   NULL;
    avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, "out.ts");
    AVStream            *invstream  =   inFmtCtx->streams[vInd];
    AVStream            *ovstream   =   avformat_new_stream(outFmtCtx, NULL);
    avcodec_parameters_copy(ovstream->codecpar, inCodPmt);
    avio_open(&outFmtCtx->pb, "out.ts", AVIO_FLAG_WRITE);
    avformat_write_header(outFmtCtx, NULL);
    /*
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
    }*/
    while(av_read_frame(inFmtCtx, inPkt) >= 0){
        if (inPkt->stream_index != 0){
            av_packet_unref(inPkt);
            continue;
        }
        invstream = inFmtCtx->streams[vInd];
        inPkt->stream_index = 0;
        ovstream = outFmtCtx->streams[0];
        inPkt->pts = av_rescale_q_rnd(inPkt->pts, invstream->time_base, ovstream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        inPkt->dts = av_rescale_q_rnd(inPkt->dts, invstream->time_base, ovstream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        inPkt->duration = av_rescale_q(inPkt->duration, invstream->time_base, ovstream->time_base);
        inPkt->pos = -1;
        av_interleaved_write_frame(outFmtCtx, inPkt);
        av_packet_unref(inPkt);
    }
    av_write_trailer(outFmtCtx);
    avformat_close_input(&inFmtCtx);
    return 0;
}