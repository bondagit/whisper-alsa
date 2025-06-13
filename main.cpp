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

#include <boost/program_options.hpp>
#include <iostream>
#include <signal.h>
#include <thread>

#include "config.hpp"
#include "log.hpp"
#include "transcriber.hpp"

namespace po = boost::program_options;
namespace postyle = boost::program_options::command_line_style;
namespace logging = boost::log;

static const std::string version("whisper-alsa-1.0.0");
static std::atomic<bool> terminate = false;

void termination_handler(int signum) {
  BOOST_LOG_TRIVIAL(info) << "main:: got signal " << signum;
  // Terminate program
  terminate = true;
}

bool is_terminated() { return terminate.load(); }

const std::string &get_version() { return version; }

int main(int argc, char *argv[]) {
  int rc(EXIT_SUCCESS);
  po::options_description desc("Options");
  desc.add_options()
      ("version,v", "Print version and exit")
      ("device_name,D", po::value<std::string>()->default_value("default"), "ALSA capture device name")
      ("channels,c", po::value<int>()->default_value(2), "ALSA channels to capture")
      ( "sample_rate,r", po::value<int>()->default_value(16000), "ALSA capture sample rate")
      ( "buffer_duration,s", po::value<int>()->default_value(5), "Audio buffer duration in seconds from 2 to 10")
      ( "silence_threshold,t", po::value<float>()->default_value(0.001f, "0.001"), "Audio buffer sample silence threshold")
      ( "buffers_num,n", po::value<int>()->default_value(4), "Audio buffers number from 3 to 10")
      ( "language,l", po::value<std::string>()->default_value("en"), "Whisper default language")
      ( "model,m", po::value<std::string>()->default_value("models/ggml-base.en.bin"), "Whisper model to use")
      ("openvino_device,o", po::value<std::string>()->default_value("CPU"), "Whisper openvino device to use")
      ("vad_enabled,e", po::value<bool>()->default_value(false), "Whisper enable/disable VAD")
      ("use_context,x", po::value<bool>()->default_value(false), "Whisper enable/disable token context")
      ("vad_model,a", po::value<std::string>()->default_value("models/ggml-silero-v5.1.2.bin"), "Whisper VAD model to use")
      ("vad_threshold,l", po::value<float>()->default_value(0.1f, "0.1"), "Whisper VAD threshold to use")
      ( "log_level,d", po::value<int>()->default_value(2), "Log levelfrom 0=trace to 5=fatal")
      ("help,h", "Print this help " "message");
  int unix_style = postyle::unix_style | postyle::short_allow_next;

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv)
                  .options(desc)
                  .style(unix_style)
                  .run(),
              vm);

    po::notify(vm);

    if (vm.count("version")) {
      std::cout << version << '\n';
      return EXIT_SUCCESS;
    }
    if (vm.count("help")) {
      std::cout << "USAGE: " << argv[0] << '\n' << desc << '\n';
      return EXIT_SUCCESS;
    }

  } catch (po::error &poe) {
    std::cerr << poe.what() << '\n'
              << "USAGE: " << argv[0] << '\n'
              << desc << '\n';
    return EXIT_FAILURE;
  }

  signal(SIGINT, termination_handler);
  signal(SIGTERM, termination_handler);
  signal(SIGCHLD, SIG_IGN);

  std::srand(std::time(nullptr));

  Config config;
  config.set_device_name(vm["device_name"].as<std::string>());
  config.set_channels(vm["channels"].as<int>());
  config.set_log_severity(vm["log_level"].as<int>());
  config.set_sample_rate(vm["sample_rate"].as<int>());
  config.set_file_duration(vm["buffer_duration"].as<int>());
  config.set_files_num(vm["buffers_num"].as<int>());
  config.set_silence_threshold(vm["silence_threshold"].as<float>());
  config.set_language(vm["language"].as<std::string>());
  config.set_model(vm["model"].as<std::string>());
  config.set_openvino_device(vm["openvino_device"].as<std::string>());
  config.set_vad_enabled(vm["vad_enabled"].as<bool>());
  config.set_vad_model(vm["vad_model"].as<std::string>());
  config.set_vad_threshold(vm["vad_threshold"].as<float>());
  config.set_use_context(vm["use_context"].as<bool>());

  /* init logging */
  log_init(config);

  BOOST_LOG_TRIVIAL(debug) << "main:: initializing ...";
  try {
    auto transcriber = Transcriber::create(config);
    if (!transcriber->init()) {
      throw std::runtime_error(std::string("main:: Transcriber init failed"));
    }

    BOOST_LOG_TRIVIAL(debug) << "main:: init done, entering loop...";

    if (!transcriber->start_capture()) {
      throw std::runtime_error(
          std::string("main:: Transcriber start capture failed"));
    }

    while (!is_terminated()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::string text;
    if (transcriber->get_text(text)) {
      std::cout << "Transcription:\n" << text << std::endl;
    }
    if (!transcriber->stop_capture()) {
      throw std::runtime_error(
          std::string("main:: Transcriber stop capture failed"));
    }

    if (!transcriber->terminate()) {
      throw std::runtime_error(std::string("main:: terminate failed"));
    }
  } catch (std::exception &e) {
    BOOST_LOG_TRIVIAL(fatal) << "main:: fatal exception error: " << e.what();
    rc = EXIT_FAILURE;
 }

  std::cout << "exiting with code: " << rc << std::endl;
  return rc;
}
