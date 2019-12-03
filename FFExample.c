#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int rC = -1;

void chk(int ret, const char* eMsg){
    if(ret < 0){
        printf(eMsg);
        exit(rC);
    }else{
        rC++;
    }
}

int findVideoStream(AVFormatContext *ctx){
    int ret = -1;
    for (int i = 0; i < ctx->nb_streams; i++){
        AVCodecParameters *tmp = NULL;
        tmp = ctx->streams[i]->codecpar;
        if(tmp->codec_type == AVMEDIA_TYPE_VIDEO){
            ret = i;
            break;
        }
    }
    return(ret);
}

void transcode(const char* iFilename, const char* oFilename){
    int ret = -1;
    AVFormatContext *iFx = avformat_alloc_context(), *oFx = avformat_alloc_context();
    if(iFx == NULL | oFx == NULL){
        chk(-1, "Не удалось зарезервировать необходимую память");
    }
    ret = avformat_open_input(&iFx, iFilename, NULL, NULL);
    chk(ret, "Ошибка открытия входного файла!");
    int vSn = findVideoStream(iFx);
    chk(vSn, "Во входной файле не найден видео-поток");
    ret = avformat_alloc_output_context2(&oFx, NULL, NULL, oFilename);
    chk(ret, "Ошибка открытия выходного файла");
    ret = avformat_find_stream_info(iFx, NULL);
    chk(ret, "Не удалось распознать тип видео-потока");
    AVCodecParameters *iCp = iFx->streams[vSn]->codecpar, *oCp = avcodec_parameters_alloc();
    if(iCp == NULL | oCp == NULL){
        chk(-1, "Не удалось зарезервировать необходимую память");
    }
    AVCodec *iC = avcodec_find_decoder(iCp->codec_id), *oC = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(iC == NULL){
        chk(-1, "Не удалось обнаружить подходящий декодер");
    }
    if(oC = NULL){
        chk(-1, "Не удалось обнаружит необходимый кодировщик");
    }
    AVCodecContext *iCx = avcodec_alloc_context3(iC), *oCx = avcodec_alloc_context3(oC);
    if(iCx == NULL | oCx == NULL){
        chk(-1, "Не удалось зарезервировать необходимую память");
    }
    ret = avcodec_parameters_to_context(iCx, iCp);
    chk(ret, "Не удалось присвоть параметры кодека контексту ввода");
    ret = avcodec_parameters_to_context(oCx, oCp);
    chk(ret, "Не удалось присвоть параметры кодека контексту вывода");
    ret = avcodec_open2(iCx, iC, NULL);
    chk(ret, "Не удалось инициализировать декодер");
    AVDictionary *oCo = NULL;
    ret = av_dict_parse_string(&oCo, "profile=baseline", "=", ",", AV_DICT_APPEND);
    chk(ret, "Не удалось установить параметры кодировщика");
    ret = avcodec_open2(oCx, oC, &oCo);
    chk(ret, "Не удалось инициализировать кодировщик");
    AVStream *iVs = iFx->streams[vSn];
    if(iVs == NULL){
        chk(-1, "Не удалось установить входной поток");
    }
    AVStream *oVs = avformat_new_stream(oFx, NULL);
    if(oVs == NULL){
        chk(-1, "Не удалось установить выходной потко");
    }
    ret = avio_open(&oFx->pb, oFilename, AVIO_FLAG_WRITE);
    chk(ret, "Не удалось открыть выходной файл для записи");
    ret = avformat_write_header(oFx, NULL);
    chk(ret, "Не удалось записать заголовок в выходной файл");
    while(1){
        AVPacket *iP = av_packet_alloc(), *oP = av_packet_alloc();
        AVFrame *iF = av_frame_alloc();
        if(iP == NULL | oP == NULL | iF == NULL){
            chk(-1, "Не удалось зарезервировать необходимую память");
        }
        if(av_read_frame(iFx, iP) < 0){
            break;
        }
        if(iP->stream_index != vSn){
            av_packet_unref(iP);
            continue;
        }
        ret = avcodec_send_packet(iCx, iP);
        chk(ret, "Не удалось послать пакет декодеру");
        ret = avcodec_receive_frame(iCx, iF);
        chk(ret, "Не удалось получить кадр от декодера");
        ret = avcodec_send_frame(oCx, iF);
        chk(ret, "Не удалось послать кадр кодировщику");
        ret = avcodec_receive_packet(oCx, oP);
        chk(ret, "Не удалось получить кадр от кодировщика");
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
}

int main(int argc, char **argv)
{
    transcode("bbb_sunflower_1080p_30fps_normal.mp4", "out.mp4");
    return 0;
}