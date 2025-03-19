#include "dictionary.h"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iomanip>
#include <algorithm>

namespace preprocessor {

namespace {

const unsigned char kCapitalized = 0x40;
const unsigned char kUppercase = 0x07;
const unsigned char kEndUpper = 0x06;
const unsigned char kEscape = 0x0C;
const unsigned char UC0 = 0xC0, UC1 = 0XC1;

const int L1Limit = 47;
const int LTLimit = 60;

std::vector<int> sorted_ascii = {0x65,0x61,0x74,0x69,0x6F,0x6E,0x72,0x73,0x6C,0x68,0x64,0x63,0x6D,0x75,0x70,0x67,0x66,0x79,0x77,0x62,0x76,0x31,0x30,0x6B,0x32,0x43,0x53,0x54,0x41,0x39,0x33,0x35,0x34,0x38,0x4D,0x78,0x49,0x36,0x50,0x42,0x37,0x52,0x44,0x45,0x48,0x4C,0x46,0x47,0x4E,0x57,0x55,0x4A,0x4F,0x7A,0x6A,0x4B,0x71,0x56,0x59,0x5A,0x51,0x58};
// std::vector<int> sorted_ascii = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x7F, 0xC0, 0xC1, 0xDD, 0xDF, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x100};
void EncodeByte(unsigned char c, FILE* output) {
  if (c == kEndUpper || c == kEscape || c == kUppercase ||
      c == kCapitalized || c >= 0x80) {
    putc(kEscape, output);
  }
  putc(c, output);
}

void EncodeBytes(unsigned int bytes, FILE* output) {
  putc((unsigned char)bytes&0xFF, output);
  if (bytes & 0xFF00) {
    putc((unsigned char)((bytes&0xFF00)>>8), output);
  } else {
    return;
  }
  if (bytes & 0xFF0000) {
    putc((unsigned char)((bytes&0xFF0000)>>16), output);
  }
  // if (bytes & 0xFF000000) {
  //   putc((unsigned char)((bytes&0xFF000000)>>24), output);
  // }
}

}
const std::unordered_set<char> SEP = {',', '.', ';', '?', '!', '\n'};

std::vector<std::string> read_most_common_words(const std::string &file_path) {
    std::vector<std::string> words;
    std::ifstream file(file_path);
    std::string word;
    while (std::getline(file, word)) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    return words;
}
// Function to determine the ASCII position in a non-sorted vector
int asciiPosition(int ascii) {
    // Find the iterator pointing to 'ascii' in the vector
    auto it = std::find(sorted_ascii.begin(), sorted_ascii.end(), ascii);
    
    // Check if the ASCII value is found
    if (it != sorted_ascii.end()) {
        int index = std::distance(sorted_ascii.begin(), it);

        // Return 1 if index is within the first 12 elements, otherwise return 2
        return (index < L1Limit) ? 1 : 2;
    }

    // Return 0 if 'ascii' is not found in the vector
    return 0;
}
Dictionary::Dictionary(FILE* dictionary, bool encode, bool decode) {
    
    // Step 1: Prepare ASCII codes array and calculate 
    // getAsciiFrequencies
    // std::unordered_map<int, int> ascii_freq_map;

    // // Use std::ifstream for efficient file reading
    // std::ifstream inputFile(".main_phda9prepr");
    // if (!inputFile) {
    //     std::cerr << "Error: Unable to open input file!" << std::endl;
    //     return;
    // }

    // // Read file contents into a buffer for faster processing
    // char c;
    // while (inputFile.get(c)) {
    //     ascii_freq_map[static_cast<unsigned char>(c)]++;
    // }
    // Find the first three unused ASCII codes
    // int unusedCount = 0;
    // std::cout << "The first three unused ASCII codes are: ";
    // for (int i = 0x80; i < 256; ++i) {
    //     if (ascii_freq_map.find(static_cast<unsigned char>(i)) == ascii_freq_map.end()) {
            // If this ASCII code was never used (not found in the map)
    //         std::cout << i;
    //         unusedCount++;
    //         if (unusedCount < 5) {
    //             std::cout << ", ";
    //         } else {
    //             break;
    //         }
    //     }
    // }

    // std::cout << std::endl;

    // char c;
    // while (c = getc(in)) {
    //     ascii_freq_map[static_cast<unsigned char>(c)]++;
    // }
    // Step 2: Sort ASCII codes by frequency

    // std::vector<std::pair<int, int>> freq_vector(ascii_freq_map.begin(), ascii_freq_map.end());
    // std::sort(freq_vector.begin(), freq_vector.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
    //     return a.second > b.second;
    // });

    // std::vector<int> sorted_ascii = {0x65,0x61,0x74,0x69,0x6F,0x6E,0x72,0x73,0x6C,0x68,0x64,0x63,0x6D,0x75,0x70,0x0A,0x67,0x66,0x5D,0x5B,0x79,0x2E,0x27,0x77,0x62,0x2C,0x76,0x31, 0x3E,0x30,0x7C,0x3D,0x6B,0x2F,0x32,0x43,0x53,0x3C,0x3A,0x54,0x2D,0x41,0x39,0x2A,0x33,0x35,0x29,0x28,0x34,0x38,0x22,0x4D,0x78,0x49,0x36,0x50,0x42,0x37,0x52,0x26,0x44,0x45,0x48,0x4C,0x46,0x47,0x4E,0x57,0x55,0x25,0x4A,0x4F,0x7A,0x6A,0x3B,0x4B,0x71,0x7B,0x56,0x7D,0x5F,0xC3,0x23,0xD0,0x59,0x21,0x24,0xE3,0xD7,0x80,0x83,0x05,0x5A,0xB0,0xA9,0x5C,0x51,0xE2,0xD1,0x58,0xB8,0xC2,0x3F,0x82,0xE0,0xA1,0x5E,0xE5,0x99,0x2B,0xBC,0xB3,0xA4,0x81,0xC5,0xCE,0x94,0xAD,0xBD,0xBE,0xB5,0xB2,0xC4,0xD8,0xA8,0x93,0xBA,0xA7,0xBB,0xE6,0xB9,0x9C,0xB1,0xB6,0xB4,0x09,0xD9,0x95,0x84,0xA3,0x8D,0x88,0xAB,0xE1,0xAA,0xA0,0x90,0xE7,0x9D,0xA5,0xA6,0x85,0xB7,0x98,0x89,0xBF,0xAF,0x97,0x9F,0xCF,0x8C,0xA2,0xE8,0x87,0x86,0xAE,0xE9,0x91,0x9B,0x9E,0x96,0x8B,0x8A,0x8F,0xE4,0xAC,0xEC,0x7E,0x92,0x9A,0xEB,0x8E,0xC9,0xCA,0xED,0xCB,0x40,0xEA,0xD6,0xDB,0xCC,0xC7,0xEF,0x60,0xDA,0xD5,0xC6,0xD2,0xDC,0xD3,0xC8,0xCD,0xDE,0xF0,0xD4,0xEE,0x03};
    // for (const auto& pair : freq_vector) {
    //     sorted_ascii.push_back(pair.first);
    // }

    // Step 2: Generate symbols (1, 2, 3 letter combinations)
    std::vector<std::string> symbols;

    // Generate 1-letter symbols
    int limit1 = L1Limit;
    // std::cout << sorted_ascii.size() << std::endl;
    for (size_t i = 0; i < sorted_ascii.size() && i < limit1; ++i) {
        // Print the value in hexadecimal format
        // std::cout << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                  // << sorted_ascii[i] << ",";
        symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[i])));
    }

    // Generate 2-letter symbols
    int limit2 = 0;
    for (size_t i = limit1; i < sorted_ascii.size() && i < limit1+limit2; ++i) {
        for (size_t j = limit1; j < sorted_ascii.size() && j < limit1+limit2; ++j) {
            symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[i])) + static_cast<char>(sorted_ascii[j]));
        }
    }

    // Generate 3-letter symbols
    int limit3 = LTLimit - L1Limit; // means 20 * 60 * 60
    for (size_t i = limit1+limit2; i < sorted_ascii.size() && i < limit1+limit2+limit3; ++i) {
        for (size_t j = 0; j < sorted_ascii.size() && j < LTLimit; ++j) {
            for (size_t k = 0; k < sorted_ascii.size() && k < LTLimit; ++k) {
              if( j != k)
                symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[k])) + static_cast<char>(sorted_ascii[j]) + static_cast<char>(sorted_ascii[i]));
            }
        }
    }
    std::cout << "Step 2: Generated symbols (1-letter, 2-letter, 3-letter combinations)." << std::endl;

    // Step 4 Convert symbols into int Array
    std::vector<int> symbol_offset;

    // Iterate over each symbol
    for (const auto& symbol : symbols) {
        int intValue = 0;  // Initialize the integer to store the packed bytes
        
        // Ensure the symbol is not longer than 3 characters (as per the requirement)
        size_t length = symbol.size() < 3 ? symbol.size() : 3;

        // Pack the first 3 characters into the integer (each character as a byte)
        for (size_t i = 0; i < length; ++i) {
            intValue |= (static_cast<unsigned char>(symbol[i]) << (8 * (length - 1 - i)));
        }

        // Save the result to the intArray
        symbol_offset.push_back(intValue);
    }

    fseek(dictionary, 0L, SEEK_END);
    unsigned long long len = ftell(dictionary);
    fseek(dictionary, 0L, SEEK_SET);
    std::string line;
    size_t index = 0;
    int line_count = symbol_offset[index];

    const int kBoundary1 = 0xff, kBoundary2 = kBoundary1 + 0xffff,
        kBoundary3 = kBoundary2 + 0xffffff;
    for (unsigned pos = 0; pos < len; ++pos) {
        unsigned char c = getc(dictionary);
        if (c >= 'a' && c <= 'z') line += c;
        else if (!line.empty()) {
            if (line.size() > longest_word_) longest_word_ = line.size();
            unsigned int bytes;
            if (line_count < kBoundary1) {
                bytes = 0x80 + line_count;
                // bytes = (line_count << 8) + UC0;
            } else if (line_count < kBoundary2) {
                // bytes = (line_count << 8) + UC1;
                bytes = 0x8080 + line_count;
            } else if (line_count < kBoundary3) {
                bytes = 0x808080 + line_count;
                // bytes = (line_count << 8) + UC2;
            }
            line_count=symbol_offset[++index];
            if (encode) byte_map_[line] = bytes;
            if (decode) reverse_map_[bytes] = line;
            line.clear();
        }
    }

}

void Dictionary::Encode(FILE* input, int len, FILE* output) {
  std::string word;
  int num_upper = 0, num_lower = 0;
  for (int pos = 0; pos < len; ++pos) {
    unsigned char c = getc(input);
    bool advance = false;
    if (word.size() > longest_word_) {
      advance = true;
    } else if (c >= 'a' && c <= 'z') {
      if (num_upper > 1) {
        advance = true;
      } else {
        ++num_lower;
        word += c;
      }
    } else if (c >= 'A' && c <= 'Z') {
      if (num_lower > 0) {
        advance = true;
      } else {
        ++num_upper;
        word += (c - 'A' + 'a');
      }
    } else {
      advance = true;
    }
    if (pos == len - 1 && !advance) {
      EncodeWord(word, num_upper, false, output);
    }
    if (advance) {
      if (word.empty()) {
        EncodeByte(c, output);
      } else {
        bool next_lower = (c >= 'a' && c <= 'z');
        EncodeWord(word, num_upper, next_lower, output);
        num_lower = 0;
        num_upper = 0;
        word.clear();
        if (next_lower) {
          ++num_lower;
          word += c;
        } else if (c >= 'A' && c <= 'Z') {
          ++num_upper;
          word += (c - 'A' + 'a');
        } else {
          EncodeByte(c, output);
        }
        if (pos == len - 1 && !word.empty()) {
          EncodeWord(word, num_upper, false, output);
        }
      }
    }
  }
}
int co = 0;
void Dictionary::EncodeWord(const std::string& word, int num_upper,
    bool next_lower, FILE* output) {
  if (num_upper > 1) putc(kUppercase, output);
  else if (num_upper == 1) putc(kCapitalized, output);
  auto it = byte_map_.find(word);
  // if(++co >= 20)
  //   return;
  if (it != byte_map_.end()) {
    // std::cout << word << '\t';
    // std::cout << std::hex << it->second << std::endl;
    EncodeBytes(it->second, output);
  } else if (!EncodeSubstring(word, output)) {
    for (unsigned int i = 0; i < word.size(); ++i) {
      putc((unsigned char)word[i], output);
    }
  }
  if (num_upper > 1 && next_lower) {
    putc(kEndUpper, output);
  }
}

bool Dictionary::EncodeSubstring(const std::string& word, FILE* output) {
  if (word.size() <= 7) return false;
  unsigned int size = word.size() - 1;
  if (size > longest_word_) size = longest_word_;
  std::string suffix = word.substr(word.size() - size, size);
  while (suffix.size() >= 7) {
    auto it = byte_map_.find(suffix);
    if (it != byte_map_.end()) {
      for (unsigned int i = 0; i < word.size() - suffix.size(); ++i) {
        putc((unsigned char)word[i], output);
      }
      EncodeBytes(it->second, output);
      return true;
    }
    suffix.erase(0, 1);
  }
  std::string prefix = word.substr(0, size);
  while (prefix.size() >= 7) {
    auto it = byte_map_.find(prefix);
    if (it != byte_map_.end()) {
      EncodeBytes(it->second, output);
      for (unsigned int i = prefix.size(); i < word.size(); ++i) {
        putc((unsigned char)word[i], output);
      }
      return true;
    }
    prefix.erase(prefix.size() - 1, 1);
  }
  return false;
}

unsigned char Dictionary::Decode(FILE* input) {
  while (output_buffer_.empty()) {
    AddToBuffer(input);
  }
  unsigned char next = output_buffer_.front();
  output_buffer_.pop_front();
  return next;
}

unsigned char NextChar(FILE* input) {
  int c = getc(input);
  if (c>='{' && c<127) c+='P'-'{';
  else if (c>='P' && c<'T') c-='P'-'{';
  else if ( (c>=':' && c<='?') || (c>='J' && c<='O') ) c^=0x70;
  if (c=='X' || c=='`') c^='X'^'`';
  return c;
}

void Dictionary::AddToBuffer(FILE* input) {
  unsigned char c = NextChar(input);
  if (c == kEscape) {
    decode_upper_ = false;
    output_buffer_.push_back(NextChar(input));
  } else if (c == kUppercase) {
    decode_upper_ = true;
  } else if (c == kCapitalized) {
    decode_capital_ = true;
  } else if (c == kEndUpper) {
    decode_upper_ = false;
  } else if (c >= 0x80) { 
    int state_c = asciiPosition(c-0x80);
    int bytes = c;// means 1 byte
    if(state_c == 2){ // means 3 bytes
      c = NextChar(input);
      bytes = bytes + (c << 8);
      c = NextChar(input);
      bytes = bytes + (c << 16);
    }
    // if(++co >= 20)
    //   return;
    std::string word = reverse_map_[bytes];
    // std::cout << word << '\t';
    // std::cout << std::hex << bytes << std::endl;
    for (unsigned int i = 0; i < word.size(); ++i) {
      if (i == 0 && decode_capital_) {
        word[i] = (word[i] - 'a') + 'A';
        decode_capital_ = false;
      }
      if (decode_upper_) {
        word[i] = (word[i] - 'a') + 'A';
      }
      output_buffer_.push_back(word[i]);
    }
  } else {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
      decode_upper_ = false;
    }
    if (decode_capital_ || decode_upper_) {
      c = (c - 'a') + 'A';
    }
    if (decode_capital_) decode_capital_ = false;
    output_buffer_.push_back(c);
  }
}

}
