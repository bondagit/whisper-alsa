# whisper-alsa

## Introduction

This application performs real-time inference on audio from an ALSA capture device.
It builds upon a multithreaded design to achieve high performance and minimize the risk of audio capture overruns.

## Architecture

1. **Threads**:
   - **Capture Thread**: Captures the audio streams from the configured channels using the ALSA interface. It writes the captured audio data into **rotating audio buffers** to decouple audio capture from transcription computation. Such decoupling is required as the ALSA buffer size depends on the specifc audio device and we want to cope with spikes in transcription processing that could cause capture overruns.
   The capture thread also perfoms audio resampling to 16KHz (if required), downmixing, audio format conversion from PCM signed to float and audio silence detection to filter out silence buffers.
   - **Transcription Thread**: Reads data from the current audio buffer and executes transcriptions via Whisper that uses the available CPU cores and GPUs for processing. Transcription result is stored in a text buffer that is written to output on exit.

2. **Rotating Audio Buffers**:
   - The capture thread writes audio data into rotating audio buffers of a specifc duration. These buffers ensure that audio capture is independent from the transcriptions and they can run in parallel. The transcription can start when the first audio buffer with no silence is filled, so it runs with a latency of a single buffer.
   - The number of rotating buffers and their durations can be set via command line arguments.

4. **Configuration Parameters**:
   - The application accept a number of configuration parameters. These parameters are described in the **How to Build and Run a Test** section below and the the daemon's README.

See the diagram below:

![whisper-alsa](https://github.com/user-attachments/assets/8dfd393d-8f03-4c2f-8a43-7935a9651760)

## How to Build and Run a Test

To enable the transcription feature in the AES67 Linux Daemon, follow these steps:

### 1. Build the application

To compile the application follow these steps:

- install required depdendecies, see _debian-packages.sh_ script

- clone the whisper.cpp repository and switch to the branch/release you need:

      git clone https://github.com/ggml-org/whisper.cpp.git

- compile Whisper with:

      cmake -B build
      cmake --build build --config Release

- compile the **wishper-alsa** with the following and replace _[whisper_path]_ with your whisper.cpp path:

      cmake . -DWHISPER_CPP_DIR=[whisper_path]/whisper.cpp
      make -j

### 2. Parameters

The applcation accepts the following command line parameters:

     USAGE: ./whisper-alsa
     Options:
       -v [ --version ]                      Print version and exit
       -D [ --device_name ] arg (=default)   ALSA capture device name
       -c [ --channels ] arg (=2)            ALSA channels to capture
       -r [ --sample_rate ] arg (=16000)     ALSA capture sample rate
       -s [ --buffer_duration ] arg (=5)     Audio buffer duration in seconds from 2 to 10
       -t [ --silence_threshold ] arg (=0.001) 
                                             Audio buffer sample silence threshold
       -n [ --buffers_num ] arg (=4)         Audio buffers number from 3 to 10
       -l [ --language ] arg (=en)           Whisper default language
       -m [ --model ] arg (=models/ggml-base.en.bin) Whisper model to use
       -o [ --openvino_device ] arg (=CPU)   Whisper openvino device to use
       -e [ --vad_enabled ] arg (=0)         Whisper enable/disable VAD
       -x [ --use_context ] arg (=0)         Whisper enable/disable token context
       -a [ --vad_model ] arg (=models/ggml-silero-v5.1.2.bin) 
                                             Whisper VAD model to use
       -l [ --vad_threshold ] arg (=0.1)     Whisper VAD threshold to use
       -d [ --log_level ] arg (=2)           Log levelfrom 0=trace to 5=fatal
       -h [ --help ]                         Print this help message

where:

> **log\_level**
> Log severity level (0 to 5).    
> All traces major or equal to the specified level are enabled. (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=fatal).

> **device\_name**
> Name of the ALSA capture device to use. Default "default".
> Use _arecord -L_ to get a list of all ALSA capture devices available.

> **channels**
> Number of audio channels captured by the ALSA capture thread. Default 2.
> Downmixing to 1 channel is performed by the capture thread.

> **sample\_rate**
> Sample rate used by the ALSA capture thread. Default 16000.
> Resampling to 16000 is peformend by ALSA.

> **buffers\_num**
> Number of buffers in the rotating audio buffers pool. Default 4.

> **buffer\_duration**: 
> Duration (in seconds) of each audio buffer. Default 5.

> **silence_threshold**: 
> An audio sample is marked as silence if its value is below this threshold. Default 0.001.
> An audio buffer containing more than 100ms (1600 samples) of non-silence is passed to Whisper, > otherwise it gets discarded.

> **model**: 
> Whisper model file path. The default is the base English model.

> **language**: 
> Language setting for Whisper. Default is English "en".

> **vad\_enabled**: 
> 1 to enable Whisper VAD. Disabled by default.

> **vad\_threshold**: 
> Whisper VAD threshold to use. Default 0.1.

> **use\_context**
> The application stores Whisper tokens returned from previous audio buffer prceossing and 
> present them to the next call.

> **openvino\_device**: 
> OpenVINO device for inference, if supported by the current model. Default is "CPU".

### 3. Run a test

Run a test with the small model and VAD enabled:

     ./whisper-alsa -m ../whisper.cpp/models/ggml-small.en.bin -a ../whisper.cpp/models/ggml-silero-v5.1.2.bin -e 1 

Transcription is presented both in real-time and at application exit:

     [2025-06-13 12:44:46.604996] [0x000073567cab34c0] [info]    transcriber:: init
     [2025-06-13 12:44:46.605028] [0x000073567cab34c0] [info]    transcriber:: starting audio capture ... 
     [2025-06-13 12:44:46.811139] [0x0000735679fd96c0] [info]    whisper:: init returned in 193 ms
     [2025-06-13 12:44:52.649360] [0x0000735679fd96c0] [info]    transcriber:: file 0 samples 80000 capturing file 1
     [2025-06-13 12:44:55.344765] [0x0000735679fd96c0] [info]    whisper:: [00:00:01.600 -> 00:00:04.080] text [ Good luck, my dear. Good luck!] 
     [2025-06-13 12:44:55.344799] [0x0000735679fd96c0] [info]    whisper:: transribe() returned in 2695 ms
     [2025-06-13 12:44:57.687248] [0x0000735679fd96c0] [info]    transcriber:: file 1 samples 80000 capturing file 2
     [2025-06-13 12:44:57.702408] [0x0000735679fd96c0] [info]    whisper:: transribe() returned in 15 ms
     [2025-06-13 12:44:59.479794] [0x000073567cab34c0] [info]    main:: got signal 2

     Transcription:
     Good luck, my dear. Good luck!

     [2025-06-13 12:44:59.620198] [0x000073567cab34c0] [info]    transcriber:: stopping audio capture ... 
     [2025-06-13 12:44:59.680006] [0x000073567cab34c0] [info]    transcriber:: terminating ... 
     [2025-06-13 12:44:59.680073] [0x000073567cab34c0] [info]    main:: end

### 4. Notes

- ALSA: the application uses the ALSA interface to read from the desired capture device. The audio is captured using _S16_LE_ format as this is the most commonly supported. Audio gets resamples, downmixed and presented to Whisper as mono, _FLOAT_LE_ at 16KHz.
- Integration with Whisper: the application implements a basic integration with Whisper. Real-time transcription poses some challenges: 
  - audio chunking should be done at word boundaries to avoid cutting words.
  - adoption of LocalAgreement-2 policy can improve the transcirption accuracy: incremental audio chunks can be presented to Whisper and a confirmed transcription is returned when 2 runs agree on a text prefix.
- Performance Considerations: Whisper transcription is computationally demanding. On a system with a _Intel Core i9-11900H_ processor the _base_ and the _small_ models work in real-time, larger models require dedicated GPUs for real-time performance.
- System Monitoring: monitor warnings in the application's log. Check for messages like the following and in case reduce the load:
      transcriber:: requesting current capture file, probably running to slow, skipping file     
      transcriber:: whisper processing took longer than the audio buffer duration


