#include "WBRecode.h"

int concat(const char* concatFilePath,const char* oFilename){
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = NULL;
    AVStream *iVs = NULL, *oVs = NULL;
    AVPacket *P = av_packet_alloc();
    AVDictionary *iFd = NULL;
    av_dict_set_int(&iFd, "safe", 0, 0);
    avformat_open_input(&iFx, concatFilePath, av_find_input_format("concat"), &iFd);
    avformat_find_stream_info(iFx, NULL);
    av_dump_format(iFx, 0, concatFilePath, 0);
    av_dict_free(&iFd);
    iVs = iFx->streams[0];
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    av_opt_set(oFx->priv_data, "movflags", "+faststart", 0);
    oVs = avformat_new_stream(oFx, NULL);
    avcodec_parameters_copy(oVs->codecpar, iVs->codecpar);
    oVs->time_base = iVs->time_base;
    av_dump_format(oFx, 0, oFilename, 1);
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    const int step = oVs->time_base.den / 9;
    int Pcount = 0;
    while(av_read_frame(iFx, P) >= 0){
        P->pts = Pcount * step;
        P->dts = Pcount * step;
        P->duration = step;
        Pcount++;
        av_interleaved_write_frame(oFx, P);
        av_packet_unref(P);
    }
    av_write_trailer(oFx);
    avformat_close_input(&iFx);
    avio_closep(&oFx->pb);
    av_packet_free(&P);
    avformat_free_context(iFx);
    avformat_free_context(oFx);
    return(0);
}

int transcode(const char* iFilename, const char* oFilename){
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = NULL;
    AVStream *iVs = NULL, *oVs = NULL;
    AVCodec *iC = NULL, *oC = avcodec_find_encoder_by_name("libx264");
    AVCodecContext *iCx = NULL, *oCx = NULL;
    AVPacket *iP = av_packet_alloc(), *oP = av_packet_alloc();
    AVFrame *F = av_frame_alloc();
    avformat_open_input(&iFx, iFilename, NULL, NULL);
    avformat_find_stream_info(iFx, NULL);
    av_dump_format(iFx, 0, iFilename, 0);
    iVs = iFx->streams[0];
    iC = avcodec_find_decoder(iVs->codecpar->codec_id);
    iCx = avcodec_alloc_context3(iC);
    avcodec_parameters_to_context(iCx, iVs->codecpar);
    avcodec_open2(iCx, iC, NULL);
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    oVs = avformat_new_stream(oFx, NULL); 
    oCx = avcodec_alloc_context3(oC);
    oCx->width = iCx->width;
    oCx->height = iCx->height;
    oCx->pix_fmt = iCx->pix_fmt;
    oCx->time_base = (AVRational){1,9};
    av_opt_set(oCx->priv_data, "x264-params", "bframes=2:keyint=4:min-keyint=4:force-cfr=1", 0);
    avcodec_open2(oCx, oC, NULL);
    avcodec_parameters_from_context(oVs->codecpar, oCx);
    oVs->time_base = oCx->time_base;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    av_dump_format(oFx, 0, oFilename, 1);
    const int step = oVs->time_base.den / oCx->time_base.den;
    int Fcount = 0, Pcount = 0;
    while(av_read_frame(iFx, iP) >= 0){
        int ret = avcodec_send_packet(iCx, iP);
        while(ret >= 0){
            ret = avcodec_receive_frame(iCx, F);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }else if(ret < 0){
                return(ret);
            }
            F->pts = Fcount * step;
            F->pict_type = AV_PICTURE_TYPE_NONE;
	        Fcount++;
            ret = avcodec_send_frame(oCx, F);
            while(ret >= 0){
                ret = avcodec_receive_packet(oCx, oP);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }else if(ret < 0){
                    return(ret);
                }
                oP->pts = Pcount * step;
                oP->dts = Pcount * step;
                oP->duration = step;
                Pcount++;
                ret = av_interleaved_write_frame(oFx, oP);
                av_packet_unref(oP);
            }
            av_frame_unref(F);
        }
        av_packet_unref(iP);
        av_frame_unref(F);
        av_packet_unref(oP);
    }
    av_write_trailer(oFx);
    avformat_close_input(&iFx);
    avio_closep(&oFx->pb);
    avcodec_close(iCx);
    avcodec_close(oCx);
    av_packet_free(&iP);
    av_packet_free(&oP);
    av_frame_free(&F);
    avformat_free_context(iFx);
    avformat_free_context(oFx);
    avcodec_free_context(&iCx);
    avcodec_free_context(&oCx);
    return(0);
}