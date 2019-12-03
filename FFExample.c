#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int findVideoStream(AVFormatContext *ctx){
    int ret =  -1;
    for (int i = 0; i < ctx->nb_streams; i++){
        if(ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            ret = i;
            break;
        }
    }
    return(ret);
}

void transcode(const char* iFilename, const char* oFilename){
    AVPacket *Pkt = av_packet_alloc();
    AVFrame *iF = av_frame_alloc();
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = NULL;
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    avformat_open_input(&iFx, iFilename, NULL, NULL);
    int vSn = findVideoStream(iFx);
    avformat_find_stream_info(iFx, NULL);
    AVCodec *iC = avcodec_find_decoder(iFx->streams[vSn]->codecpar->codec_id), *oC = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *iCx = avcodec_alloc_context3(iC), *oCx = avcodec_alloc_context3(oC);
    avcodec_parameters_to_context(iCx, iFx->streams[vSn]->codecpar);
    avcodec_parameters_to_context(oCx, iFx->streams[vSn]->codecpar);
    avcodec_open2(iCx, iC, NULL);
    AVStream *iVs = iFx->streams[vSn];
    oCx->width = 1280;
    oCx->height = 720;
    oCx->time_base = av_inv_q(iVs->avg_frame_rate);
    oCx->framerate = iVs->avg_frame_rate;
    oCx->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
    oCx->level = 31;
    av_opt_set(oCx->priv_data, "profile", "baseline", 0);
    av_opt_set(oCx->priv_data, "level", "31", 0);
    avcodec_open2(oCx, oC, NULL);
    avcodec_parameters_from_context(oVs->codecpar, oCx);
    oVs->time_base = oCx->time_base;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    int ret = 0;
    do{
        ret = av_read_frame(iFx, Pkt);
        if(Pkt->stream_index != vSn){
            av_packet_unref(Pkt);
            continue;
        }
        avcodec_send_packet(iCx, Pkt);
        if(avcodec_receive_frame(iCx, iF) == AVERROR(EAGAIN)){
            continue;
        }
        avcodec_send_frame(oCx, iF);
        if(avcodec_receive_packet(oCx, Pkt) == AVERROR(EAGAIN)){
            continue;
        }
        Pkt->pts = av_rescale_q(Pkt->pts, iVs->time_base, oVs->time_base);
        Pkt->dts = av_rescale_q(Pkt->dts, iVs->time_base, oVs->time_base);
        Pkt->duration = av_rescale_q(Pkt->duration, iVs->time_base, oVs->time_base);
        av_write_frame(oFx, Pkt);
        av_packet_unref(Pkt);
    }while(ret >= 0);
    av_write_trailer(oFx);
    avformat_close_input(&iFx);
    av_packet_free(&Pkt);
    av_frame_free(&iF);
    avformat_free_context(iFx);
    avformat_free_context(oFx);
    avcodec_free_context(&iCx);
    avcodec_free_context(&oCx);
}

int main(int argc, char **argv)
{
    transcode("bbb_sunflower_1080p_30fps_normal.mp4", "out.mp4");
    return 0;
}