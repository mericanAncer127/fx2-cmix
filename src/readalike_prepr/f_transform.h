#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>

using namespace std;

// Utility to convert a string to lower case (not required in this case)
std::string to_lower(const std::string &str) {
    std::string result = str;
    for (char &ch : result) {
        ch = std::tolower(ch);
    }
    return result;
}

// Utility to trim spaces from the left side of the string
std::string lstrip(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : str.substr(start);
}

// Utility to remove '\n' from the right end of the string (equivalent to rstrip(b'\n') in Python)
std::string rstrip_newline(const std::string &str) {
    if (!str.empty() && str.back() == '\n') {
        return str.substr(0, str.size() - 1);  // Remove the trailing newline character
    }
    return str;
}
// Utility to trim spaces from the right side of the string
std::string rstrip(const std::string &str) {
    size_t end = str.find_last_not_of(" \t\r\n");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

// Utility to trim spaces from both sides of the string (equivalent to strip in Python)
std::string strip(const std::string &str) {
    return rstrip(lstrip(str));
}

// Helper function to write the pattern map
void write_pattern_map(const std::unordered_map<std::string, std::string>& pattern_identifiers) {
    std::ofstream mapfile("pattern_map.txt");
    if (mapfile.is_open()) {
        for (const auto& pair : pattern_identifiers) {
            mapfile << pair.second << ": " << pair.first << "\n";
        }
        mapfile.close();
    } else {
        std::cerr << "Error: Could not write the pattern map." << std::endl;
    }
}

// Main transformation function
void pat_transform() {
    std::unordered_map<std::string, int> tag_patterns;  // Stores the frequency of patterns
    std::unordered_map<std::string, std::string> pattern_identifiers; // Maps patterns to unique identifiers
    char max_unique_id = 'a';

    // Tags and their associated characters in a vector to preserve insertion order
    std::vector<std::pair<std::string, char>> tag_chars = {
        {"<comment>", 'c'}, {"</comment>", 'c'},
        {"<page>", 'p'}, {"</page>", 'p'},
        {"<title>", 't'}, {"</title>", 't'},
        {"<text ", 'x'}, {"</text>", 'x'},
        {"<minor />", 'm'},
        {"</revision>", 'r'}
    };

    std::ifstream infile(".main_phda9prepr", std::ios::binary);
    if (!infile) {
        std::cerr << "Error: The file '" << ".main_phda9prepr" << "' was not found." << std::endl;
        return;
    }

    // Output file
    std::ofstream outfile(".pat.main_phda9prepr", std::ios::binary);
    if (!outfile) {
        std::cerr << "Error: Could not create output file." << std::endl;
        return;
    }

    std::string current_pattern;
    std::string st_line;
    bool the_end = false;
    std::string line;
    
    // Process each line in the input file
    bool last_line_is_page_end = false;  // To detect very last article
    while (std::getline(infile, line)) {
        if (the_end) {
            outfile << line << std::endl;
            continue;
        }

        std::string stripped_line = strip(line);  // Apply strip() to remove spaces around the line
        bool tag_found = false;
        std::string ex_tag1, ex_tag2;

        //Last article detection
        if(last_line_is_page_end ) { // Means the last article
            if(stripped_line.find("<page>") == std::string::npos) {
                std::cout << "the end" << std::endl;
                the_end = true;
                outfile << line << std::endl;
                continue;
            }
            last_line_is_page_end = false;
        }
        // Check for tag patterns, now iterating in insertion order using the vector
        for (const auto& tag_pair : tag_chars) {
            const std::string& tag = tag_pair.first;
            char tag_char = tag_pair.second;

            if (stripped_line.rfind(tag) != std::string::npos && stripped_line.rfind(tag) == stripped_line.length() - tag.length()) { // order is important. becase only-tag lines are considered as ending with that tag.
                // std::cout << tag << '\t'<< stripped_line << '\t' << stripped_line.rfind(tag) << std::endl;
                // Ends with tag
                if (current_pattern.find(std::string(1, tag_char) + std::string(1, tag_char)) == std::string::npos) {
                    if (tag == "</comment>" && stripped_line != st_line) { // Should review here at last pttCcxxrp pattern found
                        current_pattern += std::toupper(tag_char);
                    } else {
                        current_pattern += tag_char;
                    }
                    tag_found = true;
                    ex_tag2 = tag;
                }
                continue;
            }

            if (stripped_line.find(tag) == 0) {
                // Starts with tag
                if (current_pattern.find(std::string(1, tag_char) + std::string(1, tag_char)) == std::string::npos) {
                    current_pattern += tag_char;
                    st_line = stripped_line;
                    tag_found = true;
                    ex_tag1 = tag;
                }
                continue;
            }
        }
        // End of article detection
        if (stripped_line.find("</page>") == 0) {
            last_line_is_page_end = true;
            // if (stripped_line != "</page>") {
                // the_end = true;
            // }

            // Assign a unique identifier to the pattern if it's new
            if (tag_patterns[current_pattern] == 0) {
                tag_patterns[current_pattern] = 1;
                pattern_identifiers[current_pattern] = std::string(1, max_unique_id);
                max_unique_id = max_unique_id + 1;
            } else {
                tag_patterns[current_pattern]++;
            }

            // Replace the </page> tag with the pattern identifier
            size_t pos = stripped_line.find("</page>");
            if (pos != std::string::npos) {
                stripped_line.replace(pos, 7, "</page>" + pattern_identifiers[current_pattern]);
            }
            outfile << stripped_line << std::endl;

            // Reset for the next article
            current_pattern.clear();
        } else if (tag_found) {
            // Remove tags and write the line
            if (!ex_tag1.empty() && stripped_line.find(ex_tag1) == 0) {
                if (ex_tag1 == "</text>") {
                    line.replace(0, ex_tag1.length(), "");
                } else {
                    line = lstrip(line);
                    line.replace(0, ex_tag1.length(), "");
                }
            }

            if (!ex_tag2.empty() && stripped_line.rfind(ex_tag2) == stripped_line.length() - ex_tag2.length()) {
                line.replace(line.length() - ex_tag2.length(), ex_tag2.length(), "");
            }

            // Apply rstrip(b'\n') behavior: remove newline from the end if present
            line = rstrip_newline(line);

            outfile << line << std::endl;
            tag_found = false;
            ex_tag1.clear();
            ex_tag2.clear();
        } else {
            // Write non-tag content directly
            outfile << line << std::endl;
        }
    }

    // Write the pattern map
    write_pattern_map(pattern_identifiers);

    std::cout << "Transformation completed. Patterns saved in 'pattern_map.txt'." << std::endl;
}


// Function to insert a string after leading spaces of an existing string
std::string insert_string(const std::string &org_str, const std::string &ins_str) {
    size_t index = org_str.find_first_not_of(" \t\r\n");
    return org_str.substr(0, index) + ins_str + org_str.substr(index);
}

// Function to perform reverse transformation
void pat_de_transform() {
    // Pattern map for each character to its corresponding tag sequence
    std::unordered_map<char, std::string> pattern_map = {
        {'a', "pttmccxxrp"},
        {'b', "pttmccxrp"},
        {'c', "pttccxrp"},
        {'d', "pttxrp"},
        {'e', "pttmxrp"},
        {'f', "pttccxxrp"},
        {'g', "pttxxrp"},
        {'h', "pttmxxrp"},
        {'i', "pttcCxxrp"},
        {'j', "pttmcCxrp"},
        {'k', "pttmcCxxrp"},
        {'l', "pttcCxrp"}
    };

    // Tags corresponding to the characters p, t, c, x, m, r (with exact spaces)
    std::unordered_map<char, std::vector<std::string>> tag_map = {
        {'p', {"  <page>", "  </page>"}},
        {'t', {"    <title>", "</title>"}},
        {'c', {"      <comment>", "</comment>"}},
        {'x', {"      <text ", "</text>"}},
        {'m', {"      <minor />"}},
        {'r', {"    </revision>"}}
    };

    // Open input and output files
    std::ifstream infile(".pat.main_phda9prepr", std::ios::binary);
    if (!infile) {
        std::cerr << "Error: The file '" << ".pat.main_phda9prepr" << "' was not found." << std::endl;
        return;
    }

    std::ofstream outfile(".main_phda9prepr_detransformed", std::ios::binary);
    if (!outfile) {
        std::cerr << "Error: Could not create output file." << std::endl;
        return;
    }

    std::string line;
    std::vector<std::string> current_article;
    std::vector<std::string> new_article;

    std::getline(infile, line);
    line += "\n"; 
    outfile.write(line.c_str(), line.size());

    // bool last_line_was_page_end = false;
    // bool the_end = false;

    while (std::getline(infile, line)) {
        // Strip leading and trailing spaces
        std::string stripped_line = strip(line);

        // if(last_line_was_page_end) {
        //     if(!stripped_line.empty()){
        //         the_end = true;
        //         // output
        //         continue;
        //     }
        //     last_line_was_page_end = false
        // }
        
        // Check if it's an end-of-page pattern identifier
        if (stripped_line.find("</page>") != std::string::npos && stripped_line.find("</page>") == 0) {
            // last_line_was_page_end = true;
            char pattern_id = stripped_line[7];  // The identifier is just after the `</page>` tag
            std::string current_pattern = pattern_map[pattern_id]; // Map to the corresponding pattern string

            // Track which tags we have seen in this article
            std::unordered_map<char, bool> tags_count;

            size_t current_pos = 0;
            for (char tag_key : current_pattern) {
                char org_tag_key = tag_key;  // Preserve the original case for logic
                tag_key = std::tolower(tag_key); // Work with lowercase for comparison

                if (tag_key == 'p' || tag_key == 'm' || tag_key == 'r') {
                    // Handle page, minor, and revision tags
                    if (tags_count[tag_key]) {
                        if (tag_key == 'p') {
                            new_article.push_back(tag_map[tag_key][1] + stripped_line.substr(8) + "\n"); // Closing <page> tag with extra content
                        } else {
                            new_article.push_back(tag_map[tag_key][1] + "\n"); // Closing tag
                        }
                    } else {
                        new_article.push_back(tag_map[tag_key][0] + "\n"); // Opening tag
                        tags_count[tag_key] = true;
                    }
                    // Remove the first element of current_article (equivalent to popping the front element)
                    current_article.erase(current_article.begin());  // move to the next line if tag is closed

                } else if ((tag_key == 't' || tag_key == 'c') && !current_article.empty()) {
                    // Handle title and comment tags
                    if (tags_count[tag_key]) {
                        if (org_tag_key == 'C') {
                            while (current_article.size() > 0 && current_article[1].find("xml") != 0) { // Could error because of npos
                                current_article.erase(current_article.begin()); // Move to next line if uppercase
                                new_article.back() += current_article[0] + '\n'; // Concatenate to the last line of new_article
                            }
                        }

                        std::string cur_line = new_article.back();
                        new_article.back() = cur_line.substr(0, cur_line.size() - 1) + tag_map[tag_key][1] + "\n"; // Close the tag
                        current_article.erase(current_article.begin()); // Pop front element
                    } else {
                        new_article.push_back(tag_map[tag_key][0] + current_article[0] + "\n"); // Opening tag with content
                        tags_count[tag_key] = true;
                    }

                } else if (tag_key == 'x' && current_article.size() > 0) {
                    if (tags_count[tag_key]) {  // </text> - closing tag
                        // Update the last line of the new_article: remove the last newline and append the closing </text> tag
                        new_article.back() = new_article.back().substr(0, new_article.back().size()-1) + tag_map[tag_key][1] + "\n";
                        continue;
                    } else {
                        tags_count[tag_key] = true;
                    }

                    // Accumulate content for <text>
                    std::string text_content;
                    for (size_t i = 0; i < current_article.size() - 1; ++i) {
                        text_content += current_article[i]+'\n';  // Accumulate all lines except the last one
                    }

                    new_article.push_back(tag_map[tag_key][0] + text_content);  // Add <text> tag with content
                }

                current_pos++;
            }
            
            // Combine all strings in new_article into one and write it to the file
            std::ostringstream oss;
            for (const auto& str : new_article) {
                oss << str;  // Append each string to the ostringstream
            }
            outfile.write(oss.str().c_str(), oss.str().size());

            new_article.clear();
            current_article.clear();
        } else {
            current_article.push_back(line);
        }
    }

    // Write any remaining content
    if (!current_article.empty()) {
        std::ostringstream oss;
        for (const auto& str : current_article) {
            oss << str << std::endl;  // Append each string to the ostringstream
        }
        outfile.write(oss.str().c_str(), oss.str().size());
        // outfile.write(current_article.back().c_str(), current_article.back().size());
    }

    std::cout << "De-transformation completed." << std::endl;
}

int transform() {
const vector<string> replaceStrings = {
      "<title>", "</title>",
      "<id>", "</id>",
      "<restrictions>", "</restrictions>",
      "<id>", "</id>",
      "<timestamp>", "</timestamp>",
      "<username>", "</username>",
      "<id>", "</id>",
      "<ip>", "</ip>",
      "<minor />", "<minor />",
      "<comment>", "</comment>"
  };

  vector<string> xml_data(10);

  // Open the binary file for reading
  string file_name = ".pat.main_phda9prepr";
  string result_file_name = ".ex_pat.main_phda9prepr";

  bool isStarted = false;
  bool isCommentStarted = false;
  bool isTextStarted = false;
  string page_data;
  string text_data;
  string comment_data;
  int cnt = 0;

  ifstream input_file(file_name);
  ofstream output_file(result_file_name, ios::binary);

  if (!input_file.is_open()) {
      cerr << "Failed to open the input file!" << endl;
      return 1;
  }

  if (!output_file.is_open()) {
      cerr << "Failed to open the output file!" << endl;
      return 1;
  }

  string line;
  while (getline(input_file, line)) {
      string stripped_line = strip(line);
      page_data.append(line).append("\n");

      if (stripped_line.find("<comment>") != string::npos) {
          comment_data.clear();
          isCommentStarted = true;
      }

      if (isCommentStarted) {
          comment_data.append(line).append("\n");
          if (stripped_line.find("</comment>") != string::npos) {
              isCommentStarted = false;
          }
          continue;
      }

      if (stripped_line.find("<text") != string::npos) {
          if (!isTextStarted) {
              cnt++;
              // cout << cnt << endl;
              text_data.clear();
              isTextStarted = true;
          }
      }

      if (isTextStarted) {
          text_data.append(line).append("\n");
          if (stripped_line.find("</text>") != string::npos || 
              (stripped_line.find("/>") != string::npos && stripped_line.find("<text") != string::npos)) {
              isTextStarted = false;
          }
          continue;
      }

      if (stripped_line.find("<page>") != string::npos) {
          fill(xml_data.begin(), xml_data.end(), "");
          isStarted = true;
      }

      if (isStarted) {
          for (size_t index = 0; index < 9; ++index) {
              if (stripped_line.find(replaceStrings[index * 2]) != string::npos) {
                  if (index == 1 && !xml_data[index].empty()) {
                      index += 2;  // revision id
                  }
                  if (index == 3 && !xml_data[index].empty()) {
                      index += 3;  // customer id
                  }
                  if (index == 8) {
                      xml_data[index] = "_";
                  } else {
                      string temp = stripped_line;
                      temp.erase(0, replaceStrings[index * 2].length());
                      temp.erase(temp.length() - replaceStrings[index * 2 + 1].length());
                      xml_data[index] = temp;
                  }
                  break;
              }
          }
      } else {
          output_file << line << "\n";
      }

      if (stripped_line.find("</page>") != string::npos) {
          output_file << xml_data[0] << "\n"
                      << xml_data[1] << "\n"
                      << xml_data[2] << "\n"
                      << xml_data[3] << "\n"
                      << xml_data[4] << "\n"
                      << xml_data[5] << "\n"
                      << xml_data[6] << "\n"
                      << xml_data[7] << "\n"
                      << xml_data[8] << "\n"
                      << comment_data
                      << text_data;
          page_data.clear();
          comment_data.clear();
      }
  }

  if (!page_data.empty()) {
      output_file << page_data;
  }

  input_file.close();
  output_file.close();

  return 0;
}

int de_transform() {
  string source_file_name = ".main_decomp";
  string result_file_name = ".de_ex_transformed_main";

  bool isStarted = false;
  bool isCommentStarted = false;
  bool isTextStarted = false;
  string page_data = "";
  string comment_data = "";
  string text_data = "";
  int cnt = 0;

  ifstream input_file(source_file_name);
  ofstream output_file(result_file_name, ios::binary);

  if (!input_file.is_open()) {
      cerr << "Failed to open the input file!" << endl;
      return 1;
  }

  if (!output_file.is_open()) {
      cerr << "Failed to open the output file!" << endl;
      return 1;
  }

  string line;
  while (getline(input_file, line)) {
      if (line.find("<comment>") != string::npos) {
          isCommentStarted = true;
          comment_data = "";
      }
      if (isCommentStarted) {
          comment_data += line + "\n";
      }
      if (line.find("</comment>") != string::npos) {
          isCommentStarted = false;
      }

      if (line.find("<text") != string::npos) {
          isTextStarted = true;
          text_data = "";
      }
      if (isTextStarted) {
          text_data += line + "\n";
      }

      if (!isTextStarted && !isCommentStarted) {
          page_data += line + "\n";
      }

      if (line.find("</text>") != string::npos || 
          (line.find("<text") != string::npos && line.find("/>") != string::npos)) {
          cnt++;
          // cout << cnt << endl;

          istringstream ss(page_data);
          vector<string> data_list;
          string temp_line;
          while (getline(ss, temp_line)) {
              data_list.push_back(temp_line);
          }
          page_data = "";

          output_file << "  <page>\n";
          output_file << "    <title>" << data_list[0] << "</title>\n";
          output_file << "    <id>" << data_list[1] << "</id>\n";
          if (!data_list[2].empty()) {
              output_file << "    <restrictions>" << data_list[2] << "</restrictions>\n";
          }
          output_file << "    <revision>\n";
          if (!data_list[3].empty()) {
              output_file << "      <id>" << data_list[3] << "</id>\n";
          }
          if (!data_list[4].empty()) {
              output_file << "      <timestamp>" << data_list[4] << "</timestamp>\n";
          }
          output_file << "      <contributor>\n";
          if (!data_list[5].empty()) {
              output_file << "        <username>" << data_list[5] << "</username>\n";
          }
          if (!data_list[6].empty()) {
              output_file << "        <id>" << data_list[6] << "</id>\n";
          }
          if (!data_list[7].empty()) {
              output_file << "        <ip>" << data_list[7] << "</ip>\n";
          }
          output_file << "      </contributor>\n";
          if (data_list[8] == "_") {
              output_file << "      <minor />\n";
          }
          output_file << comment_data;
          output_file << text_data;
          output_file << "    </revision>\n";
          output_file << "  </page>\n";

          isTextStarted = false;
          comment_data = "";
      }

      if (line.find("</siteinfo>") != string::npos) {
          output_file << page_data;
          page_data = "";
      }
  }

  output_file << page_data;
  output_file << text_data;

  input_file.close();
  output_file.close();

  return 0;
}