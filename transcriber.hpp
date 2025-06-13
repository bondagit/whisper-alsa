//
//  transcriber.cpp
//
//  Copyright (c) 2019 2025 Andrea Bondavalli. All rights reserved.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the MIT license
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef _WHISPER_HPP_
#define _WHISPER_HPP_

#include <alsa/asoundlib.h>
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include "capture.hpp"
#include "config.hpp"
#include "whisper.hpp"

class Transcriber {
public:
  static std::shared_ptr<Transcriber> create(const Config &config);
  Transcriber() = delete;

  bool init();
  bool terminate();

  bool get_text(std::string &out);
  bool clear_text();

  bool start_capture();
  bool stop_capture();

protected:
  explicit Transcriber(const Config &config) : config_(config){};

private:
  void open_files(uint8_t files_id);
  void close_files(uint8_t files_id);
  void save_files(uint8_t files_id);

  const Config &config_;
  uint16_t file_duration_{5};
  uint8_t files_num_{4};
  uint8_t channels_{4};
  float silence_threshold_{1e-4};
  uint16_t keep_samples_{1600};
  snd_pcm_uframes_t chunk_samples_{0};
  size_t bytes_per_frame_{0};
  size_t buffer_samples_{0};
  uint32_t buffer_offset_{0};
  uint32_t silence_samples_;
  std::vector<float> tmp_buf_;
  std::map<uint8_t, std::vector<float>> output_bufs_;
  uint32_t file_counter_{0};
  std::atomic<uint8_t> file_id_{0};
  std::unique_ptr<uint8_t[]> buffer_;
  uint32_t rate_{16000};
  std::future<bool> res_capts_;
  std::future<bool> res_trans_;
  std::atomic_bool running_{false};
  Capture capture_;
  std::mutex whisper_mutex_;
  std::condition_variable whisper_cond_;
  Whisper whisper_{config_};
};

#endif
