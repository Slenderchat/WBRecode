#include "WBRecode.h"

#define chkCi(i) if(i){return(i);}
#define chkCv(o) if(!o){return(-1);}
#define chkTi(i) if(i){return(i);}
#define chkTv(o) if(!o){return(-1);}

int findVideoStream(AVFormatContext *ctx){
    int ret =  -1;
    for (unsigned int i = 0; i < ctx->nb_streams; i++){
        if(ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            ret = i;
            break;
        }
    }
    return(ret);
}

int concat(const char* concatFilePath,const char* oFilename){
    int ret;
    AVFormatContext *iFx = avformat_alloc_context();
    chkCv(iFx);
    AVDictionary *iFd = NULL;
    ret = av_dict_set_int(&iFd, "safe", 0, 0);
    chkCi(ret);
    ret = avformat_open_input(&iFx, concatFilePath, av_find_input_format("concat"), &iFd);
    chkCi(ret);
    AVStream *iVs = iFx->streams[0];
    chkCv(iVs);
    AVFormatContext *oFx = NULL;
    ret = avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    chkCi(ret);
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    chkCv(oVs);
    oVs->codecpar = iVs->codecpar;
    oVs->time_base = iVs->time_base;
    ret = avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    chkCi(ret);
    ret = avformat_write_header(oFx, NULL);
    chkCi(ret);
    AVPacket *P = av_packet_alloc();
    chkCv(P);
    AVFrame *F = av_frame_alloc();
    chkCv(F);
    while(1){
        ret = av_read_frame(iFx, P);
        if(ret == AVERROR_EOF){
            break;
        }else{
            chkCi(ret);
        }
        ret = av_interleaved_write_frame(oFx, P);
        chkCi(ret);
    }
    ret = av_write_trailer(oFx);
    chkCi(ret);
    return(0);
}

int transcode(const char* iFilename, const char* oFilename){
    int ret = 0;
    AVFormatContext *iFx = avformat_alloc_context();
    chkTv(iFx);
    ret = avformat_open_input(&iFx, iFilename, NULL, NULL);
    chkTi(ret);
    ret = avformat_find_stream_info(iFx, NULL);
    chkTi(ret);
    int vSn = findVideoStream(iFx);
    chkTi(vSn);
    AVStream *iVs = iFx->streams[vSn];
    chkTv(iVs);
    AVCodec *iC = avcodec_find_decoder(iVs->codecpar->codec_id);
    chkTv(iC);
    AVCodecContext *iCx = avcodec_alloc_context3(iC);
    chkTv(iCx);
    ret = avcodec_parameters_to_context(iCx, iVs->codecpar);
    chkTi(ret);
    ret = avcodec_open2(iCx, iC, NULL);
    chkTi(ret);
    AVFormatContext *oFx = NULL;
    ret = avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    chkTi(ret);
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    chkTv(oVs);
    AVCodec *oC = avcodec_find_encoder_by_name("libx264");
    chkTv(oC);
    AVCodecContext *oCx = avcodec_alloc_context3(oC);
    chkTv(oCx);
    oCx->width = 1280;
    oCx->height = 720;
    oCx->pix_fmt = AV_PIX_FMT_YUV420P;
    oCx->time_base = (AVRational){1,9};
    AVDictionary *oCd = NULL;
    ret = av_dict_set(&oCd, "profile", "baseline", 0);
    chkTi(ret);
    ret = avcodec_open2(oCx, oC, &oCd);
    chkTi(ret);
    ret = avcodec_parameters_from_context(oVs->codecpar, oCx);
    chkTi(ret);
    oVs->time_base = oCx->time_base;
    ret = avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    chkTi(ret);
    ret = avformat_write_header(oFx, NULL);
    chkTi(ret);
    AVPacket *P = av_packet_alloc();
    chkTv(P);
    AVFrame *F = av_frame_alloc();
    chkTv(F);
    while(1){
        ret = av_read_frame(iFx, P);
        if(ret == AVERROR_EOF){
            break;
        }else{
            chkTi(ret);
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
        }else{
            chkTi(ret);
        }
        ret = avcodec_receive_frame(iCx, F);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else{
            chkTi(ret);
        }
        F->pts = av_rescale_q_rnd(iCx->frame_number - 1, oCx->time_base, oVs->time_base, AV_ROUND_DOWN);
        ret = avcodec_send_frame(oCx, F);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else{
            chkTi(ret);
        }
        ret = avcodec_receive_packet(oCx, P);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else{
            chkTi(ret);
        }
        ret = av_interleaved_write_frame(oFx, P);
        chkTi(ret);
    }
    ret = av_write_trailer(oFx);
    chkTi(ret);
    return(0);
}