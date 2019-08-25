#include "myplay.h"
#include<QDebug>

MyPlay::MyPlay()
{
    this->load_buf.clear();
    this->play_buf.clear();
    this->hasPause=false;
}
void MyPlay::slt_play(char*url)
{
    qDebug("[###url]:%s\n",url);
    this->m_url.clear();
    this->m_url.append(url);
    this->hasUrl=true;
}

void MyPlay::slt_play_all(QString cmd)
{
    qDebug("[cmd:%s]\n",cmd.toLatin1().data());
    if(cmd.indexOf("play_all")!=-1)
    {
        this->hasPlay_all=true;
      if(hasPause) hasPause=!hasPause;
    }
}

void MyPlay::slt_load(char*url,QString cmd)
{
    qDebug("[###url]:%s---cmd:%s\n",url,cmd.toLatin1().data());
    this->m_url.clear();
    this->m_url.append(url);
    this->hasLoad=true;
}

void MyPlay::slt_pause()
{
    qDebug("received pause!");
    hasPause=!hasPause;
}

int MyPlay::pcm16le_load(char *url) //load channel_l
{
    char* output="output.pcm";
    transcoding(url,output);
    FILE *fp=fopen(output,"rb+");
    char *sample=static_cast<char*>(malloc(4));
    qint16 val=0;
    int cnt=0;
    while(!feof(fp)){
        fread(sample,1,4,fp);
        cnt++;
        val=sample[1]*256+sample[0];
        load_buf.append(val);
    }
    qDebug("[out of loop Cnt:%d\n",cnt);
    emit sig_duration(m_dur);
    emit sig_wave_vector(load_buf);
    //  play_partial(va,700000,800000,m_dur);
    play_buf.swap(load_buf);
    load_buf.clear();
    free(sample);
    fclose(fp);
    return 0;
}
void MyPlay::run()
{
    while(true)
    {
//        qDebug()<<"func:"<<__FUNCTION__<<"is called"<<endl;
        if(hasLoad)
        {
            hasLoad=false;
            this->pcm16le_load(this->m_url.toLatin1().data());
        }
        if(hasPlay_all)
        {
            hasPlay_all=false;
            this->playall(this->play_buf,this->m_dur);
        }
    }
}


void MyPlay::transcoding(char* filename,char*output)
{
    AVFormatContext	*pFormatCtx;
    int				i, audioStream;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVPacket		*packet;
    uint8_t			*out_buffer;
    AVFrame			*pFrame;
    int ret;
    int index = 0;
    int got_picture;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;
    FILE *pFile=nullptr;
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    //Open
    if(avformat_open_input(&pFormatCtx,filename,nullptr,nullptr)!=0){
        printf("Couldn't open input stream.\n");
        return;
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx,nullptr)<0){
        printf("Couldn't find stream information.\n");
        return ;
    }
    // Dump valid information onto standard error
    av_dump_format(pFormatCtx, 0, filename, false);
    this->m_dur=pFormatCtx->duration;
    qDebug("durtion:[%ld]\n",this->m_dur);
    // Find the first audio stream
    audioStream=-1;
    int dAudioTimeBase;
    for(i=0; i < pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){

            audioStream=i;
            dAudioTimeBase = av_q2d(pFormatCtx->streams[i]->time_base) * 1000;
            break;
        }

    if(audioStream==-1){
        printf("Didn't find a audio stream.\n");
        return;
    }

    // Get a pointer to the codec context for the audio stream
    pCodecCtx=pFormatCtx->streams[audioStream]->codec;

    // Find the decoder for the audio stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==nullptr){
        printf("Codec not found.\n");
        return;
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec,nullptr)<0){
        printf("Could not open codec.\n");
        return;
    }

    pFile=fopen(output, "wb");

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    //Out Audio Param
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples=pCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=44100;
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size
    int out_buffer_size=av_samples_get_buffer_size(nullptr,out_channels ,out_nb_samples,out_sample_fmt, 1);

    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
    pFrame=av_frame_alloc();
    //    printf("Bitrate: %3d\n", pFormatCtx->bit_rate);
    //    printf("Codec Name: %s\n", pCodecCtx->codec->long_name);
    //    printf("Channels:  %d \n", pCodecCtx->channels);
    //    printf("Sample per Second  %d \n", pCodecCtx->sample_rate);

    //FIX:Some Codec's Context Information is missing
    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
    //Swr
    au_convert_ctx = swr_alloc();
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
                                      in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
    swr_init(au_convert_ctx);
    while(av_read_frame(pFormatCtx, packet)>=0){
        if(packet->stream_index==audioStream){
            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
            if ( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                return ;
            }
            if ( got_picture > 0 ){
                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
                //#if 1
                //                printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
                //#endif
                //Write PCM
                fwrite(out_buffer, 1, out_buffer_size, pFile);

                index++;
            }
        }
        av_free_packet(packet);
    }
    swr_free(&au_convert_ctx);
    fclose(pFile);
    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}
//int  MyPlay::playall(QVector<qint16>&v,quint64 duration) {
//    if(v.isEmpty()){
//        qDebug("[pcm data is empty!]");
//        return 0;
//    }
//    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//        qDebug("[sdl could not initialized with error:%s]\n",SDL_GetError());
//        return -1;
//    }
//    SDL_AudioSpec desired_spec;
//    desired_spec.freq = 44100;
//    desired_spec.format = AUDIO_S16SYS;
//    desired_spec.channels = 2;
//    desired_spec.silence = 0;
//    desired_spec.samples = 1024;
//    desired_spec.size=4;
//    desired_spec.callback =nullptr;
//    SDL_AudioDeviceID deviceID;
//    if ((deviceID = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
//        qDebug("[open audio device with error device id %s]\n",deviceID);
//        return -1;
//    }
//    SDL_PauseAudioDevice(deviceID, 0);
////    qDebug("v.size:%d\n",v.size());
//    for(int inx=0;inx<v.size();++inx)
//    {
//        SDL_QueueAudio(deviceID, &v[inx],4);

//        qDebug("#%d",hasPause);
//        if(hasPause)
//            SDL_PauseAudioDevice(deviceID, 1);
//        else
//           SDL_PauseAudioDevice(deviceID, 0);
//        qDebug()<<__FUNCTION__<<"is called"<<endl;
//    }
////    Uint32 dur=static_cast<Uint32>(duration/1000); //the data translate must be careful;
////    SDL_Delay(dur);
//    SDL_Delay(1000);
//    SDL_CloseAudio();
//    SDL_Quit();
//}
int  MyPlay::playall(QVector<qint16>&v,quint64 duration) {
    if(v.isEmpty()){
        qDebug("[pcm data is empty!]");
        return 0;
    }
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug("[sdl could not initialized with error:%s]\n",SDL_GetError());
        return -1;
    }
    SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.silence = 0;
    desired_spec.samples = 1024;
    desired_spec.size=4;
    desired_spec.callback =nullptr;
    SDL_AudioDeviceID deviceID;
    if ((deviceID = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
        qDebug("[open audio device with error device id %s]\n",deviceID);
        return -1;
    }
    SDL_PauseAudioDevice(deviceID, 0);
//    qDebug("v.size:%d\n",v.size());
    for(int inx=0;inx<v.size();++inx)
    {
        SDL_QueueAudio(deviceID, &v[inx],4);

//        qDebug("#%d",hasPause);
        if(hasPause)
            SDL_PauseAudioDevice(deviceID, 1);
        else
           SDL_PauseAudioDevice(deviceID, 0);
//        qDebug()<<__FUNCTION__<<"is called"<<endl;
    }
   Uint32 dur=static_cast<Uint32>(duration/1000); //the data translate must be careful;
    SDL_Delay(dur);
//    SDL_Delay(1000);
    SDL_CloseAudio();
    SDL_Quit();
}

int  MyPlay::play_partial(QVector<qint16>&v,quint64 start,quint64 end,quint64 duration) {

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug("[sdl could not initialized with error:%s]\n",SDL_GetError());
        return -1;
    }
    SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.silence = 0;
    desired_spec.samples = 1024;
    desired_spec.size=4;
    desired_spec.callback =nullptr;

    SDL_AudioDeviceID deviceID;
    if ((deviceID = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
        qDebug("[open audio device with error device id %d]\n",deviceID);
        return -1;
    }
    SDL_PauseAudioDevice(deviceID, 0);
    for(quint64 inx=start;inx<end;++inx)
    {
        SDL_QueueAudio(deviceID, &v[inx],4);
    }
    qreal sz=static_cast<qreal>(v.size());
    qreal gap=static_cast<qreal>(end-start);
    qreal partial_duration=(gap/sz)*static_cast<qreal>(duration)*1000.00;
    SDL_Delay(static_cast<quint32>(partial_duration));
    SDL_CloseAudio();
    SDL_Quit();
}
int  MyPlay::play_all(QVector<qint16>&buff) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug("[sdl could not initialized with error:%s]\n",SDL_GetError());
        return -1;
    }
    SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.silence = 0;
    desired_spec.samples =1024;
    //    desired_spec.size=1;
    desired_spec.callback = nullptr;
    Uint32 buffer_size = 2;
    char *buffer = static_cast<char*>(malloc(buffer_size));
    SDL_AudioDeviceID deviceID;
    if ((deviceID = SDL_OpenAudioDevice(NULL, 0, &desired_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
        qDebug("[open audio device with error device id %s]\n",deviceID);
        return -1;
    }
    SDL_PauseAudioDevice(deviceID, 0);//start play if param is false(0)
    for(int i=0;i<buff.count();++i)
    {
        //        SDL_QueueAudio(deviceID,(const void*)(&buff[i]),1);
        buff[0]=buff[i]&0x00ff;
        buff[1]=buff[i]>>8;
        SDL_QueueAudio(deviceID, buffer, buffer_size);
    }
    SDL_Delay(20000);
    SDL_CloseAudio();
    SDL_Quit();
}

void  fill_audio(void *udata,Uint8 *stream,int len)
{
    SDL_memset(stream, 0, len);//将stream 清零
    if(audio_len==0)
        return;
    len=(len>audio_len?audio_len:len);	/*  Mix  as  much  data  as  possible  */
    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

//void MyPlay::play_with_SDL2(char*url)
//{
////    AVFormatContext	*pFormatCtx;
////    int				i, audioStream;
////    AVCodecContext	*pCodecCtx;
////    AVCodec			*pCodec;
////    AVPacket		*packet;
////    uint8_t			*out_buffer;
////    AVFrame			*pFrame;
////    SDL_AudioSpec wanted_spec;
////    int ret;
////    uint32_t len = 0;
////    int got_picture;
////    int index = 0;
////    int64_t in_channel_layout;
////    struct SwrContext *au_convert_ctx;
////    FILE *pFile=NULL;
////    av_register_all();
////    avformat_network_init();
////    pFormatCtx = avformat_alloc_context();
////    //Open
////    if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){
////        printf("Couldn't open input stream.\n");
////        return ;
////    }
////    // Retrieve stream information
////    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
////        printf("Couldn't find stream information.\n");
////        return ;
////    }
////    //Dump valid information onto standard error
////    av_dump_format(pFormatCtx, 0, url, false);
////    // Find the first audio stream
////    audioStream=-1;
////    for(i=0; i < pFormatCtx->nb_streams; i++)
////        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
////            audioStream=i;
////            break;
////        }

////    if(audioStream==-1){
////        printf("Didn't find a audio stream.\n");
////        return ;
////    }

////    // Get a pointer to the codec context for the audio stream
////    pCodecCtx=pFormatCtx->streams[audioStream]->codec;

////    // Find the decoder for the audio stream
////    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
////    if(pCodec==NULL){
////        printf("Codec not found.\n");
////        return ;
////    }
////    // Open codec
////    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
////        printf("Could not open codec.\n");
////        return ;
////    }
//    pFile=fopen("output.pcm", "wb");
//    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
//    av_init_packet(packet);

//    //Out Audio Param
//    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
//    //nb_samples: AAC-1024 MP3-1152
//    int out_nb_samples=pCodecCtx->frame_size;
//    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
//    int out_sample_rate=44100;
//    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
//    //Out Buffer Size
//    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

//    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
//    pFrame=av_frame_alloc();
//#if USE_SDL
//    //Init
//    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
//        printf( "Could not initialize SDL - %s\n", SDL_GetError());
//        return ;
//    }
//    //SDL_AudioSpec
//    wanted_spec.freq = out_sample_rate;
//    wanted_spec.format = AUDIO_S16SYS;
//    wanted_spec.channels = out_channels;
//    wanted_spec.silence = 0;
//    wanted_spec.samples = out_nb_samples;
//    wanted_spec.callback = fill_audio;
//    wanted_spec.userdata = pCodecCtx;

//    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
//        printf("can't open audio.\n");
//        return ;
//    }
//#endif

//    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
//    au_convert_ctx = swr_alloc();
//    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
//                                      in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
//    swr_init(au_convert_ctx);
//    SDL_PauseAudio(0);

//    while(av_read_frame(pFormatCtx, packet)>=0)
//    {
//        if(packet->stream_index==audioStream)
//        {
//            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
//            if ( ret < 0 ) {
//                printf("Error in decoding audio frame.\n");
//                return ;
//            }
//            if ( got_picture > 0 ){
//                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);

////#if OUTPUT_PCM
////                //Write PCM
////                fwrite(out_buffer, 1, out_buffer_size, pFile);
////#endif
//                index++;
//            }
//            while(audio_len>0)//Wait until finish
//                SDL_Delay(1);

//            //Set audio buffer (PCM data)
//            audio_chunk = (Uint8 *) out_buffer;
//            //Audio buffer length
//            audio_len =out_buffer_size;
//            audio_pos = audio_chunk;
////            emit sig_wave((char*)audio_pos);
//        }
//        av_free_packet(packet);
//    }
//    swr_free(&au_convert_ctx);
//    SDL_CloseAudio();//Close SDL
//    SDL_Quit();
//    av_free(out_buffer);
//    avcodec_close(pCodecCtx);
//    avformat_close_input(&pFormatCtx);
//}

void MyPlay::play_with_SDL2(char*url)
{
    AVFormatContext	*pFormatCtx;
    int				i, audioStream;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVPacket		*packet;
    uint8_t			*out_buffer;
    AVFrame			*pFrame;
    SDL_AudioSpec wanted_spec;
    int ret;
    uint32_t len = 0;
    int got_picture;
    int index = 0;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;
    FILE *pFile=NULL;
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    //Open
    if(avformat_open_input(&pFormatCtx,url,nullptr,NULL)!=0){
        printf("Couldn't open input stream.\n");
        return ;
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return ;
    }
    //Dump valid information onto standard error
    av_dump_format(pFormatCtx, 0, url, false);
    //    this->m_dur=pFormatCtx->duration;
    //qDebug("durtion:[%ld]\n",this->m_dur);

    // Find the first audio stream
    audioStream=-1;
    for(i=0; i < pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStream=i;
            break;
        }

    if(audioStream==-1){
        printf("Didn't find a audio stream.\n");
        return ;
    }

    // Get a pointer to the codec context for the audio stream
    pCodecCtx=pFormatCtx->streams[audioStream]->codec;

    // Find the decoder for the audio stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return ;
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        printf("Could not open codec.\n");
        return ;
    }
    pFile=fopen("output.pcm", "wb");
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    //Out Audio Param
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples=pCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=44100;
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size
    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
    pFrame=av_frame_alloc();
#if USE_SDL
    //Init
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return ;
    }
    //SDL_AudioSpec
    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = pCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        printf("can't open audio.\n");
        return ;
    }
#endif

    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
    au_convert_ctx = swr_alloc();
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
                                      in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
    swr_init(au_convert_ctx);
    SDL_PauseAudio(0);

    while(av_read_frame(pFormatCtx, packet)>=0)
    {
        if(packet->stream_index==audioStream)
        {
            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
            if ( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                return ;
            }
            if ( got_picture > 0 ){
                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
                //#if 1
                //                printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
                //#endif

#if OUTPUT_PCM
                //Write PCM
                fwrite(out_buffer, 1, out_buffer_size, pFile);
#endif
                index++;
            }
            while(audio_len>0)//Wait until finish
                SDL_Delay(1);

            //Set audio buffer (PCM data)
            audio_chunk = (Uint8 *) out_buffer;
            //Audio buffer length
            audio_len =out_buffer_size;
            audio_pos = audio_chunk;
            emit sig_wave((char*)audio_pos);
        }
        av_free_packet(packet);
    }
    swr_free(&au_convert_ctx);
    SDL_CloseAudio();//Close SDL
    SDL_Quit();
    fclose(pFile);
    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}
