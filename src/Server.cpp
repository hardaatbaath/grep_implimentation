#include <iostream>
#include <string>
#include <vector>

// Forward declaration
std::vector<int> match_pattern(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos);

// Parse character class [abc] or [^abc]
std::pair<std::string, bool> parse_char_class(const std::string& pattern, int start) {
    int i = start + 1;  // Skip '['
    bool negated = false;
    
    // Check for negation
    if (i < pattern.length() && pattern[i] == '^') {
        negated = true;
        i++;
    }

    // Extract characters until closing bracket
    std::string chars;
    while (i < pattern.length() && pattern[i] != ']') {
        chars += pattern[i];
        i++;
    }
    return {chars, negated};
}

// Match character against character class
bool match_char_class(char c, const std::string& char_class, bool negated) {
    bool found = char_class.find(c) != std::string::npos;
    return negated ? !found : found;
}

// Find matching closing bracket
int find_closing_bracket(const std::string& pattern, int start, char open_bracket) {
    int i = start + 1;
    char close_bracket = (open_bracket == '(') ? ')' : ']';

    // Skip potential negation in character classes
    if (open_bracket == '[' && i < pattern.length() && pattern[i] == '^') {
        i++;
    }

    // Find matching closing bracket
    while (i < pattern.length() && pattern[i] != close_bracket) {
        i++;
    }
    return i;
}

// Get length of pattern element at current position
int get_element_length(const std::string& pattern, int idx) {
    if (idx >= pattern.length()) return 0;

    if (pattern[idx] == '\\') {
        return 2; // Default for escape sequences
    } 
    else if (pattern[idx] == '[') return find_closing_bracket(pattern, idx, '[') - idx + 1; 
    else if (pattern[idx] == '(') return find_closing_bracket(pattern, idx, '(') - idx + 1; 
    else return 1;
}

// Match single character at specific position
bool match_position(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos) {
    if (input_pos >= input_line.length()) {
        return false;
    }

    char current_char = input_line[input_pos];

    // Handle escape sequences
    if (pattern[pattern_pos] == '\\' && pattern_pos + 1 < pattern.length()) {
        char next = pattern[pattern_pos + 1];
        if (next == 'd') {
            return std::isdigit(current_char);
        }
        else if (next == 'w') {
            return std::isalnum(current_char) || current_char == '_';
        }
        else {
            return current_char == next;
        }
    }

    // Match any character
    if (pattern[pattern_pos] == '.') {
        return true;
    }

    return current_char == pattern[pattern_pos];
}

// Handle groups with alternation (|)
std::vector<int> match_group(const std::string& input_line, int input_pos, const std::string& group_content) {
    std::vector<int> results;

    // Split by | while respecting nested groups
    std::vector<std::string> alternatives;
    std::string current;
    int depth = 0;

    // Parse alternatives separated by |
    for (int i = 0; i < group_content.length(); i++) {
        char c = group_content[i];
        
        // Handle escaped characters
        if (c == '\\' && i + 1 < group_content.length()) {
            current += c;
            current += group_content[++i];
        } 
        // Handle new nested groups
        else if (c == '(') {
            // Increase depth
            depth++;
            current += c;
        } 
        // Handle old nested groups
        else if (c == ')') {
            depth--;
            current += c;
        } 
        // Handle alternation
        else if (c == '|' && depth == 0) {
            alternatives.push_back(current);
            current = "";
        }  
        // Handle regular characters
        else {
            current += c;
        }
    }

    // Base case: Handle when there's no alternations left
    if (!current.empty()) {
        alternatives.push_back(current);
    }

    // Try each alternative
    for (const std::string& alt : alternatives) {
        std::vector<int> alt_results = match_pattern(input_line, input_pos, alt, 0);
        results.insert(results.end(), alt_results.begin(), alt_results.end());
    }
    
    return results;
}

// Handle quantifiers (?, +)
std::vector<int> match_quantifier(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos) {
    std::vector<int> results;

    // Base case: reached end of pattern
    if (pattern_pos >= pattern.length()) {
        results.push_back(input_pos);
        return results;
    }

    // Get element length, if 0 that means it's a normal character, return input_pos
    int elem_len = get_element_length(pattern, pattern_pos);
    if (elem_len == 0) {
        results.push_back(input_pos);
        return results;
    }

    // Check for quantifier after current element
    char quantifier = '\0';
    if (pattern_pos + elem_len < pattern.length()) {
        char next = pattern[pattern_pos + elem_len];
        if (next == '?' || next == '+') {
            quantifier = next;
        }
    }

    // Handle parentheses groups
    if (pattern[pattern_pos] == '(') {
        int close = find_closing_bracket(pattern, pattern_pos, '(');
        std::string group_content = pattern.substr(pattern_pos + 1, close - pattern_pos - 1);

        if (quantifier == '?') {
            // Match 0 times
            results.push_back(input_pos);
            // Match 1 time
            std::vector<int> group_results = match_group(input_line, input_pos, group_content);
            results.insert(results.end(), group_results.begin(), group_results.end());
        } 
        else if (quantifier == '+') {
            // Must match at least once
            std::vector<int> group_results = match_group(input_line, input_pos, group_content);
            for (int end_pos : group_results) {
                // push one repetition
                results.push_back(end_pos);
                // Try more repetitions
                std::vector<int> more = match_quantifier(input_line, end_pos, pattern, pattern_pos);
                results.insert(results.end(), more.begin(), more.end());
            }
            results.insert(results.end(), group_results.begin(), group_results.end());
        }
        else {
            // No quantifier - match exactly once
            results = match_group(input_line, input_pos, group_content);
        }
        return results;
    }
    
    // Handle character classes []
    if (pattern[pattern_pos] == '[') {
        int close = find_closing_bracket(pattern, pattern_pos, '[');
        auto [char_class, negated] = parse_char_class(pattern, pattern_pos);

        // If the character matches the character class
        if (input_pos < input_line.length() && match_char_class(input_line[input_pos], char_class, negated)) {
            if (quantifier == '?') {
                results.push_back(input_pos);      // Match 0 times
                results.push_back(input_pos + 1);  // Match 1 time
            }
            else if (quantifier == '+') {
                // Match 1 or more times
                int current = input_pos;
                while (current < input_line.length() && match_char_class(input_line[current], char_class, negated)) {
                    current++;
                    results.push_back(current);
                }
            }
            else {
                // No quantifier - match exactly once
                results.push_back(input_pos + 1);
            }
        }
        else if (quantifier == '?') {
            // Optional and didn't match - that's ok
            results.push_back(input_pos);
        }
        return results;
    }
    
    // Handle regular characters, ., \d, \w
    bool matches = match_position(input_line, input_pos, pattern, pattern_pos);

    if (quantifier == '?') {
        // Match 0 times
        results.push_back(input_pos);
        // Match 1 time if possible
        if (matches) {
            results.push_back(input_pos + 1);
        }
    }
    else if (quantifier == '+') {
        // Must match at least once
        if (matches) {
            int current = input_pos + 1;
            results.push_back(current);
            // Continue matching while possible
            while (current < input_line.length() && match_position(input_line, current, pattern, pattern_pos)) {
                current++;
                results.push_back(current);
            }
        }
    }
    else {
        // No quantifier - match exactly once
        if (matches) {
            results.push_back(input_pos + 1);
        }
    }
    return results;
}

// Match pattern starting at given positions
std::vector<int> match_pattern(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos) {
    std::vector<int> results;

    // Base case: reached end of pattern
    if (pattern_pos >= pattern.length()) {
        results.push_back(input_pos);
        return results;
    }
    
    // Handle start anchor
    if (pattern[pattern_pos] == '^') {
        if (input_pos != 0) {
            return results;
        }
        return match_pattern(input_line, input_pos, pattern, pattern_pos + 1);
    }
    
    // Handle end anchor
    if (pattern[pattern_pos] == '$') {
        if (input_pos != input_line.length()) {
            return results;
        }
        return match_pattern(input_line, input_pos, pattern, pattern_pos + 1);
    }
    
    // Get element length and check for quantifier
    int elem_len = get_element_length(pattern, pattern_pos);
    int next_pattern_pos = pattern_pos + elem_len;
    
    // Skip quantifier if present
    if (next_pattern_pos < pattern.length() && (pattern[next_pattern_pos] == '?' || pattern[next_pattern_pos] == '+')) {
        next_pattern_pos++;
    }
    
    // Get all possible positions after matching current element
    std::vector<int> possible_positions = match_quantifier(input_line, input_pos, pattern, pattern_pos);
    
    // Continue matching from each possible position
    for (int next_input_pos : possible_positions) {
        std::vector<int> remaining = match_pattern(input_line, next_input_pos, pattern, next_pattern_pos);
        results.insert(results.end(), remaining.begin(), remaining.end());
    }
    return results;
}

// Match complete string against pattern
bool match_string(const std::string& input_line, const std::string& pattern) {
    // Check if the pattern has a start anchor
    bool has_start_anchor = !pattern.empty() && pattern[0] == '^';

    if (has_start_anchor) {
        // Must match from the beginning
        std::vector<int> results = match_pattern(input_line, 0, pattern, 0);
        return !results.empty();
    } 
    else {
        // Try matching from each position
        for (int i = 0; i <= input_line.length(); i++) {
            std::vector<int> results = match_pattern(input_line, i, pattern, 0);
            if (!results.empty()) {
                return true;
            }
        }
        return false;
    }
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
        return !match_string(input_line, pattern);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}