#include <iostream>
#include <string>

struct MatchResult {
    bool success;
    size_t pos;
    
    MatchResult(bool s = false, size_t p = 0) : success(s), pos(p) {}
};

// Forward declarations
MatchResult match_pattern(const std::string& text, size_t start_pos, const std::string& pattern);
MatchResult match_group(const std::string& text, size_t start_pos, const std::string& pattern, size_t& pattern_pos);

MatchResult match_alternation(const std::string& text, size_t start_pos, const std::string& alt_pattern) {
    // Find pipe position, accounting for nested groups
    int depth = 0;
    size_t pipe_pos = std::string::npos;
    
    for (size_t i = 0; i < alt_pattern.size(); i++) {
        if (alt_pattern[i] == '(') depth++;
        else if (alt_pattern[i] == ')') depth--;
        else if (alt_pattern[i] == '|' && depth == 0) {
            pipe_pos = i;
            break;
        }
    }
    
    if (pipe_pos == std::string::npos) {
        // No alternation, just match directly
        return match_pattern(text, start_pos, alt_pattern);
    }
    
    // Try left side
    std::string left = alt_pattern.substr(0, pipe_pos);
    MatchResult left_result = match_pattern(text, start_pos, left);
    if (left_result.success) {
        return left_result;
    }
    
    // Try right side
    std::string right = alt_pattern.substr(pipe_pos + 1);
    return match_pattern(text, start_pos, right);
}

MatchResult match_atom(const std::string& text, size_t start_pos, const std::string& pattern, size_t& pattern_pos) {
    if (pattern_pos >= pattern.size()) {
        return MatchResult(true, start_pos);
    }
    
    char ch = pattern[pattern_pos];
    
    if (ch == '(') {
        return match_group(text, start_pos, pattern, pattern_pos);
    }
    else if (ch == '\\' && pattern_pos + 1 < pattern.size()) {
        char next = pattern[pattern_pos + 1];
        pattern_pos += 2;
        
        if (start_pos >= text.size()) {
            return MatchResult(false, start_pos);
        }
        
        bool matches = false;
        if (next == 'd') {
            matches = isdigit(text[start_pos]);
        } else if (next == 'w') {
            matches = isalnum(text[start_pos]) || text[start_pos] == '_';
        } else {
            matches = (text[start_pos] == next);
        }
        
        if (matches) {
            return MatchResult(true, start_pos + 1);
        }
        return MatchResult(false, start_pos);
    }
    else if (ch == '[') {
        size_t close_pos = pattern.find(']', pattern_pos + 1);
        if (close_pos == std::string::npos) {
            return MatchResult(false, start_pos);
        }
        
        std::string char_class = pattern.substr(pattern_pos + 1, close_pos - pattern_pos - 1);
        pattern_pos = close_pos + 1;
        
        if (start_pos >= text.size()) {
            return MatchResult(false, start_pos);
        }
        
        bool negated = false;
        if (!char_class.empty() && char_class[0] == '^') {
            negated = true;
            char_class = char_class.substr(1);
        }
        
        bool found = char_class.find(text[start_pos]) != std::string::npos;
        if (negated) found = !found;
        
        if (found) {
            return MatchResult(true, start_pos + 1);
        }
        return MatchResult(false, start_pos);
    }
    else if (ch == '.') {
        pattern_pos++;
        if (start_pos >= text.size()) {
            return MatchResult(false, start_pos);
        }
        return MatchResult(true, start_pos + 1);
    }
    else {
        // Literal character
        pattern_pos++;
        if (start_pos >= text.size() || text[start_pos] != ch) {
            return MatchResult(false, start_pos);
        }
        return MatchResult(true, start_pos + 1);
    }
}

MatchResult match_group(const std::string& text, size_t start_pos, const std::string& pattern, size_t& pattern_pos) {
    if (pattern_pos >= pattern.size() || pattern[pattern_pos] != '(') {
        return MatchResult(false, start_pos);
    }
    
    // Find matching closing paren
    int depth = 1;
    size_t close_pos = pattern_pos + 1;
    while (close_pos < pattern.size() && depth > 0) {
        if (pattern[close_pos] == '(') depth++;
        else if (pattern[close_pos] == ')') depth--;
        close_pos++;
    }
    
    if (depth > 0) {
        return MatchResult(false, start_pos);
    }
    
    std::string group_content = pattern.substr(pattern_pos + 1, close_pos - pattern_pos - 2);
    pattern_pos = close_pos;
    
    // Check for quantifiers
    if (pattern_pos < pattern.size()) {
        char quantifier = pattern[pattern_pos];
        
        if (quantifier == '+') {
            pattern_pos++;
            
            // Must match at least once
            MatchResult first = match_alternation(text, start_pos, group_content);
            if (!first.success) {
                return MatchResult(false, start_pos);
            }
            
            size_t current_pos = first.pos;
            
            // Keep matching while possible
            while (current_pos < text.size()) {
                MatchResult next = match_alternation(text, current_pos, group_content);
                if (!next.success || next.pos == current_pos) {
                    break;
                }
                current_pos = next.pos;
            }
            
            return MatchResult(true, current_pos);
        }
        else if (quantifier == '?') {
            pattern_pos++;
            
            // Try to match once
            MatchResult attempt = match_alternation(text, start_pos, group_content);
            if (attempt.success) {
                return attempt;
            }
            
            // Zero matches is also valid
            return MatchResult(true, start_pos);
        }
        else if (quantifier == '*') {
            pattern_pos++;
            
            size_t current_pos = start_pos;
            
            // Keep matching while possible
            while (current_pos < text.size()) {
                MatchResult next = match_alternation(text, current_pos, group_content);
                if (!next.success || next.pos == current_pos) {
                    break;
                }
                current_pos = next.pos;
            }
            
            return MatchResult(true, current_pos);
        }
    }
    
    // No quantifier, match exactly once
    return match_alternation(text, start_pos, group_content);
}

MatchResult match_pattern(const std::string& text, size_t start_pos, const std::string& pattern) {
    size_t pattern_pos = 0;
    size_t current_pos = start_pos;
    
    while (pattern_pos < pattern.size()) {
        char ch = pattern[pattern_pos];
        
        if (ch == '$') {
            // End anchor
            return MatchResult(current_pos == text.size(), current_pos);
        }
        
        MatchResult atom_result = match_atom(text, current_pos, pattern, pattern_pos);
        if (!atom_result.success) {
            return MatchResult(false, current_pos);
        }
        
        current_pos = atom_result.pos;
        
        // Handle quantifiers for atoms
        if (pattern_pos < pattern.size()) {
            char quantifier = pattern[pattern_pos];
            
            if (quantifier == '+') {
                pattern_pos++;
                // We already matched once, keep going
                size_t backup_pattern_pos = pattern_pos - 1;
                while (current_pos < text.size()) {
                    size_t temp_pattern_pos = backup_pattern_pos - 1;
                    MatchResult next = match_atom(text, current_pos, pattern, temp_pattern_pos);
                    if (!next.success) {
                        break;
                    }
                    current_pos = next.pos;
                }
            }
            else if (quantifier == '?') {
                pattern_pos++;
                // Already matched once, which satisfies ?
            }
            else if (quantifier == '*') {
                pattern_pos++;
                // We matched once, keep going
                size_t backup_pattern_pos = pattern_pos - 1;
                while (current_pos < text.size()) {
                    size_t temp_pattern_pos = backup_pattern_pos - 1;
                    MatchResult next = match_atom(text, current_pos, pattern, temp_pattern_pos);
                    if (!next.success) {
                        break;
                    }
                    current_pos = next.pos;
                }
            }
        }
    }
    
    return MatchResult(true, current_pos);
}

bool match_patterns(const std::string& input, const std::string& pattern) {
    if (pattern.empty()) {
        return input.empty();
    }
    
    if (pattern[0] == '^') {
        // Anchored at start
        std::string rest_pattern = pattern.substr(1);
        MatchResult result = match_pattern(input, 0, rest_pattern);
        return result.success;
    }
    
    // Try matching at each position
    for (size_t i = 0; i <= input.size(); i++) {
        MatchResult result = match_pattern(input, i, pattern);
        if (result.success) {
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
    
    try {
        bool result = match_patterns(input_line, pattern);
        return result ? 0 : 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}