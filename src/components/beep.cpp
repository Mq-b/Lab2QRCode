#include "beep.h"

#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>

// 使用 Qt 的音频输出播放扫描提示音
void playBeep() {
    static QAudioOutput *audioOutput = nullptr;
    static QBuffer *buffer = nullptr;

    if (!audioOutput) {
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(1);
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        audioOutput = new QAudioOutput(format);
    }

    if (!buffer) {
        QByteArray data = QByteArray::fromRawData(
            reinterpret_cast<const char *>(pcmScanBeep.data()),
            static_cast<int>(pcmScanBeep.size() * static_cast<std::size_t>(sizeof(int16_t))));

        buffer = new QBuffer;
        buffer->setData(data);
        buffer->open(QIODevice::ReadOnly);
    }

    // 重新从头播放
    buffer->seek(0);
    audioOutput->stop();
    audioOutput->start(buffer);
}
