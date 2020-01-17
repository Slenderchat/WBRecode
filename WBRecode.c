#include "WBRecode.h"

bool concat(const char* concatFilePath,const char* oFilename){
    av_log_set_level(AV_LOG_WARNING);
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = NULL;
    AVStream *iVs = NULL, *oVs = NULL;
    AVPacket *P = av_packet_alloc();
    AVDictionary *iFd = NULL;
    av_dict_set_int(&iFd, "safe", 0, 0);
    avformat_open_input(&iFx, concatFilePath, av_find_input_format("concat"), &iFd);
    avformat_find_stream_info(iFx, NULL);
    av_dict_free(&iFd);
    iVs = iFx->streams[0];
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    av_opt_set(oFx->priv_data, "movflags", "+faststart", 0);
    oVs = avformat_new_stream(oFx, NULL);
    avcodec_parameters_copy(oVs->codecpar, iVs->codecpar);
    oVs->time_base = iVs->time_base;
    oVs->avg_frame_rate = (AVRational){9,1};
    oVs->r_frame_rate = oVs->avg_frame_rate;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    const int step = oVs->time_base.den / oVs->avg_frame_rate.num;
    int Pcount = 0, ret = 0;
    bool isFlush = false;
    while(1){
        ret = av_read_frame(iFx, P);
        if(ret == AVERROR_EOF){
            P = NULL;
            isFlush = true;
        }else if(ret < 0){
            return(false);
        }
        if(P){
            P->pts = Pcount * step;
            P->dts = (Pcount - 1) * step;
            P->duration = step;
            Pcount++;
        }
        av_interleaved_write_frame(oFx, P);
        if(P){
            av_packet_unref(P);
        }
        if(isFlush){
            break;
        }
    }
    av_write_trailer(oFx);
    avformat_close_input(&iFx);
    avio_closep(&oFx->pb);
    av_packet_free(&P);
    avformat_free_context(iFx);
    avformat_free_context(oFx);
    return(true);
}

bool transcode(const char* iFilename, const char* oFilename){
    av_log_set_level(AV_LOG_WARNING);
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = NULL;
    AVStream *iVs = NULL, *oVs = NULL;
    const AVCodec *iC = avcodec_find_decoder(AV_CODEC_ID_H264), *oC = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *iCx = NULL, *oCx = NULL;
    AVPacket *iP = av_packet_alloc(), *oP = av_packet_alloc();
    AVFrame *iF = av_frame_alloc(), *oF = av_frame_alloc();
    AVFilterGraph *fG = avfilter_graph_alloc();
    AVFilterContext *inFx, *outFx, *fpsFx;
    const AVFilter *inF = avfilter_get_by_name("buffer"), *outF = avfilter_get_by_name("buffersink"), *fpsF = avfilter_get_by_name("fps");
    avformat_open_input(&iFx, iFilename, NULL, NULL);
    avformat_find_stream_info(iFx, NULL);
    iVs = iFx->streams[0];
    iCx = avcodec_alloc_context3(iC);
    avcodec_parameters_to_context(iCx, iVs->codecpar);
    avcodec_open2(iCx, iC, NULL);
    avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    oVs = avformat_new_stream(oFx, oC); 
    oCx = avcodec_alloc_context3(oC);
    oCx->width = iCx->width;
    oCx->height = iCx->height;
    oCx->pix_fmt = AV_PIX_FMT_YUV420P;
    oCx->time_base = (AVRational){1,9};
    oCx->framerate = av_inv_q(oCx->time_base);
    oCx->profile = FF_PROFILE_H264_HIGH;
    oCx->field_order = AV_FIELD_PROGRESSIVE;
    av_opt_set(oCx->priv_data, "x264-params", "force-cfr=1", 0);
    avcodec_open2(oCx, oC, NULL);
    avcodec_parameters_from_context(oVs->codecpar, oCx);
    oVs->time_base = oCx->time_base;
    oVs->avg_frame_rate = av_inv_q(oVs->time_base);
    oVs->r_frame_rate = oVs->avg_frame_rate;
    avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    avformat_write_header(oFx, NULL);
    char tmp[100];
    snprintf(tmp, sizeof(tmp), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", iCx->width, iCx->height, iCx->pix_fmt, iVs->time_base.num, iVs->time_base.den, iCx->sample_aspect_ratio.num, iCx->sample_aspect_ratio.den);
    avfilter_graph_create_filter(&inFx, inF, "in", tmp, NULL, fG);
    avfilter_graph_create_filter(&fpsFx, fpsF, "fps", "fps=9", NULL, fG);
    avfilter_graph_create_filter(&outFx, outF, "out", NULL, NULL, fG);
    avfilter_link(inFx, 0, fpsFx, 0);
    avfilter_link(fpsFx, 0, outFx, 0);
    avfilter_graph_config(fG, NULL);
    const int step = oVs->time_base.den / oCx->time_base.den;
    int Fcount = 0, Pcount = 0;
    bool isFlush = false;
    while(1){
        int ret = av_read_frame(iFx, iP);
        if(ret == AVERROR_EOF) {
            iP = NULL;
        }else if(ret < 0){
            return(false);
        }
        ret = avcodec_send_packet(iCx, iP);
        if(iP){
            av_packet_unref(iP);
        }
        if(ret == AVERROR_EOF){
            break;
        }
        else if(ret < 0) {
            return(false);
        }
        while(1){
            ret = avcodec_receive_frame(iCx, iF);
            if(ret == AVERROR(EAGAIN)){
                break;
            }else if(ret == AVERROR_EOF){
                iF = NULL;
            }else if(ret < 0){
                return(false);
            }
            ret = av_buffersrc_add_frame(inFx, iF);
            if(iF){
                av_frame_unref(iF);
            }
            if(ret < 0){
                return(false);
            }
            while(1){
                ret = av_buffersink_get_frame(outFx, oF);
                if(ret == AVERROR(EAGAIN)){
                    break;
                }else if(ret == AVERROR_EOF){
                    oF = NULL;
                }else if(ret < 0){
                    return(false);
                }
                if(oF){
                    oF->pts = Fcount * step;
                    Fcount++;
                    oF->pict_type = AV_PICTURE_TYPE_NONE;
                }
                ret = avcodec_send_frame(oCx, oF);
                if(oF){
                    av_frame_unref(oF);
                }
                if(ret < 0){
                    return(false);
                }
                while(1){
                    ret = avcodec_receive_packet(oCx, oP);
                    if(ret == AVERROR(EAGAIN)){
                        break;
                    }else if(ret == AVERROR_EOF){
                        oP = NULL;
                        isFlush = true;
                    }else if(ret < 0){
                        return(false);
                    }
                    if(oP){
                        oP->pts = Pcount * step;
                        oP->dts = (Pcount - 1) * step;
                        oP->duration = step;
                        Pcount++;
                    }
                    ret = av_interleaved_write_frame(oFx, oP);
                    if(oP){
                        av_packet_unref(oP);
                    }
                    if(isFlush){
                        goto end;
                    }
                }
            }
        }
    }
    end:
    av_write_trailer(oFx);
    avformat_close_input(&iFx);
    avio_closep(&oFx->pb);
    avcodec_close(iCx);
    avcodec_close(oCx);
    av_packet_free(&iP);
    av_packet_free(&oP);
    av_frame_free(&iF);
    av_frame_free(&oF);
    avformat_free_context(iFx);
    avformat_free_context(oFx);
    avcodec_free_context(&iCx);
    avcodec_free_context(&oCx);
    avfilter_graph_free(&fG);
    return(true);
}