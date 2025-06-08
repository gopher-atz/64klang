//
// Copyright (C) Tammo Hinrichs 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE.md file for full license information
//

// simple WASAPI sound output

#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

// stuff lifted from Capturinha because I'm lazy

#define CHECK(x) { HRESULT _hr=(x); if(FAILED(_hr)) DebugBreak(); }

template <typename T> class RCPtr
{
    T* ptr = nullptr;
public:
    constexpr RCPtr() : ptr(nullptr) {}
    RCPtr(const RCPtr& p) : ptr(p.ptr) { if (ptr) ptr->AddRef(); }
    constexpr RCPtr(RCPtr&& p) : ptr(p.ptr) { p.ptr = nullptr; }
    constexpr RCPtr(T* p) { ptr = p; }

    // this works with COM or if your class implements it manually
    template <typename T2> RCPtr(const RCPtr<T2>& pp) : ptr(nullptr)
    {
        if (pp.IsValid()) pp->QueryInterface(__uuidof(T), (void**)&ptr);
    }

    ~RCPtr() { Clear(); }

    RCPtr& operator = (const RCPtr& p) { Clear(); ptr = p.ptr; if (ptr) ptr->AddRef(); return *this; }
    RCPtr& operator = (RCPtr&& p) noexcept { Clear(); ptr = p.ptr; p.ptr = nullptr; return *this; }
    template <typename T2> RCPtr& operator =(const RCPtr<T2>& pp)
    {
        Clear();
        if (pp.IsValid()) pp->QueryInterface(__uuidof(T), (void**)&ptr);
        return *this;
    }

    void Clear() { if (ptr) { ptr->Release(); ptr = 0; } }
    constexpr bool IsValid() const { return ptr != nullptr; }
    constexpr bool operator !() const { return !IsValid(); }

    T* operator -> () const { return ptr; }
    T& Ref() const { return *ptr; }

    // casts
    constexpr operator T* () const { return ptr; }
    constexpr operator void** () { Clear(); return (void**)&ptr; }
    constexpr operator T** () { Clear(); return &ptr; }
};

template<typename T> constexpr T Min(T a, T b) { return a < b ? a : b; }
template<typename T> constexpr T Max(T a, T b) { return a > b ? a : b; }
template<typename T> constexpr T Clamp(T v, T min, T max) { return Min(Max(v, min), max); }

typedef void (*AudioCallback)(float* buffer, int samples);

namespace AudioOut {
    static HANDLE Thread;
    static volatile bool running = true;
    static AudioCallback cb;
    static int rate;

    static DWORD WINAPI ThreadFunc(void*)
    {
        // init COM
        CHECK(CoInitializeEx(NULL, COINIT_MULTITHREADED));

        // Acquire Device enumerator
        RCPtr<IMMDeviceEnumerator> enumerator;
        CHECK(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), enumerator));

        // Get default endpoint
        RCPtr<IMMDevice> device;
        CHECK(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, device));

        WAVEFORMATEXTENSIBLE waveFormat = {};
        waveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        waveFormat.Format.nChannels = 2;
        waveFormat.Format.nSamplesPerSec = rate;
        waveFormat.Format.wBitsPerSample = 32;
        waveFormat.Format.nBlockAlign = waveFormat.Format.nChannels * waveFormat.Format.wBitsPerSample / 8;
        waveFormat.Format.nAvgBytesPerSec = waveFormat.Format.nSamplesPerSec * waveFormat.Format.nBlockAlign;
        waveFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        waveFormat.Samples.wValidBitsPerSample = 32;
        waveFormat.dwChannelMask = (1 << waveFormat.Format.nChannels) - 1; // again, very space opera
        waveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        // initialize playback client
        UINT bufferSize = 0;
        RCPtr<IAudioClient> client;
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, client));

        // let's get the latency as low as we can
        REFERENCE_TIME period = 100000;
        CHECK(client->GetDevicePeriod(NULL, &period));

        CHECK(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, period, 0, (WAVEFORMATEX*)&waveFormat, NULL));
        CHECK(client->SetEventHandle(eventHandle));
        CHECK(client->GetBufferSize(&bufferSize));
        RCPtr<IAudioRenderClient> renderClient;
        CHECK(client->GetService(__uuidof(IAudioRenderClient), renderClient));
        CHECK(client->Start());

        while (running)
        {
            if (WaitForSingleObject(eventHandle, 1000) == 0xffffffff)
                break; // something is wrong, bail

            UINT32 padding;
            client->GetCurrentPadding(&padding);
            UINT32 framesAvailable = bufferSize - padding;

            if (framesAvailable > 0)
            {
                BYTE* buffer{};
                CHECK(renderClient->GetBuffer(framesAvailable, &buffer));
                cb((float*)buffer, framesAvailable);
                CHECK(renderClient->ReleaseBuffer(framesAvailable, 0));
            }
        }

        client->Stop();
        CloseHandle(eventHandle);
        return 0;
    }
}

static void StartAudio(int sampleRate, AudioCallback callback)
{
    AudioOut::rate = sampleRate;
    AudioOut::cb = callback;
    AudioOut::running = true;
    DWORD tid;
    AudioOut::Thread = CreateThread(0, 0, AudioOut::ThreadFunc, 0, 0, &tid);
}

static void StopAudio()
{
    AudioOut::running = false;
    WaitForSingleObject(AudioOut::Thread, INFINITE);
    CloseHandle(AudioOut::Thread);
}
