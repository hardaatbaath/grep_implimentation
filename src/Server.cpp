#include <iostream>
#include <string>

// Recursive pattern matcher
bool match_pattern(const std::string& input_line, const std::string& pattern, int input_pos, int pattern_pos) {
    // Base case: pattern fully consumed
    if (pattern_pos >= pattern.length()) {
        return true;
    }
    
    // Input exhausted but pattern remains
    if (input_pos >= input_line.length()) {
        // Check if remaining pattern can match empty string (only $ or quantifiers with 0 matches)
        if (pattern_pos < pattern.length() && pattern.at(pattern_pos) == '$') {
            return pattern_pos + 1 == pattern.length();
        }
        return false;
    }

    // Handle optional quantifier (?)
    if ((pattern_pos + 1 < pattern.length()) && pattern.at(pattern_pos + 1) == '?') {
        char pattern_char = pattern.at(pattern_pos);
        
        // Try matching without the optional character first
        if (match_pattern(input_line, pattern, input_pos, pattern_pos + 2)) {
            return true;
        }
        
        // Try matching with the optional character
        if (match_single_char(input_line.at(input_pos), pattern_char, pattern, pattern_pos)) {
            return match_pattern(input_line, pattern, input_pos + 1, pattern_pos + 2);
        }
        
        return false;
    }
    
    // Handle one-or-more quantifier (+)
    if ((pattern_pos + 1 < pattern.length()) && pattern.at(pattern_pos + 1) == '+') {
        char pattern_char = pattern.at(pattern_pos);
        
        // Must match at least once
        if (!match_single_char(input_line.at(input_pos), pattern_char, pattern, pattern_pos)) {
            return false;
        }
        
        // Count consecutive matches
        int match_count = 0;
        int temp_pos = input_pos;
        while (temp_pos < input_line.length() && 
               match_single_char(input_line.at(temp_pos), pattern_char, pattern, pattern_pos)) {
            match_count++;
            temp_pos++;
        }
        
        // Try from longest match down to 1 (greedy backtracking)
        for (int try_matches = match_count; try_matches >= 1; try_matches--) {
            if (match_pattern(input_line, pattern, input_pos + try_matches, pattern_pos + 2)) {
                return true;
            }
        }
        return false;
    }
    
    // Handle zero-or-more quantifier (*)
    if ((pattern_pos + 1 < pattern.length()) && pattern.at(pattern_pos + 1) == '*') {
        char pattern_char = pattern.at(pattern_pos);
        
        // Count consecutive matches
        int match_count = 0;
        int temp_pos = input_pos;
        while (temp_pos < input_line.length() && 
               match_single_char(input_line.at(temp_pos), pattern_char, pattern, pattern_pos)) {
            match_count++;
            temp_pos++;
        }
        
        // Try from longest match down to 0 (greedy backtracking)
        for (int try_matches = match_count; try_matches >= 0; try_matches--) {
            if (match_pattern(input_line, pattern, input_pos + try_matches, pattern_pos + 2)) {
                return true;
            }
        }
        return false;
    }

    // Handle alternation groups (cat|dog|cow)
    if (pattern.at(pattern_pos) == '(') {
        return match_group(input_line, pattern, input_pos, pattern_pos);
    }

    // Handle beginning anchor ^
    if (pattern.at(pattern_pos) == '^') {
        if (input_pos != 0) return false;
        return match_pattern(input_line, pattern, input_pos, pattern_pos + 1);
    }
    
    // Handle end anchor $
    if (pattern.at(pattern_pos) == '$') {
        return input_pos == input_line.length();
    }

    // Handle single character match
    if (match_single_char(input_line.at(input_pos), pattern.at(pattern_pos), pattern, pattern_pos)) {
        int next_pattern_pos = pattern_pos + 1;
        
        // Skip past escape sequences
        if (pattern.at(pattern_pos) == '\\' && pattern_pos + 1 < pattern.length()) {
            next_pattern_pos = pattern_pos + 2;
        }
        // Skip past character classes
        else if (pattern.at(pattern_pos) == '[') {
            next_pattern_pos = find_closing_bracket(pattern, pattern_pos) + 1;
        }
        
        return match_pattern(input_line, pattern, input_pos + 1, next_pattern_pos);
    }
    
    return false;
}

// Helper function to match alternation groups recursively
bool match_group(const std::string& input_line, const std::string& pattern, int input_pos, int pattern_pos) {
    int bracket_start = pattern_pos;
    int bracket_end = find_closing_paren(pattern, bracket_start);
    
    if (bracket_end == -1) return false; // Unmatched parenthesis
    
    // Extract group content
    std::string group_content = pattern.substr(bracket_start + 1, bracket_end - bracket_start - 1);
    
    // Split alternatives by '|' and try each recursively
    std::vector<std::string> alternatives = split_alternatives(group_content);
    
    for (const std::string& alt : alternatives) {
        // Try to match this alternative recursively
        if (match_pattern(input_line, alt, input_pos, 0)) {
            // Calculate how many characters this alternative consumed
            int consumed = calculate_match_length(input_line, alt, input_pos);
            
            // Continue matching the rest of the pattern after the group
            if (match_pattern(input_line, pattern, input_pos + consumed, bracket_end + 1)) {
                return true;
            }
        }
    }
    
    return false;
}

// Helper function to match a single character with all special cases
bool match_single_char(char input_char, char pattern_char, const std::string& pattern, int pattern_pos) {
    // Handle escape sequences
    if (pattern_char == '\\') {
        if (pattern_pos + 1 >= pattern.length()) return false;
        char escaped = pattern.at(pattern_pos + 1);
        
        if (escaped == 'd') {
            return (input_char >= '0' && input_char <= '9');
        } else if (escaped == 'w') {
            return ((input_char >= 'a' && input_char <= 'z') || 
                   (input_char >= 'A' && input_char <= 'Z') || 
                   (input_char >= '0' && input_char <= '9') || 
                   input_char == '_');
        } else if (escaped == 's') {
            return (input_char == ' ' || input_char == '\t' || 
                   input_char == '\n' || input_char == '\r');
        } else {
            return input_char == escaped;
        }
    }

    // Handle character groups [abc] and negative groups [^abc]
    if (pattern_char == '[') {
        return match_character_class(input_char, pattern, pattern_pos);
    }

    // Handle wildcard
    if (pattern_char == '.') {
        return true;
    }

    // Literal character match
    return input_char == pattern_char;
}

// Helper function to match character classes
bool match_character_class(char input_char, const std::string& pattern, int start_pos) {
    int pos = start_pos + 1; // Skip '['
    
    // Check for negation
    bool is_negative = false;
    if (pos < pattern.length() && pattern.at(pos) == '^') {
        is_negative = true;
        pos++;
    }
    
    // Find closing bracket
    int end_pos = find_closing_bracket(pattern, start_pos);
    if (end_pos == -1) {
        return input_char == '['; // Treat as literal if malformed
    }
    
    // Check if character is in the set
    bool char_in_set = false;
    while (pos < end_pos) {
        if (pos + 2 < end_pos && pattern.at(pos + 1) == '-') {
            // Range like a-z
            if (input_char >= pattern.at(pos) && input_char <= pattern.at(pos + 2)) {
                char_in_set = true;
                break;
            }
            pos += 3;
        } else {
            // Single character
            if (input_char == pattern.at(pos)) {
                char_in_set = true;
                break;
            }
            pos++;
        }
    }
    
    return is_negative ? !char_in_set : char_in_set;
}

// Helper functions for parsing
int find_closing_paren(const std::string& pattern, int start_pos) {
    int depth = 1;
    for (int i = start_pos + 1; i < pattern.length(); i++) {
        if (pattern.at(i) == '(') depth++;
        else if (pattern.at(i) == ')') {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

int find_closing_bracket(const std::string& pattern, int start_pos) {
    for (int i = start_pos + 1; i < pattern.length(); i++) {
        if (pattern.at(i) == ']') return i;
    }
    return -1;
}

std::vector<std::string> split_alternatives(const std::string& group) {
    std::vector<std::string> alternatives;
    std::string current;
    int depth = 0;
    
    for (char c : group) {
        if (c == '(') {
            depth++;
        } else if (c == ')') {
            depth--;
        } else if (c == '|' && depth == 0) {
            alternatives.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    alternatives.push_back(current);
    return alternatives;
}

int calculate_match_length(const std::string& input_line, const std::string& pattern, int start_pos) {
    int input_pos = start_pos;
    int pattern_pos = 0;
    
    // Simple forward matching to calculate length
    while (pattern_pos < pattern.length() && input_pos < input_line.length()) {
        if (match_single_char(input_line.at(input_pos), pattern.at(pattern_pos), pattern, pattern_pos)) {
            input_pos++;
            if (pattern.at(pattern_pos) == '\\') pattern_pos += 2;
            else if (pattern.at(pattern_pos) == '[') {
                pattern_pos = find_closing_bracket(pattern, pattern_pos) + 1;
            } else {
                pattern_pos++;
            }
        } else {
            break;
        }
    }
    
    return input_pos - start_pos;
}

bool match_string(const std::string &input_line, const std::string &pattern) {
    // Handle anchored patterns specially
    if (!pattern.empty() && pattern.at(0) == '^') {
        // Must start from beginning
        return match_pattern(input_line, pattern, 0, 0);
    }
    
    // Try matching from each position
    for (int start_pos = 0; start_pos <= input_line.length(); start_pos++) {
        if (match_pattern(input_line, pattern, start_pos, 0)) {
            return true;
        }
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

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
    std::getline(std::cin, input_line);
    
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