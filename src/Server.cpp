#include <iostream>
#include <string>

bool match_group(const std::string& input_line, const std::string& pattern) {
    return input_line.find_first_of(pattern) != std::string::npos;
}

bool match_pattern(const std::string& input_line, const std::string& pattern, bool start_of_line = true) {
    if (pattern.size() == 0) return true;
    
    // Handle end of string anchor
    if (pattern[0] == '$') {
        return input_line.empty();
    }
    
    if (input_line.size() == 0) return false;

    // Handle alternation groups (abc|def)
    if (pattern[0] == '(' && pattern.find('|') != std::string::npos) {
        // Find the matching closing parenthesis
        int paren_count = 0;
        size_t close_pos = 0;
        for (size_t i = 0; i < pattern.size(); i++) {
            if (pattern[i] == '(') paren_count++;
            else if (pattern[i] == ')') {
                paren_count--;
                if (paren_count == 0) {
                    close_pos = i;
                    break;
                }
            }
        }
        
        // Extract the content inside parentheses
        std::string group_content = pattern.substr(1, close_pos - 1);
        std::string remaining_pattern = pattern.substr(close_pos + 1);
        
        // Find the | position within this group
        size_t pipe_pos = group_content.find('|');
        std::string subpattern1 = group_content.substr(0, pipe_pos);
        std::string subpattern2 = group_content.substr(pipe_pos + 1);
        
        // Try both alternatives
        return match_pattern(input_line, subpattern1 + remaining_pattern, start_of_line) || 
               match_pattern(input_line, subpattern2 + remaining_pattern, start_of_line);
    }

    // Handle + quantifier
    if (pattern.size() > 1 && pattern[1] == '+') {
        char target = pattern[0];
        
        // Special handling for \d+
        if (pattern.substr(0, 2) == "\\d") {
            size_t i = 0;
            while (i < input_line.size() && isdigit(input_line[i])) {
                i++;
            }
            if (i > 0) {
                return match_pattern(input_line.substr(i), pattern.substr(3), false);
            }
            return false;
        }
        
        // Special handling for \w+
        if (pattern.substr(0, 2) == "\\w") {
            size_t i = 0;
            while (i < input_line.size() && isalnum(input_line[i])) {
                i++;
            }
            if (i > 0) {
                return match_pattern(input_line.substr(i), pattern.substr(3), false);
            }
            return false;
        }
        
        // Handle character groups like [abc]+
        if (target == '[') {
            auto close_bracket = pattern.find(']');
            if (close_bracket != std::string::npos) {
                std::string char_class = pattern.substr(1, close_bracket - 1);
                bool neg = char_class[0] == '^';
                if (neg) char_class = char_class.substr(1);
                
                size_t i = 0;
                while (i < input_line.size()) {
                    bool matches = (char_class.find(input_line[i]) != std::string::npos);
                    if (neg) matches = !matches;
                    if (!matches) break;
                    i++;
                }
                
                if (i > 0) {
                    return match_pattern(input_line.substr(i), pattern.substr(close_bracket + 2), false);
                }
                return false;
            }
        }
        
        // Regular character +
        size_t i = 0;
        while (i < input_line.size() && input_line[i] == target) {
            i++;
        }
        if (i > 0) {
            return match_pattern(input_line.substr(i), pattern.substr(2), false);
        }
        return false;
    }

    // Handle ? quantifier
    if (pattern.size() > 1 && pattern[1] == '?') {
        // Try matching with the optional character
        if (pattern[0] == input_line[0]) {
            if (match_pattern(input_line.substr(1), pattern.substr(2), false)) {
                return true;
            }
        }
        // Try matching without the optional character
        return match_pattern(input_line, pattern.substr(2), false);
    }

    // Handle . (any character)
    if (pattern[0] == '.') {
        return match_pattern(input_line.substr(1), pattern.substr(1), false);
    }

    // Handle \d (digit)
    if (pattern.substr(0, 2) == "\\d") {
        if (isdigit(input_line[0])) {
            return match_pattern(input_line.substr(1), pattern.substr(2), false);
        }
        if (!start_of_line) {
            return match_pattern(input_line.substr(1), pattern, false);
        }
        return false;
    }

    // Handle \w (alphanumeric)
    if (pattern.substr(0, 2) == "\\w") {
        if (isalnum(input_line[0])) {
            return match_pattern(input_line.substr(1), pattern.substr(2), false);
        }
        if (!start_of_line) {
            return match_pattern(input_line.substr(1), pattern, false);
        }
        return false;
    }

    // Handle character classes [abc] or [^abc]
    if (pattern[0] == '[') {
        auto close_bracket = pattern.find(']');
        if (close_bracket != std::string::npos) {
            std::string char_class = pattern.substr(1, close_bracket - 1);
            bool neg = char_class[0] == '^';
            if (neg) char_class = char_class.substr(1);
            
            bool matches = (char_class.find(input_line[0]) != std::string::npos);
            if (neg) matches = !matches;
            
            if (matches) {
                return match_pattern(input_line.substr(1), pattern.substr(close_bracket + 1), false);
            }
            if (!start_of_line) {
                return match_pattern(input_line.substr(1), pattern, false);
            }
            return false;
        }
    }

    // Handle literal character match
    if (pattern[0] == input_line[0]) {
        return match_pattern(input_line.substr(1), pattern.substr(1), false);
    }
    
    // If we're not at start of line, try advancing the input
    if (!start_of_line) {
        return match_pattern(input_line.substr(1), pattern, false);
    }
    
    return false;
}

bool match_patterns(const std::string& input_line, const std::string& pattern) {
    if (pattern[0] == '^') {
        return match_pattern(input_line, pattern.substr(1), true);
    }
    
    return match_pattern(input_line, pattern, false);
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

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
    std::getline(std::cin, input_line);
    
    try {
        bool result = match_patterns(input_line, pattern);
        return result ? 0 : 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}