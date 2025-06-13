//
//  whisper.cpp
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

#include <shared_mutex>
#include <sstream>
#include <whisper.h>

class Whisper {
public:
  Whisper(const Config &config) : config_(config){};
  Whisper(const Whisper &) = delete;

  bool init();
  const std::string get_text();
  void clear_text();
  void process_result();
  void terminate();
  void segment();
  bool transribe(const float *in, uint32_t samples_in);

private:
  const Config &config_;
  std::string to_timestamp(int64_t t, bool comma = false);

  std::string language_;
  std::vector<whisper_token> prompt_tokens_;
  std::stringstream output_text_;
  std::shared_mutex text_mutex_;
  struct whisper_state *state_;
  struct whisper_context *ctx_{0};
};
