#include "WBRecode.h"

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

int concat(const char* concatFilePath,const char* oFilename){
    AVFormatContext *iFx = avformat_alloc_context();
    AVDictionary *iFd = NULL;
    av_dict_set_int(&iFd, "safe", 0, 0);
    avformat_open_input(&iFx, concatFilePath, av_find_input_format("concat"), &iFd);
    AVStream *iVs = iFx->streams[0];
    AVFormatContext *oFx = NULL;
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    oVs->codecpar = iVs->codecpar;
    oVs->time_base = iVs->time_base;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    AVPacket *P = av_packet_alloc();
    AVFrame *F = av_frame_alloc();
    while(1){
        int ret = 0;
        ret = av_read_frame(iFx, P);
        if(ret < 0){
            break;
        }
        ret = av_interleaved_write_frame(oFx, P);
        if (ret < 0){
            return(ret);
        }
    }
    av_write_trailer(oFx);
    return(0);
}

int transcode(const char* iFilename, const char* oFilename){
    AVFormatContext *iFx = avformat_alloc_context();
    avformat_open_input(&iFx, iFilename, NULL, NULL);
    avformat_find_stream_info(iFx, NULL);
    int vSn = findVideoStream(iFx);
    AVStream *iVs = iFx->streams[vSn];
    AVCodec *iC = avcodec_find_decoder(iVs->codecpar->codec_id);
    AVCodecContext *iCx = avcodec_alloc_context3(iC);
    avcodec_parameters_to_context(iCx, iVs->codecpar);
    avcodec_open2(iCx, iC, NULL);
    AVFormatContext *oFx = NULL;
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    AVCodec *oC = avcodec_find_encoder_by_name("libx264");
    AVCodecContext *oCx = avcodec_alloc_context3(oC);
    oCx->width = 1280;
    oCx->height = 720;
    oCx->pix_fmt = AV_PIX_FMT_YUV420P;
    oCx->time_base = (AVRational){1,9};
    AVDictionary *oCd = NULL;
    av_dict_set(&oCd, "profile", "baseline", 0);
    avcodec_open2(oCx, oC, &oCd);
    avcodec_parameters_from_context(oVs->codecpar, oCx);
    oVs->time_base = oCx->time_base;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    AVPacket *P = av_packet_alloc();
    AVFrame *F = av_frame_alloc();
    while(1){
        int ret = 0;
        ret = av_read_frame(iFx, P);
        if(ret < 0){
            break;
        }
        if(P->stream_index != vSn){
            av_packet_unref(P);
            continue;
        }
        P->pts = av_rescale_q_rnd(P->pts, iVs->time_base, oVs->time_base, AV_ROUND_DOWN);
        P->dts = av_rescale_q_rnd(P->dts, iVs->time_base, oVs->time_base, AV_ROUND_DOWN);
        P->duration = av_rescale_q_rnd(P->duration, iVs->time_base, oVs->time_base, AV_ROUND_DOWN);
        ret = avcodec_send_packet(iCx, P);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret < 0){
            return(ret);
        }
        ret = avcodec_receive_frame(iCx, F);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret < 0){
            return(ret);
        }
        F->pts = av_rescale_q_rnd(iCx->frame_number - 1, oCx->time_base, oVs->time_base, AV_ROUND_DOWN);
        ret = avcodec_send_frame(oCx, F);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret < 0){
            return(ret);
        }
        ret = avcodec_receive_packet(oCx, P);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret < 0){
            return(ret);
        }
        ret = av_interleaved_write_frame(oFx, P);
        if (ret < 0){
            return(ret);
        }
    }
    av_write_trailer(oFx);
    return(0);
}
