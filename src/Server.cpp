#include <iostream>
#include <string>

bool match_pattern(const std::string& input_line, const std::string& pattern, int& input_pos, int& pattern_pos) {
    // Check bounds for the inputs
    if (input_pos >= input_line.length() || pattern_pos >= pattern.length()) return false;
    
    // Handle escape sequences
    if (pattern.at(pattern_pos) == '\\') {
        pattern_pos++;
        if (pattern_pos >= pattern.length()) return false; // Check bounds after increment
        if(pattern.at(pattern_pos) == 'd') return (std::string(1, input_line.at(input_pos)).find_first_of("1234567890") != std::string::npos);
        else if(pattern.at(pattern_pos) == 'w') return (std::string(1, input_line.at(input_pos)).find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890") != std::string::npos);
    }

    // Handle character groups [abc] and negative groups [^abc]
    else if (pattern.at(pattern_pos) == '[') {
        int bracket_start = pattern_pos;
        pattern_pos++; // Move past '['
        
        // Check for negative group [^...]
        bool is_negative = false;
        if (pattern_pos < pattern.length() && pattern.at(pattern_pos) == '^') is_negative = true;
        
        // Find the closing bracket
        int bracket_end = bracket_start + 1;
        while (bracket_end < pattern.length() && pattern.at(bracket_end) != ']') {
            bracket_end++;
        }
        
        // In case the bracket is not closed, treat it as a literal '['
        if (bracket_end >= pattern.length()) {
            pattern_pos = bracket_start;
            return input_line.at(input_pos) == '[';
        }
        
        // Extract the character set
        int set_start = is_negative ? bracket_start + 2 : bracket_start + 1;
        int set_length = bracket_end - set_start;
        std::string char_set = pattern.substr(set_start, set_length);
        
        // Move pattern_pos past the closing bracket
        pattern_pos = bracket_end;
        
        // Check if input character matches the character set
        char input_char = input_line.at(input_pos);
        bool char_in_set = char_set.find(input_char) != std::string::npos;
        
        if (is_negative) {
            return !char_in_set; // For [^abc], return true if char is NOT in set
        } else {
            return char_in_set;  // For [abc], return true if char is in set
        }
    }

    //
    else if (pattern.at(pattern_pos) == '+') {
        // Handle + in the beginning
        if (pattern_pos == 0) return false;

        // Get the next character
        char next_char = pattern.at(pattern_pos + 1);
        char prev_char = pattern.at(pattern_pos - 1);
        int count = 0;

        // Iterate until the character matches the previous character
        while (input_pos < input_line.length() && (input_line.at(input_pos) == prev_char)) input_pos++;

        // Move back if the next character also matches the previous character
        if (input_line.at(input_pos) == next_char) input_pos--;
        
        input_pos--;
        return true;
    }

    else if (input_pos == 0 && pattern.at(pattern_pos) == '^') pattern_pos++;

    // Handle literal character matching
    return input_line.at(input_pos) == pattern.at(pattern_pos);
}

bool match_string(const std::string &input_line, const std::string &pattern) {
    int pattern_length = pattern.length();
    int input_length = input_line.length();

    // Try matching the pattern starting at each position in the input
    for (int start_pos = 0; start_pos < input_length; start_pos++) {
        int input_pos = start_pos;
        int pattern_pos = 0;
        bool match_found = true;
        
        // Try to match the entire pattern starting at start_pos
        while (pattern_pos < pattern_length && input_pos < input_length) {
            if (!match_pattern(input_line, pattern, input_pos, pattern_pos)) {
                match_found = false;
                break;
            }
            input_pos++;
            pattern_pos++; // I am better than Claude
        }
        // Very important to check the limits of the length (against Out of Bounds errors)
        if (match_found && input_pos == input_length && pattern_pos < pattern_length && pattern.at(pattern_pos) == '$') return true;

        // If we matched the entire pattern, return true
        if (match_found && pattern_pos == pattern_length) return true;
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