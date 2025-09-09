#include <iostream>
#include <string>
#include <vector>

// Forward declarations
bool match_pattern(const std::string& input, const std::string& pattern, int input_pos, int pattern_pos);

// Helper to match single character including escape sequences
bool match_char(char input_char, const std::string& pattern, int& pattern_pos) {
    if (pattern_pos >= pattern.length()) return false;
    
    char pattern_char = pattern[pattern_pos];
    
    if (pattern_char == '\\') {
        if (pattern_pos + 1 >= pattern.length()) return false;
        pattern_pos++; // Move to escaped character
        char escaped = pattern[pattern_pos];
        
        switch (escaped) {
            case 'd': return (input_char >= '0' && input_char <= '9');
            case 'w': return ((input_char >= 'a' && input_char <= 'z') || 
                             (input_char >= 'A' && input_char <= 'Z') || 
                             (input_char >= '0' && input_char <= '9') || 
                             input_char == '_');
            case 's': return (input_char == ' ' || input_char == '\t' || 
                             input_char == '\n' || input_char == '\r');
            default: return input_char == escaped;
        }
    }
    
    if (pattern_char == '.') return true;
    
    // Handle character classes [abc]
    if (pattern_char == '[') {
        int start = pattern_pos + 1;
        bool negated = false;
        
        if (start < pattern.length() && pattern[start] == '^') {
            negated = true;
            start++;
        }
        
        // Find closing bracket
        int end = start;
        while (end < pattern.length() && pattern[end] != ']') end++;
        if (end >= pattern.length()) return input_char == '['; // Malformed
        
        bool found = false;
        for (int i = start; i < end; i++) {
            if (i + 2 < end && pattern[i + 1] == '-') {
                // Range
                if (input_char >= pattern[i] && input_char <= pattern[i + 2]) {
                    found = true;
                    break;
                }
                i += 2;
            } else {
                if (input_char == pattern[i]) {
                    found = true;
                    break;
                }
            }
        }
        
        pattern_pos = end; // Move to closing bracket
        return negated ? !found : found;
    }
    
    return input_char == pattern_char;
}

// Find matching closing parenthesis
int find_closing_paren(const std::string& pattern, int start) {
    int depth = 1;
    for (int i = start + 1; i < pattern.length(); i++) {
        if (pattern[i] == '(') depth++;
        else if (pattern[i] == ')') {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

// Match group and return consumed characters
int match_group(const std::string& input, const std::string& pattern, int input_pos, int group_start, int group_end) {
    std::string group_content = pattern.substr(group_start + 1, group_end - group_start - 1);
    
    // Split alternatives by |
    std::vector<std::string> alternatives;
    std::string current;
    int depth = 0;
    
    for (char c : group_content) {
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (c == '|' && depth == 0) {
            alternatives.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    alternatives.push_back(current);
    
    // Try each alternative
    for (const std::string& alt : alternatives) {
        if (match_pattern(input, alt, input_pos, 0)) {
            // Calculate how much this alternative consumed
            int temp_input = input_pos;
            int temp_pattern = 0;
            
            while (temp_pattern < alt.length() && temp_input < input.length()) {
                if (alt[temp_pattern] == '\\') {
                    int saved_pattern = temp_pattern;
                    if (match_char(input[temp_input], alt, temp_pattern)) {
                        temp_input++;
                        temp_pattern++;
                    } else {
                        break;
                    }
                } else if (alt[temp_pattern] == '[') {
                    int saved_pattern = temp_pattern;
                    if (match_char(input[temp_input], alt, temp_pattern)) {
                        temp_input++;
                        temp_pattern++;
                    } else {
                        break;
                    }
                } else if (alt[temp_pattern] == '(') {
                    int sub_group_end = find_closing_paren(alt, temp_pattern);
                    if (sub_group_end == -1) break;
                    
                    int consumed = match_group(input, alt, temp_input, temp_pattern, sub_group_end);
                    if (consumed > 0) {
                        temp_input += consumed;
                        temp_pattern = sub_group_end + 1;
                        
                        // Handle quantifiers on subgroup
                        if (temp_pattern < alt.length() && 
                            (alt[temp_pattern] == '?' || alt[temp_pattern] == '+' || alt[temp_pattern] == '*')) {
                            temp_pattern++;
                        }
                    } else {
                        break;
                    }
                } else if (temp_pattern + 1 < alt.length() && alt[temp_pattern + 1] == '?') {
                    // Optional character
                    if (match_char(input[temp_input], alt, temp_pattern)) {
                        temp_input++;
                    }
                    temp_pattern += 2;
                } else {
                    // Regular character
                    if (match_char(input[temp_input], alt, temp_pattern)) {
                        temp_input++;
                        temp_pattern++;
                    } else {
                        break;
                    }
                }
            }
            
            if (temp_pattern == alt.length()) {
                return temp_input - input_pos;
            }
        }
    }
    
    return 0; // No match
}

// Main recursive pattern matcher
bool match_pattern(const std::string& input, const std::string& pattern, int input_pos, int pattern_pos) {
    while (pattern_pos < pattern.length()) {
        if (input_pos > input.length()) return false;
        
        // Handle anchors
        if (pattern[pattern_pos] == '^') {
            if (input_pos != 0) return false;
            pattern_pos++;
            continue;
        }
        
        if (pattern[pattern_pos] == '$') {
            return input_pos == input.length();
        }
        
        // Handle groups
        if (pattern[pattern_pos] == '(') {
            int group_end = find_closing_paren(pattern, pattern_pos);
            if (group_end == -1) return false;
            
            // Check for quantifiers after group
            char quantifier = '\0';
            if (group_end + 1 < pattern.length()) {
                char next = pattern[group_end + 1];
                if (next == '+' || next == '*' || next == '?') {
                    quantifier = next;
                }
            }
            
            if (quantifier == '+') {
                // One or more: must match at least once
                int consumed = match_group(input, pattern, input_pos, pattern_pos, group_end);
                if (consumed == 0) return false;
                
                input_pos += consumed;
                
                // Match as many more as possible
                while (input_pos < input.length()) {
                    int next_consumed = match_group(input, pattern, input_pos, pattern_pos, group_end);
                    if (next_consumed > 0) {
                        input_pos += next_consumed;
                    } else {
                        break;
                    }
                }
                
                // Try to match rest of pattern (with backtracking)
                while (true) {
                    if (match_pattern(input, pattern, input_pos, group_end + 2)) {
                        return true;
                    }
                    
                    // Backtrack by one group match if possible
                    if (input_pos > 0) {
                        // Find previous valid group boundary
                        bool found_prev = false;
                        for (int back_pos = input_pos - 1; back_pos >= 0; back_pos--) {
                            int test_consumed = match_group(input, pattern, back_pos, pattern_pos, group_end);
                            if (test_consumed > 0 && back_pos + test_consumed == input_pos) {
                                input_pos = back_pos;
                                found_prev = true;
                                break;
                            }
                        }
                        if (!found_prev) break;
                    } else {
                        break;
                    }
                }
                
                return false;
                
            } else if (quantifier == '*') {
                // Zero or more
                // First, match as many as possible
                while (input_pos < input.length()) {
                    int consumed = match_group(input, pattern, input_pos, pattern_pos, group_end);
                    if (consumed > 0) {
                        input_pos += consumed;
                    } else {
                        break;
                    }
                }
                
                // Try to match rest with backtracking
                while (true) {
                    if (match_pattern(input, pattern, input_pos, group_end + 2)) {
                        return true;
                    }
                    if (input_pos > 0) {
                        input_pos--;
                    } else {
                        break;
                    }
                }
                return false;
                
            } else if (quantifier == '?') {
                // Optional: try with group first, then without
                int consumed = match_group(input, pattern, input_pos, pattern_pos, group_end);
                if (consumed > 0 && match_pattern(input, pattern, input_pos + consumed, group_end + 2)) {
                    return true;
                }
                // Try without group
                return match_pattern(input, pattern, input_pos, group_end + 2);
                
            } else {
                // No quantifier: match once
                int consumed = match_group(input, pattern, input_pos, pattern_pos, group_end);
                if (consumed == 0) return false;
                input_pos += consumed;
                pattern_pos = group_end + 1;
                continue;
            }
        }
        
        // Handle single character quantifiers
        if (pattern_pos + 1 < pattern.length()) {
            char quantifier = pattern[pattern_pos + 1];
            
            if (quantifier == '?') {
                // Optional: try without first, then with
                if (match_pattern(input, pattern, input_pos, pattern_pos + 2)) {
                    return true;
                }
                if (input_pos < input.length()) {
                    int saved_pattern = pattern_pos;
                    if (match_char(input[input_pos], pattern, saved_pattern)) {
                        return match_pattern(input, pattern, input_pos + 1, pattern_pos + 2);
                    }
                }
                return false;
                
            } else if (quantifier == '+') {
                // One or more
                if (input_pos >= input.length()) return false;
                
                int saved_pattern = pattern_pos;
                if (!match_char(input[input_pos], pattern, saved_pattern)) return false;
                
                // Match as many as possible
                int match_count = 1;
                int temp_pos = input_pos + 1;
                while (temp_pos < input.length()) {
                    int test_pattern = pattern_pos;
                    if (match_char(input[temp_pos], pattern, test_pattern)) {
                        match_count++;
                        temp_pos++;
                    } else {
                        break;
                    }
                }
                
                // Try from longest to shortest
                for (int try_count = match_count; try_count >= 1; try_count--) {
                    if (match_pattern(input, pattern, input_pos + try_count, pattern_pos + 2)) {
                        return true;
                    }
                }
                return false;
                
            } else if (quantifier == '*') {
                // Zero or more
                int match_count = 0;
                int temp_pos = input_pos;
                while (temp_pos < input.length()) {
                    int test_pattern = pattern_pos;
                    if (match_char(input[temp_pos], pattern, test_pattern)) {
                        match_count++;
                        temp_pos++;
                    } else {
                        break;
                    }
                }
                
                // Try from longest to zero
                for (int try_count = match_count; try_count >= 0; try_count--) {
                    if (match_pattern(input, pattern, input_pos + try_count, pattern_pos + 2)) {
                        return true;
                    }
                }
                return false;
            }
        }
        
        // Regular character match
        if (input_pos >= input.length()) return false;
        
        int saved_pattern = pattern_pos;
        if (!match_char(input[input_pos], pattern, saved_pattern)) return false;
        
        input_pos++;
        pattern_pos = saved_pattern + 1;
    }
    
    return true; // Pattern fully consumed
}

bool match_string(const std::string& input_line, const std::string& pattern) {
    // Special case: if pattern starts with ^, only try from position 0
    if (!pattern.empty() && pattern[0] == '^') {
        return match_pattern(input_line, pattern, 0, 0);
    }
    
    // Try matching from each position
    for (int start = 0; start <= input_line.length(); start++) {
        if (match_pattern(input_line, pattern, start, 0)) {
            return true;
        }
    }
    
    return false;
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
    
    if (match_string(input_line, pattern)) {
        return 0;
    } else {
        return 1;
    }
}