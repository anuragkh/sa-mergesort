#ifndef CORE_DATA_INPUT_STREAM_H_
#define CORE_DATA_INPUT_STREAM_H_

#include <iostream>
#include <fstream>
#include <cstdio>

#include "succinct_utils.h"

template<typename data_type, typename pos_type = size_t>
class DataInputStream {
 public:
  DataInputStream(std::string& filename, bool memory_map = false) {
    current_idx_ = 0;
    memory_map_ = memory_map;
    filename_ = filename;
    FILE *f = fopen(filename.c_str(), "r");
    fseek(f, 0, SEEK_END);
    filesize_ = ftell(f);
    fclose(f);

    if (!memory_map_) {
      in_.open(filename_);
      data_ = NULL;
    } else {
      data_ = (data_type *) SuccinctUtils::MemoryMap(filename_);
    }
  }

  data_type Get() {
    if (memory_map_) {
      return data_[current_idx_++];
    }

    data_type val;
    if(current_idx_ * sizeof(data_type) >= filesize_) {
      val = 0;
    } else {
      in_.read(reinterpret_cast<char *>(&(val)), sizeof(data_type));
    }
    current_idx_++;
    return val;
  }

  pos_type GetCurrentIndex() {
    return current_idx_;
  }

  void Reset() {
    if (!memory_map_) {
      in_.clear();
      in_.seekg(0, std::ios::beg);
    }
    current_idx_ = 0;
  }

  void Close() {
    if (!memory_map_) {
      in_.close();
    }
  }

  void Remove() {
    remove(filename_.c_str());
  }

 private:
  std::string filename_;
  std::ifstream in_;
  data_type *data_;
  pos_type current_idx_;
  pos_type filesize_;
  bool memory_map_;

};

#endif // CORE_DATA_INPUT_STREAM_H_
