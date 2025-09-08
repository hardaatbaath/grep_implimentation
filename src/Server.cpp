#include <iostream>
#include <string>

bool match_pattern(const std::string& input_line, const std::string& pattern, int& input_pos, int& pattern_pos) {
    // Check bounds for the inputs
    if (input_pos >= input_line.length() || pattern_pos >= pattern.length()) return false;

    else if ((pattern_pos + 1 < pattern.length()) && pattern.at(pattern_pos + 1) == '?'){
        char pattern_match = pattern.at(pattern_pos);
        pattern_pos++;

        // If it matches, nothing changes. If it doesn't, just move the input pointer back
        if (pattern_match != input_line.at(input_pos))input_pos--;
        return true;
    }
    
    // Handle escape sequences
    else if (pattern.at(pattern_pos) == '\\') {
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

    // Handle + quantifier (one or more)
    else if (pattern.at(pattern_pos) == '+') {
        if (pattern_pos == 0) return false; // + cannot be at the beginning
        
        // Get the character to be repeated
        char repeat_char = pattern.at(pattern_pos - 1);
        
        // If + is at end of pattern, just return true
        if (pattern_pos + 1 >= pattern.length()) {
            input_pos--;
            return true;
        }
        
        // Get the next character
        char next_char = pattern.at(pattern_pos + 1);
        
        // Consume characters based on repeat_char type
        if (repeat_char == '.') {
            // For .+ consume any characters until we find next_char
            while (input_pos < input_line.length() && input_line.at(input_pos) != next_char) {
                input_pos++;
            }
        } else {
            // Consume all occurrences of repeat_char in input_line
            while (input_pos < input_line.length() && input_line.at(input_pos) == repeat_char) input_pos++;
        }

        // Consume all occurrences of repeat_char in pattern
        while(pattern_pos < pattern.length() && pattern.at(pattern_pos + 1) == repeat_char) pattern_pos++;
        
        // // Backtrack if needed to allow next_char to match (this logic also works, but me too OG for this)
        // if (input_pos < input_line.length() && input_line.at(input_pos) != next_char) {
        //     while (input_pos > 0 && input_line.at(input_pos) != next_char && 
        //            input_line.at(input_pos - 1) == repeat_char) {
        //         input_pos--;
        //     }
        // }
        
        input_pos--;
        return true;
    }

    // Handle alternation
    else if (pattern.at(pattern_pos) == '(') {
        int bracket_start = pattern_pos;
        
        // Find the closing ')'
        int bracket_end = bracket_start + 1;
        while(bracket_end < pattern.length() && pattern.at(bracket_end) != ')') bracket_end++;
        
        // Extract all content between ( and )
        std::string alternatives = pattern.substr(bracket_start + 1, bracket_end - bracket_start - 1);
        
        // Split by '|' and try each alternative
        int alt_start = 0;
        for (int i = 0; i <= alternatives.length(); i++) {
            if (i == alternatives.length() || alternatives.at(i) == '|') {
                // Extract current alternative
                std::string current_alt = alternatives.substr(alt_start, i - alt_start);
                
                // Try matching this alternative
                int temp_input_pos = input_pos;
                int temp_pattern_pos = 0;
                bool alt_matches = true;
                
                for (int j = 0; j < current_alt.length() && temp_input_pos < input_line.length(); j++) {
                    if (!match_pattern(input_line, current_alt, temp_input_pos, temp_pattern_pos)) {
                        alt_matches = false;
                        break;
                    }
                    temp_input_pos++;
                    temp_pattern_pos++;
                }
                
                if (alt_matches && temp_pattern_pos == current_alt.length()) {
                    input_pos = temp_input_pos - 1; // Will be incremented in main loop
                    pattern_pos = bracket_end; // Skip past closing )
                    return true;
                }
                
                alt_start = i + 1; // Move to next alternative
            }
        }
        
        return false;
    }

    // Handle wildcard characters
    else if (pattern.at(pattern_pos) == '.') return true; // '.' matches any character

    // Handle beginning of the string ^
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