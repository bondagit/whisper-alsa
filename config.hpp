//
//  config.hpp
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

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include <cstdint>
#include <memory>
#include <string>

class Config {
 public:
  uint8_t get_channels() const { return channels_; }
  uint8_t get_files_num() const { return files_num_; }
  uint16_t get_file_duration() const { return file_duration_; }
  float get_silence_threshold() const { return silence_threshold_; }
  const std::string& get_model() const { return model_; }
  const std::string& get_language() const { return language_; }
  const std::string& get_openvino_device() const { return openvino_device_; }
  int get_log_severity() const { return log_severity_; };
  const std::string& get_device_name() const { return device_name_; };
  bool get_use_context() const { return use_context_; };
  bool get_vad_enabled() const { return vad_enabled_; };
  const std::string& get_vad_model() const { return vad_model_; };
  float get_vad_threshold() const { return vad_threshold_; };

  void set_channels(uint8_t channels) { channels_ = channels; }
  void set_files_num(uint8_t files_num) { files_num_ = files_num; }
  void set_file_duration(uint8_t file_duration) {
    file_duration_ = file_duration;
  }
  void set_silence_threshold(float silence_threshold) {
    silence_threshold_ = silence_threshold;
  }
  void set_model(const std::string& model) { model_ = model; }
  void set_language(const std::string& language) { language_ = language; }
  void set_openvino_device(const std::string& openvino_device) {
    openvino_device_ = openvino_device;
  }
  void set_log_severity(int log_severity) { log_severity_ = log_severity; };
  void set_sample_rate(uint32_t sample_rate) { sample_rate_ = sample_rate; };
  void set_device_name(std::string_view device_name) {
    device_name_ = device_name;
  };
  void set_use_context(bool use_context) { use_context_ = use_context; };
  void set_vad_enabled(bool vad_enabled) { vad_enabled_ = vad_enabled; };
  void set_vad_model(const std::string& vad_model) { vad_model_ = vad_model; };
  void set_vad_threshold(float vad_threshold) {
    vad_threshold_ = vad_threshold;
  };

 private:
  uint8_t channels_{4};
  uint8_t files_num_{4};
  uint16_t file_duration_{5};
  uint32_t sample_rate_{16000};
  float silence_threshold_{1e-3};
  std::string model_{"./models/ggml-base.en.bin"};
  std::string language_{"en"};
  std::string openvino_device_{"CPU"};
  int log_severity_{2};
  std::string device_name_{"default"};
  bool use_context_{false};
  bool vad_enabled_{false};
  std::string vad_model_{"./models/ggml-silero-v5.1.2.bin"};
  float vad_threshold_{1e-1};
};

#endif
