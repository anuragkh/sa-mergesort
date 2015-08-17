#ifndef SUCCINCT_UTILS_H_
#define SUCCINCT_UTILS_H_

#include <cstdint>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <cassert>

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

/* Pop-count Constants */
#define m1   0x5555555555555555 //binary: 0101...
#define m2   0x3333333333333333 //binary: 00110011..
#define m4   0x0f0f0f0f0f0f0f0f //binary:  4 zeros,  4 ones ...
#define m8   0x00ff00ff00ff00ff //binary:  8 zeros,  8 ones ...
#define m16  0x0000ffff0000ffff //binary: 16 zeros, 16 ones ...
#define m32  0x00000000ffffffff //binary: 32 zeros, 32 ones
#define hff  0xffffffffffffffff //binary: all ones
#define h01  0x0101010101010101 //the sum of 256 to the power of 0,1,2,3...

#define ISPOWOF2(n)     ((n != 0) && ((n & (n - 1)) == 0))

class SuccinctUtils {
 public:
  // Returns the number of set bits in a 64 bit integer
  static uint64_t PopCount(uint64_t n);

  // Returns integer logarithm to the base 2
  static uint32_t IntegerLog2(uint64_t n);

  // Returns a modulo n
  static uint64_t Modulo(int64_t a, uint64_t n);

  // Computes the number of blocks of block_size in val
  static uint64_t NumBlocks(uint64_t val, uint64_t block_size);

  // Computes the max between two integers
  static int64_t Max(int64_t first, int64_t second);

  // Computes the min between two integers
  static int64_t Min(int64_t first, int64_t second);

  // Memory maps a file and returns a pointer to it
  static void* MemoryMap(std::string filename);

  // Writes an array to file
  template<typename data_type, typename size_type>
  static void WriteToFile(data_type* data, size_type size,
                          std::string& outfile);

  // Reads an array from file
  template<typename data_type, typename size_type>
  static void ReadFromFile(data_type* data, size_type size,
                           std::string& infile);
};

// Write array to file
template<typename data_type, typename size_type>
void SuccinctUtils::WriteToFile(data_type *data, size_type size,
                                std::string& outfile) {
  std::ofstream out(outfile);
  out.write(reinterpret_cast<const char *>(data), size * sizeof(data_type));
  out.close();
}

// Read array from file
template<typename data_type, typename size_type>
void SuccinctUtils::ReadFromFile(data_type *data, size_type size,
                                 std::string& infile) {
  std::ifstream in(infile);
  in.read(reinterpret_cast<char*>(data), size * sizeof(data_type));
  in.close();
}

#endif
