#include "dictionary.h"

#include <stdio.h>
#include <fstream>
#include <string>
#include <unordered_set>

namespace preprocessor {

namespace {

const unsigned char kCapitalized = 0x40;
const unsigned char kUppercase = 0x07;
const unsigned char kEndUpper = 0x06;
const unsigned char kEscape = 0x0C;

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
Dictionary::Dictionary(std::string s, FILE* dictionary, bool encode, bool decode, bool firn) {
  auto most_common_words = read_most_common_words("dict");
  if(firn == false)
    Dictionary(dictionary, encode, decode);
    
  std::unordered_map<char, int> char_freq;
  for (char ch : s) {
      char_freq[ch]++;
  }

  // Sort characters by frequency and select top symbols
  std::vector<char> symbols;
  for (const auto &[ch, freq] : char_freq) {
      if (ch != ' ') symbols.push_back(ch);
  }
  std::sort(symbols.begin(), symbols.end(), [&](char a, char b) {
      return char_freq[a] > char_freq[b];
  });
  if (symbols.size() > 35) symbols.resize(35);

  // Generate multi-character symbols
  // Initialize full_symbols with single-character strings
  
  std::vector<unsigned int> full_symbols;
  for (char ch : symbols) {
      full_symbols.push_back(ch); // Convert each char to a string
  }

  // Continue to add multi-character symbols as per the original code
  for (char l0 : symbols) {
      for (char l1 : symbols) {
          if (SEP.find(l1) == SEP.end()) {
              full_symbols.push_back((l0 << 8) + l1);
          }
      }
  }
  for (char l0 : symbols) {
      for (char l1 : symbols) {
          for (char l2 : symbols) {
              if (SEP.find(l2) == SEP.end()) {
                  full_symbols.push_back((l0 << 16) + (l1 << 8) + l2);
              }
          }
      }
  }

  // Map most common words to symbols
  std::unordered_map<std::string, std::string> word_to_symbol;
  for (size_t i = 0; i < most_common_words.size() && i < full_symbols.size(); ++i) {
      byte_map_[most_common_words[i]] = full_symbols[i];
  }
}
Dictionary::Dictionary(FILE* dictionary, bool encode, bool decode) {
  fseek(dictionary, 0L, SEEK_END);
  unsigned long long len = ftell(dictionary);
  fseek(dictionary, 0L, SEEK_SET);
  std::string line;
  int line_count = 0;
  const int kBoundary1 = 80, kBoundary2 = kBoundary1 + 3840,
      kBoundary3 = kBoundary2 + 40960;
  for (unsigned pos = 0; pos < len; ++pos) {
    unsigned char c = getc(dictionary);
    if (c >= 'a' && c <= 'z') line += c;
    else if (!line.empty()) {
      if (line.size() > longest_word_) longest_word_ = line.size();
      unsigned int bytes;
      if (line_count < kBoundary1) {
        bytes = 0x80 + line_count;
      } else if (line_count < kBoundary2) {
        bytes = 0xD0 + ((line_count-kBoundary1) / 80);
        bytes += (0x80 + ((line_count-kBoundary1) % 80)) << 8;
      } else if (line_count < kBoundary3) {
        bytes = 0xF0 + (((line_count-kBoundary2) / 80) / 32);
        bytes += (0xD0 + (((line_count-kBoundary2) / 80) % 32)) << 8;
        bytes += (0x80 + ((line_count-kBoundary2) % 80)) << 16;
      }
      if (encode) byte_map_[line] = bytes;
      if (decode) reverse_map_[bytes] = line;
      ++line_count;
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
  } else if (c >= 0x80) {
    unsigned int bytes = c;
    if (c > 0xCF) {
      c = NextChar(input);
      bytes += c << 8;
      if (c > 0xCF) {
        c = NextChar(input);
        bytes += c << 16;
      }
    }
    std::string word = reverse_map_[bytes];
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
