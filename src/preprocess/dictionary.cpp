#include "dictionary.h"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iomanip>

namespace preprocessor {

namespace {

const unsigned char kCapitalized = 0x40;
const unsigned char kUppercase = 0x07;
const unsigned char kEndUpper = 0x06;
const unsigned char kEscape = 0x0C;
const unsigned char UC0 = 192, UC1 = 193, UC2 = 221;

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
  if (bytes & 0xFF000000) {
    putc((unsigned char)((bytes&0xFF000000)>>24), output);
  }
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
Dictionary::Dictionary(FILE* dictionary, bool encode, bool decode) {
    
    // Step 1: Prepare ASCII codes array and calculate 
    // getAsciiFrequencies
    // std::unordered_map<int, int> ascii_freq_map;

    // Use std::ifstream for efficient file reading
    // std::ifstream inputFile(".main_phda9prepr");
    // if (!inputFile) {
    //     std::cerr << "Error: Unable to open input file!" << std::endl;
    //     return;
    // }

    // Read file contents into a buffer for faster processing
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

    std::vector<int> sorted_ascii = {0x20,0x65,0x61,0x74,0x69,0x6F,0x6E,0x72,0x73,0x6C,0x68,0x64,0x63,0x6D,0x75,0x70,0x0A,0x67,0x66,0x5D,0x5B,0x79,0x2E,0x27,0x77,0x62,0x2C,0x76,0x31,0x3E,0x30,0x7C,0x3D,0x6B,0x2F,0x32,0x43,0x53,0x3C,0x3A,0x54,0x2D,0x41,0x39,0x2A};
    // for (const auto& pair : freq_vector) {
    //     sorted_ascii.push_back(pair.first);
    // }

    // Step 2: Generate symbols (1, 2, 3 letter combinations)
    std::vector<std::string> symbols;

    // Generate 1-letter symbols
    int limit = 45;
    for (size_t i = 0; i < sorted_ascii.size() && i < limit; ++i) {
        // Print the value in hexadecimal format
        std::cout << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                  << sorted_ascii[i] << ",";
        symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[i])));
    }

    // Generate 2-letter symbols
    for (size_t i = 0; i < sorted_ascii.size() && i < limit; ++i) {
        for (size_t j = 0; j < sorted_ascii.size() && j < limit; ++j) {
            symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[i])) + static_cast<char>(sorted_ascii[j]));
        }
    }

    // Generate 3-letter symbols
    for (size_t i = 0; i < sorted_ascii.size() && i < limit; ++i) {
        for (size_t j = 0; j < sorted_ascii.size() && j < limit; ++j) {
            for (size_t k = 0; k < sorted_ascii.size() && k < limit; ++k) {
                symbols.push_back(std::string(1, static_cast<char>(sorted_ascii[i])) + static_cast<char>(sorted_ascii[j]) + static_cast<char>(sorted_ascii[k]));
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
                bytes = (line_count << 8) + UC0;
            } else if (line_count < kBoundary2) {
                bytes = (line_count << 8) + UC1;
            } else if (line_count < kBoundary3) {
                bytes = (line_count << 8) + UC2;
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

void Dictionary::EncodeWord(const std::string& word, int num_upper,
    bool next_lower, FILE* output) {
  if (num_upper > 1) putc(kUppercase, output);
  else if (num_upper == 1) putc(kCapitalized, output);
  auto it = byte_map_.find(word);
  // std::cout << word << '\t';
  // std::cout << std::hex << it->second << std::endl;
  if (it != byte_map_.end()) {
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
  } else if (c == UC0 || c == UC1 || c == UC2) { // means 1 byte
    unsigned int bytes = 0;
    unsigned char flag = c;
    c = NextChar(input);
    if(flag == UC0) // means 1 byte
        bytes = c;
    else if(flag == UC1){ // means 2 bytes
        bytes = c;
        c = NextChar(input);
        bytes +=  c << 8;
    }
    else if (flag == UC2) { // means 3 bytes
        bytes = c;
        c = NextChar(input);
        bytes +=  c << 8;
        c = NextChar(input);
        bytes +=  c << 16;
    }
    bytes = (bytes << 8) + flag; // add C0 | C1 | C2 at the end.

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
