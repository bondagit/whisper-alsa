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

#include <boost/algorithm/string.hpp>
#include <cmath>

#include "log.hpp"
#include "transcriber.hpp"
#include "utils.hpp"

using namespace std::chrono_literals;

std::shared_ptr<Transcriber> Transcriber::create(const Config &config) {
  // no need to be thread-safe here
  static std::weak_ptr<Transcriber> instance;
  if (auto ptr = instance.lock()) {
    return ptr;
  }
  auto ptr = std::shared_ptr<Transcriber>(new Transcriber(config));
  instance = ptr;
  return ptr;
}

bool Transcriber::init() {
  BOOST_LOG_TRIVIAL(info) << "transcriber:: init";
  running_ = false;
  return true;
}

bool Transcriber::start_capture() {
  if (running_)
    return true;

  BOOST_LOG_TRIVIAL(info) << "transcriber:: starting audio capture ... ";

  channels_ = config_.get_channels();
  files_num_ = config_.get_files_num();
  file_duration_ = config_.get_file_duration();
  silence_threshold_ = config_.get_silence_threshold();

  if (files_num_ < 3 || files_num_ > 10) {
    BOOST_LOG_TRIVIAL(info) << "transcriber:: buffers num of of range";
  }

  if (file_duration_ < 2 || file_duration_ > 10) {
    BOOST_LOG_TRIVIAL(info) << "transcriber:: buffer duration out of range";
  }

  if (!capture_.open(config_.get_device_name(), rate_, channels_)) {
    BOOST_LOG_TRIVIAL(fatal) << "transcriber:: cannot open capture";
    return false;
  }

  bytes_per_frame_ = capture_.get_bytes_per_frame();
  capture_.set_chunk_samples(8000); // 500 ms
  chunk_samples_ = capture_.get_chunk_samples();
  buffer_samples_ = rate_ * file_duration_ / chunk_samples_ * chunk_samples_;
  BOOST_LOG_TRIVIAL(debug) << "transcriber:: buffer_samples "
                           << buffer_samples_;

  buffer_.reset(new uint8_t[buffer_samples_ * bytes_per_frame_]);
  if (buffer_ == nullptr) {
    BOOST_LOG_TRIVIAL(fatal) << "transcriber:: cannot allocate audio buffer";
    return false;
  }

  buffer_offset_ = 0;
  file_id_ = 0;
  file_counter_ = 0;
  running_ = true;

  open_files(file_id_);

  /* start transcribing on a separate thread */
  res_trans_ = std::async(std::launch::async, [&]() {
    BOOST_LOG_TRIVIAL(debug) << "transcriber:: transcriptions loop start";
    if (!whisper_.init()) {
      BOOST_LOG_TRIVIAL(fatal) << "transcriber:: cannot open whisper";
      return false;
    }

    uint32_t current_file_conter = 0;
    uint8_t file_id = 0;
    while (1) {
      std::unique_lock whisper_lock(whisper_mutex_);
      /* wait for a new file to complete */
      whisper_cond_.wait(whisper_lock, [&] {
        return !running_ ||
               (file_counter_ > 0 && current_file_conter != file_counter_);
      });
      current_file_conter++;
      whisper_lock.unlock();

      if (!running_)
        break;

      if (file_id == file_id_.load()) {
        BOOST_LOG_TRIVIAL(error)
            << "transcriber:: requesting current capture file, "
            << "probably running to slow, skipping file "
            << std::to_string(file_id);
      } else {
        auto samples_num = output_bufs_[file_id].size();
        BOOST_LOG_TRIVIAL(info)
            << "transcriber:: file " << (int)file_id << " samples "
            << samples_num << " capturing file " << (int)file_id_.load();

        if (samples_num > keep_samples_) {
          whisper_.transribe(output_bufs_[file_id].data(), samples_num);
        } else {
          whisper_.segment();
        }
      }
      /* increase file id to process */
      file_id = (file_id + 1) % files_num_;
    }

    /* close Whispers*/
    whisper_.terminate();

    BOOST_LOG_TRIVIAL(debug) << "transcriber:: transcriptions loop end";
    return true;
  });

  auto status_trans = res_trans_.wait_for(1s);
  if (status_trans != std::future_status::timeout) {
    /* sync task terminated prematurely */
    running_ = false;
    return false;
  }

  /* start capturing on a separate thread */
  res_capts_ = std::async(std::launch::async, [&]() {
    BOOST_LOG_TRIVIAL(debug)
        << "transcriber:: audio capture loop start, chunk_samples = "
        << chunk_samples_;
    while (running_) {
      if ((capture_.read(buffer_.get() + buffer_offset_ * bytes_per_frame_)) <
          0) {
        break;
      }

      save_files(file_id_);
      buffer_offset_ += chunk_samples_;

      /* check if buffer is full */
      if ((buffer_offset_ + chunk_samples_) > buffer_samples_) {
        close_files(file_id_);

        std::lock_guard<std::mutex> lock(whisper_mutex_);
        /* increase file id */
        file_id_ = (file_id_ + 1) % files_num_;
        file_counter_++;
        whisper_cond_.notify_one();

        buffer_offset_ = 0;

        open_files(file_id_);
      }
    }
    BOOST_LOG_TRIVIAL(debug) << "transcriber:: audio capture loop end";

    /* notify transcription for termination*/
    std::lock_guard<std::mutex> lock(whisper_mutex_);
    whisper_cond_.notify_one();

    return true;
  });
 

  return true;
}

void Transcriber::open_files(uint8_t file_id) {
  BOOST_LOG_TRIVIAL(debug) << "transcriber:: opening file with id "
                           << std::to_string(file_id) << " ...";
  tmp_buf_.clear();
  silence_samples_ = 0;
}

void Transcriber::save_files(uint8_t file_id) {
  auto sample_size = bytes_per_frame_ / channels_;
  for (size_t offset = 0; offset < chunk_samples_; offset++) {
    float pcmFloat{0};
    for (uint16_t ch = 0; ch < channels_; ch++) {
      /* extract mapped channels and converted pcm from int to float */
      uint8_t *in = buffer_.get() +
                    (buffer_offset_ + offset) * bytes_per_frame_ +
                    ch * sample_size;
      switch (sample_size) {
      case 2: {
        int16_t pcm = *in | (*(in + 1) << 8);
        pcmFloat += static_cast<float>(pcm) / 32768.0f;
      } break;
      case 3: {
        int32_t pcm = *in | (*(in + 1) << 8) | (*(in + 2) << 16);
        // If the most significant bit of the 24th bit is set
        if (*(in + 2) & 0x80) {
          pcm |= (0xFF << 24); // Fill the upper 8 bits with 1s
        }
        pcmFloat += static_cast<float>(pcm) / 8388608.0f;
      } break;
      case 4: {
        int32_t pcm =
            *in | (*(in + 1) << 8) | (*(in + 2) << 16) | (*(in + 3) << 24);
        pcmFloat += static_cast<float>(pcm) / 2147483648.0f;
      } break;
      }
    }

    if (std::fabs(pcmFloat) < silence_threshold_) {
      silence_samples_++;
    }
    tmp_buf_.push_back(pcmFloat);
  }
}

void Transcriber::close_files(uint8_t file_id) {
  BOOST_LOG_TRIVIAL(debug) << "transcriber:: silence samples "
                           << silence_samples_;
  output_bufs_[file_id].clear();
  if (buffer_samples_ - silence_samples_ > keep_samples_) {
    std::copy(tmp_buf_.begin(), tmp_buf_.end(),
              back_inserter(output_bufs_[file_id]));
  } else {
    BOOST_LOG_TRIVIAL(info) << "transcriber:: skipping buffer with "
                            << silence_samples_ << " silence samples";
  }
}

bool Transcriber::stop_capture() {
  if (!running_)
    return true;

  BOOST_LOG_TRIVIAL(info) << "transcriber:: stopping audio capture ... ";
  running_ = false;
  bool ret = res_trans_.get();
  ret = res_capts_.get();
  capture_.close();
  return ret;
}

bool Transcriber::terminate() {
  BOOST_LOG_TRIVIAL(info) << "transcriber:: terminating ... ";
  return stop_capture();
}

bool Transcriber::get_text(std::string &out) {
  if (!running_) {
    BOOST_LOG_TRIVIAL(warning) << "Transcriber:: not running";
    return false;
  }

  out = whisper_.get_text();
  return true;
}

bool Transcriber::clear_text() {
  if (!running_) {
    BOOST_LOG_TRIVIAL(warning) << "Transcriber:: not running";
    return false;
  }

  whisper_.clear_text();
  return true;
}
