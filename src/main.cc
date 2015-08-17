#include "divsufsortxx.h"
#include "data_input_stream.h"
#include "data_output_stream.h"
#include "succinct_utils.h"

#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <map>

#ifndef CHAR
#define CHAR(c) ((c == 0) ? '$' : c)
#endif

typedef unsigned long long int TimeStamp;
static TimeStamp GetTimestamp() {
  struct timeval now;
  gettimeofday(&now, NULL);

  return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
}

template<typename alphabet_type, typename size_type>
void Densify(alphabet_type* buffer, size_type size, std::map<alphabet_type, alphabet_type> alphabet) {
  for (uint64_t i = 0; i < size; i++) {
    buffer[i] = alphabet[buffer[i]];
  }
}

void ConstructSAInMemory(std::string& filename, int64_t* SA, uint64_t size,
                         int32_t alphabet_size) {
  uint16_t* data = new uint16_t[size];
  SuccinctUtils::ReadFromFile(data, size, filename);

  divsufsortxx::constructSA(data, data + size, SA, SA + size, alphabet_size);
  delete[] data;
}

int CompareSuffix(char* data, uint64_t size, uint64_t i, uint64_t j) {
  if (i == j) {
    fprintf(stderr, "You are stupid\n");
    exit(0);
  }
  while (true) {
    if (data[i] != data[j])
      return data[i] - data[j];
    i = (i + 1) % size;
    j = (j + 1) % size;
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
    return -1;
  }

  // Filenames
  char *filename = argv[1];
  std::string filename_str(filename);
  std::string left_file = filename_str + ".tmp.left";
  std::string right_file = filename_str + ".tmp.right";
  std::string left_sa_file = filename_str + ".tmp.left.sa";
  std::string right_sa_file = filename_str + ".tmp.right.sa";
  std::string sa_file = filename_str + ".tmp.sa";

  // Timestamps, for benchmarking
  TimeStamp t0, t1, tdiff;

  // Get input size
  FILE *f = fopen(filename, "r");
  fseek(f, 0, SEEK_END);
  uint64_t fsize = ftell(f);
  fclose(f);
  uint64_t input_size_ = fsize + 1;

  // Construct left and right
  uint16_t *left, *right;
  int64_t left_size, right_size, left_idx, right_idx;
  std::map<uint16_t, uint16_t> alphabet;
  DataInputStream<char> data_stream(filename_str);
  char cur_c, nxt_c, fst_c;
  uint16_t val;
  uint64_t data_pos;

  t0 = GetTimestamp();
  left_size =
      (input_size_ % 2 == 0) ? (input_size_ / 2) : (input_size_ / 2 + 1);
  right_size = input_size_ - left_size;
  left = new uint16_t[left_size];
  right = new uint16_t[right_size];
  fst_c = cur_c = data_stream.Get();
  left_idx = right_idx = 0;
  for (data_pos = 0; data_stream.GetCurrentIndex() < input_size_; data_pos++) {
    nxt_c = data_stream.Get();
    val = cur_c * 256 + nxt_c;
    alphabet[val] = 0;
    if (data_pos % 2 == 0) {
      left[left_idx++] = val;
    } else {
      right[right_idx++] = val;
    }
    cur_c = nxt_c;
  }
  nxt_c = fst_c;
  val = cur_c * 256 + nxt_c;
  alphabet[val] = 0;
  if (data_pos % 2 == 0) {
    left[left_idx++] = val;
  } else {
    right[right_idx++] = val;
  }
  data_stream.Close();

  int32_t alphabet_size;
  typedef typename std::map<uint16_t, uint16_t>::iterator AlphabetIterator;
  for (AlphabetIterator it = alphabet.begin(); it != alphabet.end(); it++) {
    alphabet[it->first] = alphabet_size++;
  }

  Densify(left, left_size, alphabet);
  Densify(right, right_size, alphabet);

  SuccinctUtils::WriteToFile(left, left_size, left_file);
  delete[] left;
  SuccinctUtils::WriteToFile(right, right_size, right_file);
  delete[] right;

  t1 = GetTimestamp();
  tdiff = t1 - t0;
  fprintf(stderr, "Time to create left & right=%llu\n", tdiff / (1000 * 1000));

  fprintf(stderr, "input_size_ = %llu, left_size = %llu, right_size = %llu\n",
          input_size_, left_size, right_size);

  t0 = GetTimestamp();
  int64_t* lSA = new int64_t[left_size]();
  ConstructSAInMemory(left_file, lSA, left_size, alphabet_size);
  remove(left_file.c_str());
  SuccinctUtils::WriteToFile(lSA, left_size, left_sa_file);
  delete[] lSA;
  t1 = GetTimestamp();
  tdiff = t1 - t0;
  fprintf(stderr, "Time to create left SA=%llu\n", tdiff / (1000 * 1000));

  t0 = GetTimestamp();
  int64_t* rSA = new int64_t[right_size]();
  ConstructSAInMemory(right_file, rSA, right_size, alphabet_size);
  remove(right_file.c_str());
  SuccinctUtils::WriteToFile(rSA, right_size, right_sa_file);
  delete[] rSA;
  t1 = GetTimestamp();
  tdiff = t1 - t0;
  fprintf(stderr, "Time to create right SA=%llu\n", tdiff / (1000 * 1000));

  // Merge sort the results
  t0 = GetTimestamp();
  uint64_t i = 0, j = 0;
  char *data = new char[input_size_]();
  uint64_t sa_idx = 0;
  DataInputStream<int64_t> left_sa_stream(left_sa_file);
  DataInputStream<int64_t> right_sa_stream(right_sa_file);
  DataOutputStream<int64_t> sa_stream(sa_file);
  SuccinctUtils::ReadFromFile(data, input_size_, filename_str);
  int64_t first, second;
  first = 2 * left_sa_stream.Get();
  second = 2 * right_sa_stream.Get() + 1;
  while (sa_stream.GetCurrentIndex() < input_size_) {
    if (right_sa_stream.GetCurrentIndex() > right_size
        || (left_sa_stream.GetCurrentIndex() <= left_size
            && CompareSuffix(data, input_size_, first, second) < 0)) {
      sa_stream.Put(first);
      first = 2 * left_sa_stream.Get();
    } else {
      sa_stream.Put(second);
      second = 2 * right_sa_stream.Get() + 1;
    }
  }
  sa_stream.Close();
  left_sa_stream.Close();
  right_sa_stream.Close();
  left_sa_stream.Remove();
  right_sa_stream.Remove();
  delete[] data;

  t1 = GetTimestamp();
  tdiff = t1 - t0;
  fprintf(stderr, "Time to merge sort SA=%llu\n", tdiff / (1000 * 1000));

  return 0;
}
