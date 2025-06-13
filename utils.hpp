//
//  utils.hpp
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

#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <chrono>
#include <cstddef>
#include <iostream>

#include "log.hpp"

class TimeElapsed {
public:
  TimeElapsed() = delete;
  TimeElapsed(const std::string &desc) {
    desc_ = desc;
    start_ = std::chrono::high_resolution_clock::now();
  }

  uint32_t elapsed() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start_;
    return elapsed.count();
  }

  ~TimeElapsed() {
    BOOST_LOG_TRIVIAL(info) << desc_ << " returned in " << elapsed() << " ms";
  }

private:
  std::chrono::_V2::system_clock::time_point start_;
  std::string desc_;
};

#endif
