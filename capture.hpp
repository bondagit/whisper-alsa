//
//  capture.cpp
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

#ifndef _CAPTURE_HPP_
#define _CAPTURE_HPP_

#include <alsa/asoundlib.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

class Capture {
public:
  Capture() = default;
  Capture(const Capture &) = delete;

  ssize_t read(uint8_t *data);
  bool open(const std::string &device, uint32_t rate, uint8_t channels);
  void close();

  uint8_t get_bytes_per_frame() const { return bytes_per_frame_; }
  snd_pcm_uframes_t get_chunk_samples() const { return chunk_samples_; }
  void set_chunk_samples(snd_pcm_uframes_t chunk_samples) {
    chunk_samples_ = chunk_samples;
  }
  snd_pcm_format_t get_format() const { return format; }

private:
  constexpr static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  std::atomic_bool is_open_{false};
  snd_pcm_t *capture_handle_{0};
  snd_pcm_uframes_t chunk_samples_{0};
  uint32_t periods_{0};
  size_t bytes_per_frame_{0};

  bool xrun();
  bool suspend();
};

#endif
