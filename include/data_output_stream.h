#ifndef CORE_DATA_OUTPUT_STREAM_H_
#define CORE_DATA_OUTPUT_STREAM_H_

#include <iostream>
#include <fstream>
#include <cstdio>

#include "succinct_utils.h"

template<typename data_type, typename pos_type = size_t>
class DataOutputStream {
 public:
  DataOutputStream(std::string& filename, bool memory_map = false) {
    current_idx_ = 0;
    memory_map_ = memory_map;
    filename_ = filename;

    if (!memory_map_) {
      out_.open(filename_);
      data_ = NULL;
    } else {
      data_ = (data_type *) SuccinctUtils::MemoryMap(filename_);
    }
  }

  void Put(data_type val) {
    if (memory_map_) {
      data_[current_idx_++] = val;
      return;
    }
    out_.write(reinterpret_cast<const char *>(&(val)), sizeof(data_type));
    current_idx_++;
  }

  pos_type GetCurrentIndex() {
    return current_idx_;
  }

  void Close() {
    if (!memory_map_) {
      out_.close();
    }
  }

 private:
  std::string filename_;
  std::ofstream out_;
  data_type *data_;
  pos_type current_idx_;
  bool memory_map_;

};

#endif // CORE_DATA_OUTPUT_STREAM_H_
