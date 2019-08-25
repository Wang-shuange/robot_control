#ifndef MYPLAY_H
#define MYPLAY_H
#include<QThread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<qdebug.h>
#include<qpainter.h>
#include <QtCharts/QChartGlobal>
#include<qchart.h>
#include<QtCharts>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
};
#endif
#endif

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

//Output PCM
#define OUTPUT_PCM 1
//Use SDL
#define USE_SDL 1

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;
class MyPlay : public QThread
{
     Q_OBJECT
public slots:
    void slt_play(char *url);
    void slt_play_all(QString cmd);
    void slt_load(char *url,QString cmd);
    void slt_pause();

    int play_all(QVector<qint16> &buff);
public:
    MyPlay();
    int play_ex(char *url);
    void transcoding(char *filename, char *output);
    QString m_url;
    bool hasUrl=false;
    bool hasLoad=false;
    bool hasPlay_all=false;
    bool hasPause=false;
    quint64 m_dur;
    QVector<qint16> load_buf;
    QVector<qint16> play_buf;

    void play_with_SDL2(char*url);
    int pcm16le_load(char *url);
    int playall(QVector<qint16>&v,quint64 duration);
    int play_partial(QVector<qint16> &v, quint64 start, quint64 end, quint64 duration);
protected:
    void run();
signals:
    void sig_wave(char*value);
    void sig_wave_big(QString value);
    void sig_wave_array(qint16* wav_array,quint64 len);
    void sig_wave_vector(QVector<qint16>val);
    void sig_duration(quint64 dur);
    void sig_position(quint64 pos);

};

#endif // MYPLAY_H
