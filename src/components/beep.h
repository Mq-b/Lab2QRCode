#pragma once

#include <array>
#include <cstdint>
#include <numbers>

constexpr double sin_approx(double x) {
    constexpr auto pi = std::numbers::pi;
    constexpr double two_pi = 2.0 * pi;

    long long k = static_cast<long long>(x / two_pi);
    x -= static_cast<double>(k) * two_pi;
    if (x > pi) {
        x -= two_pi;
    } else if (x < -pi) {
        x += two_pi;
    }

    double x2 = x * x;
    return x * (1 - x2 / 6 + x2 * x2 / 120);
}

// 生成两段“滴滴”的扫描提示音（constexpr）
template <std::size_t sampleRate>
constexpr auto make_scan_beep() {
    // 单个 beep 和间隔的时长（毫秒）
    constexpr std::size_t beepMs = 60;
    constexpr std::size_t gapMs = 40;
    constexpr std::size_t beepSamples = sampleRate * beepMs / 1000;
    constexpr std::size_t gapSamples = sampleRate * gapMs / 1000;
    constexpr std::size_t totalSamples = beepSamples * 2 + gapSamples;

    std::array<int16_t, totalSamples> data{};

    // 稍高一点的提示音频率
    constexpr double freq = 1200.0;
    constexpr double volume = 0.7;

    // 包络：前 5ms 渐入，最后 10ms 渐出，避免“啪”的瞬态
    constexpr std::size_t attackSamples = sampleRate * 5 / 1000;
    constexpr std::size_t releaseSamples = sampleRate * 10 / 1000;

    for (std::size_t i = 0; i < totalSamples; ++i) {
        double value = 0.0;
        bool inBeep = false;
        std::size_t idxInBeep = 0;

        if (i < beepSamples) {
            // 第一声滴
            inBeep = true;
            idxInBeep = i;
        } else if (i >= beepSamples + gapSamples && i < beepSamples + gapSamples + beepSamples) {
            // 第二声滴
            inBeep = true;
            idxInBeep = i - (beepSamples + gapSamples);
        }

        if (inBeep) {
            double t = static_cast<double>(idxInBeep) / static_cast<double>(sampleRate);
            double env = 1.0;

            if (idxInBeep < attackSamples && attackSamples > 0) {
                env = static_cast<double>(idxInBeep) / static_cast<double>(attackSamples);
            } else if (idxInBeep >= beepSamples - releaseSamples && releaseSamples > 0 && idxInBeep < beepSamples) {
                std::size_t relIdx = beepSamples - idxInBeep;
                if (relIdx < releaseSamples) {
                    env = static_cast<double>(relIdx) / static_cast<double>(releaseSamples);
                }
            }

            value = env * sin_approx(2.0 * std::numbers::pi * freq * t);
        }

        double v = value * volume * 32767.0;
        if (v > 32767.0) {
            v = 32767.0;
        }
        if (v < -32768.0) {
            v = -32768.0;
        }
        data[i] = static_cast<int16_t>(v);
    }

    return data;
}

// 扫描成功提示音：两声短“滴滴”
constexpr auto pcmScanBeep = make_scan_beep<44100>();

// 播放一次 beep（使用 Qt 音频输出）
void playBeep();
