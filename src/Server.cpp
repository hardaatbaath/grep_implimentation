#include <iostream>
#include <string>

bool match_pattern(const std::string& input_line, const std::string& pattern, const int& input_pos, int& pattern_pos) {
    // Check bounds
    if (input_pos >= input_line.length() || pattern_pos >= pattern.length()) return false;
    if (pattern.at(pattern_pos) == '\\') {
        pattern_pos++;
        if (pattern_pos >= pattern.length()) return false; // Check bounds after increment
        if(pattern.at(pattern_pos) == 'd') return (std::string(1, input_line.at(input_pos)).find_first_of("1234567890") != std::string::npos);
        else if(pattern.at(pattern_pos) == 'w') return (std::string(1, input_line.at(input_pos)).find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890") != std::string::npos);
    }
    else if (input_line.at(input_pos) == pattern.at(pattern_pos)) return true; // Just return true, don't recurse
    else return false;
    return true;
}

//         return input_line.find(pattern) != std::string::npos;
//     }
//     else if (pattern == R"(\d)"){ // Using raw string literal to avoid escaping the backslash, can also use \\d
//         return input_line.find_first_of("1234567890") != std::string::npos; // Better than looping through all the numbers
//     }
//     else if (pattern == "\\w") {
//         return (input_line.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") != std::string::npos) || (match_pattern(input_line, "\\d"));
//     }
//     else if (pattern.starts_with("[") && pattern.ends_with("]")) {
//         if (pattern.at(1) == '[^') {
//             int length = pattern.length()-3;
//             std::string set = pattern.substr(2, length);
//             for (char c : set) {
//                 if (input_line.find(c) != std::string::npos) {  // For all the chars, none should be in the input line, could also use find_first_not_of
//                     length -= 1;
//                 }
//             }
//             return length!=0; // If length is greater than 0, then at least one char didn't match, return true -> becomes 0
//         }
//         return input_line.find_first_of(pattern.substr(1, pattern.length()-2)) != std::string::npos; // -2 because substr takes (pos, length)
//     }
//     else {
//         throw std::runtime_error("Unhandled pattern " + pattern);
//     }
// }

bool match_string(const std::string &input_line, const std::string &pattern) {
    // Try matching the pattern starting at each position in the input
    for (int start_pos = 0; start_pos <= input_line.length() - pattern.length(); start_pos++) {
        int input_pos = start_pos;
        int pattern_pos = 0;
        bool match_found = true;
        
        // Try to match the entire pattern starting at start_pos
        while (pattern_pos < pattern.length() && input_pos < input_line.length()) {
            if (!match_pattern(input_line, pattern, input_pos, pattern_pos)) {
                match_found = false;
                break;
            }
            input_pos++;
            pattern_pos++;
        }
        
        // If we matched the entire pattern, return true
        if (match_found && pattern_pos == pattern.length()) return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc != 3) {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];

    if (flag != "-E") {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    std::string input_line;
    std::getline(std::cin, input_line); // To get the complete input line with spaces
    
    try {
        if (match_string(input_line, pattern)) {
            return 0;
        } else {
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}