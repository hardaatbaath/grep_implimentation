#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct MatchResult {
    bool matched;
    size_t consumed;
    
    MatchResult(bool m, size_t c) : matched(m), consumed(c) {}
};

bool is_word_char(char c) {
    return isalnum(c) || c == '_';
}

// Forward declaration
MatchResult match_pattern(const std::string& input, size_t pos, const std::string& pattern, size_t ppos);

MatchResult match_alternation(const std::string& input, size_t pos, const std::string& group_content) {
    // Find the | position, being careful about nested groups
    int paren_depth = 0;
    size_t pipe_pos = std::string::npos;
    
    for (size_t i = 0; i < group_content.size(); i++) {
        if (group_content[i] == '(') paren_depth++;
        else if (group_content[i] == ')') paren_depth--;
        else if (group_content[i] == '|' && paren_depth == 0) {
            pipe_pos = i;
            break;
        }
    }
    
    if (pipe_pos == std::string::npos) {
        // No alternation, just match the group content
        return match_pattern(input, pos, group_content, 0);
    }
    
    std::string left = group_content.substr(0, pipe_pos);
    std::string right = group_content.substr(pipe_pos + 1);
    
    // Try left alternative
    MatchResult left_result = match_pattern(input, pos, left, 0);
    if (left_result.matched) {
        return left_result;
    }
    
    // Try right alternative
    return match_pattern(input, pos, right, 0);
}

MatchResult match_group(const std::string& input, size_t pos, const std::string& pattern, size_t& ppos) {
    if (ppos >= pattern.size() || pattern[ppos] != '(') {
        return MatchResult(false, 0);
    }
    
    // Find matching closing parenthesis
    int paren_count = 1;
    size_t close_pos = ppos + 1;
    while (close_pos < pattern.size() && paren_count > 0) {
        if (pattern[close_pos] == '(') paren_count++;
        else if (pattern[close_pos] == ')') paren_count--;
        close_pos++;
    }
    
    if (paren_count > 0) {
        return MatchResult(false, 0); // Unmatched parenthesis
    }
    
    std::string group_content = pattern.substr(ppos + 1, close_pos - ppos - 2);
    ppos = close_pos; // Move past the closing parenthesis
    
    // Handle quantifiers after the group
    if (ppos < pattern.size()) {
        if (pattern[ppos] == '+') {
            ppos++; // Consume the +
            // Match one or more times
            size_t total_consumed = 0;
            MatchResult first_match = match_alternation(input, pos, group_content);
            if (!first_match.matched) {
                return MatchResult(false, 0);
            }
            
            total_consumed += first_match.consumed;
            
            // Keep matching while possible
            while (pos + total_consumed < input.size()) {
                MatchResult next_match = match_alternation(input, pos + total_consumed, group_content);
                if (!next_match.matched) break;
                total_consumed += next_match.consumed;
            }
            
            return MatchResult(true, total_consumed);
        }
        else if (pattern[ppos] == '?') {
            ppos++; // Consume the ?
            // Match zero or one times
            MatchResult match_result = match_alternation(input, pos, group_content);
            if (match_result.matched) {
                return match_result;
            }
            return MatchResult(true, 0); // Zero matches is also valid
        }
        else if (pattern[ppos] == '*') {
            ppos++; // Consume the *
            // Match zero or more times
            size_t total_consumed = 0;
            while (pos + total_consumed < input.size()) {
                MatchResult match_result = match_alternation(input, pos + total_consumed, group_content);
                if (!match_result.matched) break;
                total_consumed += match_result.consumed;
            }
            return MatchResult(true, total_consumed);
        }
    }
    
    // No quantifier, match exactly once
    return match_alternation(input, pos, group_content);
}

MatchResult match_char_class(const std::string& input, size_t pos, const std::string& pattern, size_t& ppos) {
    if (ppos >= pattern.size() || pattern[ppos] != '[') {
        return MatchResult(false, 0);
    }
    
    size_t close_pos = pattern.find(']', ppos + 1);
    if (close_pos == std::string::npos) {
        return MatchResult(false, 0);
    }
    
    std::string char_class = pattern.substr(ppos + 1, close_pos - ppos - 1);
    ppos = close_pos + 1; // Move past the ]
    
    bool negated = false;
    if (!char_class.empty() && char_class[0] == '^') {
        negated = true;
        char_class = char_class.substr(1);
    }
    
    if (pos >= input.size()) {
        return MatchResult(false, 0);
    }
    
    char ch = input[pos];
    bool found = char_class.find(ch) != std::string::npos;
    if (negated) found = !found;
    
    if (!found) {
        return MatchResult(false, 0);
    }
    
    // Handle quantifiers
    if (ppos < pattern.size()) {
        if (pattern[ppos] == '+') {
            ppos++; // Consume the +
            size_t consumed = 1; // Already matched one
            while (pos + consumed < input.size()) {
                char next_ch = input[pos + consumed];
                bool next_found = char_class.find(next_ch) != std::string::npos;
                if (negated) next_found = !next_found;
                if (!next_found) break;
                consumed++;
            }
            return MatchResult(true, consumed);
        }
        else if (pattern[ppos] == '?') {
            ppos++; // Consume the ?
            return MatchResult(true, 1);
        }
        else if (pattern[ppos] == '*') {
            ppos++; // Consume the *
            size_t consumed = 1; // Already matched one
            while (pos + consumed < input.size()) {
                char next_ch = input[pos + consumed];
                bool next_found = char_class.find(next_ch) != std::string::npos;
                if (negated) next_found = !next_found;
                if (!next_found) break;
                consumed++;
            }
            return MatchResult(true, consumed);
        }
    }
    
    return MatchResult(true, 1);
}

MatchResult match_escape(const std::string& input, size_t pos, const std::string& pattern, size_t& ppos) {
    if (ppos + 1 >= pattern.size() || pattern[ppos] != '\\') {
        return MatchResult(false, 0);
    }
    
    char escape_char = pattern[ppos + 1];
    ppos += 2; // Consume \x
    
    if (pos >= input.size()) {
        return MatchResult(false, 0);
    }
    
    bool matches = false;
    if (escape_char == 'd') {
        matches = isdigit(input[pos]);
    } else if (escape_char == 'w') {
        matches = is_word_char(input[pos]);
    } else if (escape_char == 's') {
        matches = isspace(input[pos]);
    } else {
        // Literal escaped character
        matches = (input[pos] == escape_char);
    }
    
    if (!matches) {
        return MatchResult(false, 0);
    }
    
    // Handle quantifiers
    if (ppos < pattern.size()) {
        if (pattern[ppos] == '+') {
            ppos++; // Consume the +
            size_t consumed = 1; // Already matched one
            while (pos + consumed < input.size()) {
                bool next_matches = false;
                if (escape_char == 'd') {
                    next_matches = isdigit(input[pos + consumed]);
                } else if (escape_char == 'w') {
                    next_matches = is_word_char(input[pos + consumed]);
                } else if (escape_char == 's') {
                    next_matches = isspace(input[pos + consumed]);
                } else {
                    next_matches = (input[pos + consumed] == escape_char);
                }
                if (!next_matches) break;
                consumed++;
            }
            return MatchResult(true, consumed);
        }
        else if (pattern[ppos] == '?') {
            ppos++; // Consume the ?
            return MatchResult(true, 1);
        }
        else if (pattern[ppos] == '*') {
            ppos++; // Consume the *
            size_t consumed = 1; // Already matched one
            while (pos + consumed < input.size()) {
                bool next_matches = false;
                if (escape_char == 'd') {
                    next_matches = isdigit(input[pos + consumed]);
                } else if (escape_char == 'w') {
                    next_matches = is_word_char(input[pos + consumed]);
                } else if (escape_char == 's') {
                    next_matches = isspace(input[pos + consumed]);
                } else {
                    next_matches = (input[pos + consumed] == escape_char);
                }
                if (!next_matches) break;
                consumed++;
            }
            return MatchResult(true, consumed);
        }
    }
    
    return MatchResult(true, 1);
}

MatchResult match_literal(const std::string& input, size_t pos, char ch) {
    if (pos >= input.size() || input[pos] != ch) {
        return MatchResult(false, 0);
    }
    return MatchResult(true, 1);
}

MatchResult match_pattern(const std::string& input, size_t pos, const std::string& pattern, size_t ppos) {
    while (ppos < pattern.size()) {
        char ch = pattern[ppos];
        
        if (ch == '$') {
            // End anchor
            return MatchResult(pos == input.size(), 0);
        }
        else if (ch == '(') {
            MatchResult result = match_group(input, pos, pattern, ppos);
            if (!result.matched) {
                return MatchResult(false, 0);
            }
            pos += result.consumed;
        }
        else if (ch == '[') {
            MatchResult result = match_char_class(input, pos, pattern, ppos);
            if (!result.matched) {
                return MatchResult(false, 0);
            }
            pos += result.consumed;
        }
        else if (ch == '\\') {
            MatchResult result = match_escape(input, pos, pattern, ppos);
            if (!result.matched) {
                return MatchResult(false, 0);
            }
            pos += result.consumed;
        }
        else if (ch == '.') {
            if (pos >= input.size()) {
                return MatchResult(false, 0);
            }
            ppos++;
            
            // Handle quantifiers
            if (ppos < pattern.size()) {
                if (pattern[ppos] == '+') {
                    ppos++; // Consume the +
                    size_t consumed = 1; // Already matched one
                    while (pos + consumed < input.size()) {
                        consumed++; // . matches any character
                    }
                    pos += consumed;
                } else if (pattern[ppos] == '?') {
                    ppos++; // Consume the ?
                    pos++; // Match one character
                } else if (pattern[ppos] == '*') {
                    ppos++; // Consume the *
                    pos = input.size(); // Match all remaining characters
                } else {
                    pos++; // Match one character
                }
            } else {
                pos++; // Match one character
            }
        }
        else {
            // Literal character
            MatchResult result = match_literal(input, pos, ch);
            if (!result.matched) {
                return MatchResult(false, 0);
            }
            pos += result.consumed;
            ppos++;
            
            // Handle quantifiers
            if (ppos < pattern.size()) {
                if (pattern[ppos] == '+') {
                    ppos++; // Consume the +
                    while (pos < input.size() && input[pos] == ch) {
                        pos++;
                    }
                } else if (pattern[ppos] == '?') {
                    ppos++; // Consume the ?
                    // Already matched one, ? means optional so we're done
                } else if (pattern[ppos] == '*') {
                    ppos++; // Consume the *
                    while (pos < input.size() && input[pos] == ch) {
                        pos++;
                    }
                }
            }
        }
    }
    
    return MatchResult(true, pos);
}

bool match_patterns(const std::string& input, const std::string& pattern) {
    if (pattern.empty()) {
        return input.empty();
    }
    
    if (pattern[0] == '^') {
        // Anchored at start
        MatchResult result = match_pattern(input, 0, pattern.substr(1), 0);
        return result.matched && (pattern.back() == '$' ? result.consumed == input.size() : true);
    }
    
    // Try matching at each position
    for (size_t i = 0; i <= input.size(); i++) {
        MatchResult result = match_pattern(input, i, pattern, 0);
        if (result.matched) {
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