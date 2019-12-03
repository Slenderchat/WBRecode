#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

void chk(int ret, const char* eMsg){
    if(ret < 0){
        printf("%s: %d\n", eMsg, ret);
        exit(ret);
    }
}

int findVideoStream(AVFormatContext *ctx){
    int ret = -1;
    for (int i = 0; i < ctx->nb_streams; i++){
        if(ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            ret = i;
            break;
        }
    }
    return(ret);
}

void transcode(const char* iFilename, const char* oFilename){
    int ret = -1;
    AVPacket *iP = av_packet_alloc(), *oP = av_packet_alloc();
    AVFrame *iF = av_frame_alloc();
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = avformat_alloc_context();
    AVInputFormat *iFm = av_find_input_format("mp4");
    AVOutputFormat *oFm = av_guess_format("mp4", oFilename, NULL);
    AVCodec *iC = avcodec_find_decoder(AV_CODEC_ID_H264), *oC = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *iCx = avcodec_alloc_context3(iC), *oCx = avcodec_alloc_context3(oC);
    if (iFm == NULL || oFm == NULL){
        chk(-1, "Окружение не поддерживает MPEG-4");
    }
    if(iC == NULL || oC == NULL){
        chk(-1, "Не удалось обнаружить необходимые кодеки");
    }
    ret = avformat_alloc_output_context2(&oFx, oFm, NULL, oFilename);
    chk(ret, "Ошибка открытия выходного файла");
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    ret = avformat_open_input(&iFx, iFilename, NULL, NULL);
    chk(ret, "Ошибка открытия входного файла!");
    int vSn = findVideoStream(iFx);
    chk(vSn, "Во входной файле не найден видео-поток");
    ret = avformat_find_stream_info(iFx, NULL);
    chk(ret, "Не удалось распознать тип видео-потока");
    AVStream *iVs = iFx->streams[vSn];
    AVCodecParameters *iCp = iFx->streams[vSn]->codecpar, *oCp = iFx->streams[vSn]->codecpar;
    if(iCp->codec_id != AV_CODEC_ID_H264){
        chk(-1, "Входной поток не является потоком H264");
    }
    avcodec_parameters_to_context(iCx, iCp);
    avcodec_parameters_to_context(oCx, oCp);
    oCx->width = 1280;
    oCx->height = 720;
    oCx->time_base = (AVRational){1,10};
    oCx->framerate = (AVRational){10,1};
    av_opt_set(oCx->priv_data, "profile", "baseline", 0);
    av_opt_set(oCx->priv_data, "level", "31", 0);
    av_opt_set(oCx->priv_data, "crf", "21", 0);
    avcodec_parameters_from_context(oVs->codecpar, oCx);
    ret = avcodec_open2(iCx, iC, NULL);
    chk(ret, "Не удалось инициализировать декодер");
    ret = avcodec_open2(oCx, oC, NULL);
    chk(ret, "Не удалось инициализировать кодировщик");
    ret = avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    chk(ret, "Не удалось открыть выходной файл для записи");
    ret = avformat_write_header(oFx, NULL);
    chk(ret, "Не удалось записать заголовок в выходной файл");
    while(av_read_frame(iFx, iP) >= 0){
        if(iP->stream_index != vSn){
            av_packet_unref(iP);
            continue;
        }
        ret = avcodec_send_packet(iCx, iP);
        chk(ret, "Ошибка декодирования");
        ret = avcodec_receive_frame(iCx, iF);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }
        chk(ret, "Ошибка декодирования");
        ret = avcodec_send_frame(oCx, iF);
        chk(ret, "Ошибка кодирования");
        ret = avcodec_receive_packet(oCx, oP);
        if(ret == AVERROR(EAGAIN)){
            continue;
        }
        chk(ret, "Ошибка кодирования");
        oP->pts = av_rescale_q_rnd(oP->pts, iVs->time_base, oVs->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        oP->dts = av_rescale_q_rnd(oP->dts, iVs->time_base, oVs->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        oP->duration = av_rescale_q(oP->duration, iVs->time_base, oVs->time_base);
        oP->pos = -1;
        ret = av_interleaved_write_frame(oFx, oP);
        chk(ret, "Не удалось записать кадр в выходной файл");
        av_packet_unref(oP);
    }
    ret = av_write_trailer(oFx);
    chk(ret, "Не удалось записать финализировать выходной файл");
    avformat_close_input(&iFx);
    av_packet_free(&iP);
    av_packet_free(&oP);
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