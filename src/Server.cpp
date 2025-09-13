#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <algorithm>

// Thread-local storage for backreferences with proper context handling
thread_local std::vector<std::string> backreferences;

// Forward declaration
std::vector<int> match_pattern(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos, int group_index);

// Recursively resolve backreferences within captured text
std::string resolve_backref(const std::string& text, const std::vector<std::string>& captures) {
    std::string result;
    for (int i = 0; i < static_cast<int>(text.size()); i++) {
        if (text[i] == '\\' && i + 1 < static_cast<int>(text.size()) && std::isdigit(text[i+1])) {
            int num = text[i+1] - '0';
            if (num > 0 && num <= static_cast<int>(captures.size()) && !captures[num-1].empty()) {
                result += resolve_backref(captures[num-1], captures); // recursive resolution
            }
            i++; // skip the digit
        } else {
            result += text[i];
        }
    }
    return result;
}

// Count total number of capturing groups
int count_groups(const std::string& pattern) {
    int count = 0;
    for (int i = 0; i < (int)pattern.size(); i++) {
        if (pattern[i] == '\\') {
            i++; // skip escaped char
        } else if (pattern[i] == '(') {
            count++;
        }
    }
    return count;
}

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
    // TODO: Add support for ranges like a-z, 0-9, etc.
    bool found = char_class.find(c) != std::string::npos;
    return negated ? !found : found;
}

// Find matching closing bracket/paren with proper escaping and depth, with seperation of logic for [] and ()
int find_closing_bracket(const std::string& pattern, int start, char open_bracket) {
    int i = start + 1;

    //* Here static_cast is used for safer explicit conversion from size_t or other formats to int
    if (open_bracket == '[') {
        // Skip leading '^' in negated character classes
        if (i < static_cast<int>(pattern.length()) && pattern[i] == '^') {
            i++;
        }
        // Scan until unescaped ']'
        while (i < static_cast<int>(pattern.length())) {
            if (pattern[i] == '\\') { //* Unnecessary optimization for large sentences
                i += 2; // skip escaped char
                continue;
            }
            if (pattern[i] == ']') {
                return i;
            }
            i++;
        }
        return i; // not found; return end
    }

    // Handle parentheses groups with nesting
    int depth = 1;
    while (i < static_cast<int>(pattern.length()) && depth > 0) {
        if (pattern[i] == '\\') {
            i += 2; // skip escaped char
            continue;
        }
        if (pattern[i] == '(') depth++;
        else if (pattern[i] == ')') depth--;
        i++;
    }
    return i - 1; // position of ')'
}

// Calculate length of pattern element (char, escape, class, or group)
int get_element_length(const std::string& pattern, int idx) {
    if (idx >= pattern.length()) return 0;

    if (pattern[idx] == '\\') return 2; // escape sequences
    else if (pattern[idx] == '[') return find_closing_bracket(pattern, idx, '[') - idx + 1; // char class
    else if (pattern[idx] == '(') return find_closing_bracket(pattern, idx, '(') - idx + 1; // group
    else return 1; // single character
}

// Check if single character matches at given position
bool match_position(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos) {
    if (input_pos >= input_line.length()) {
        return false;
    }

    char current_char = input_line[input_pos];

    // Process escape sequences
    if (pattern[pattern_pos] == '\\' && pattern_pos + 1 < pattern.length()) {
        char next = pattern[pattern_pos + 1];
        if (next == 'd') {
            return std::isdigit(current_char); // digit shorthand
        }
        else if (next == 'w') {
            return std::isalnum(current_char) || current_char == '_'; // word character
        }
        else {
            return current_char == next; // literal escaped character
        }
    }

    // Wildcard matches any character
    if (pattern[pattern_pos] == '.') {
        return true;
    }

    // Direct character match
    return current_char == pattern[pattern_pos];
}

// Handle groups with alternation (|) and capture for backreferences
std::vector<int> match_group(const std::string& input_line, int input_pos, const std::string& group_content, int group_index) {
    std::vector<int> results;

    // Parse alternatives separated by | (respecting nesting)
    std::vector<std::string> alternatives;
    std::string current;
    int depth = 0;

    // Split group content by | at top level only
    for (int i = 0; i < static_cast<int>(group_content.length()); i++) {
        char c = group_content[i];
        
        // Handle escaped characters
        if (c == '\\' && i + 1 < static_cast<int>(group_content.length())) {
            current += c;
            current += group_content[++i]; // include escaped char
        } 
        else if (c == '(') {
            depth++; // enter nested group
            current += c;
        } 
        else if (c == ')') {
            depth--; // exit nested group
            current += c;
        } 
        else if (c == '|' && depth == 0) {
            alternatives.push_back(current); // top-level alternation
            current = "";
        }  
        else {
            current += c; // regular character
        }
    }

    // Base case: Handle when there's no alternations left
    if (!current.empty()) {
        alternatives.push_back(current);
    }

    // Try each alternative
    for (const std::string& alt : alternatives) {
        std::vector<int> alt_results = match_pattern(input_line, input_pos, alt, 0, group_index + 1);
        results.insert(results.end(), alt_results.begin(), alt_results.end());
    }
    
    return results;
}

// Apply quantifiers (?, +) to pattern elements
std::vector<int> match_quantifier(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos, int group_index) {
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

    // Process capturing groups with proper context handling
    if (pattern[pattern_pos] == '(') {
        int close = find_closing_bracket(pattern, pattern_pos, '(');
        std::string group_content = pattern.substr(pattern_pos + 1, close - pattern_pos - 1);

        // Save current capture state
        auto saved_captures = backreferences;
        
        if (quantifier == '?') {
            results.push_back(input_pos); // match 0 times
            
            // Try matching 1 time with local capture context
            std::vector<int> group_results = match_group(input_line, input_pos, group_content, group_index);
            for (int end_pos : group_results) {
                if (end_pos > input_pos && group_index + 1 <= static_cast<int>(backreferences.size())) {
                    // Capture the matched text for this group
                    std::string matched_text = input_line.substr(input_pos, end_pos - input_pos);
                    backreferences[group_index - 1] = matched_text;  // fixed index
                }
            }
            results.insert(results.end(), group_results.begin(), group_results.end());
        } 
        else if (quantifier == '+') {
            // Require at least one match
            std::vector<int> first_match = match_group(input_line, input_pos, group_content, group_index);
            for (int end_pos : first_match) {
                if (end_pos > input_pos && group_index + 1 <= static_cast<int>(backreferences.size())) {
                    // Capture the matched text for this group (first match only for + quantifier)
                    std::string matched_text = input_line.substr(input_pos, end_pos - input_pos);
                    backreferences[group_index - 1] = matched_text;  // fixed index
                }
                results.push_back(end_pos);
                
                // Try additional repetitions recursively
                std::vector<int> more = match_quantifier(input_line, end_pos, pattern, pattern_pos, group_index);
                results.insert(results.end(), more.begin(), more.end());
            }
        }
        // TODO: Add support for * and {n,m} quantifiers
        else {
            // Match exactly once
            std::vector<int> group_results = match_group(input_line, input_pos, group_content, group_index);
            for (int end_pos : group_results) {
                if (end_pos > input_pos && group_index + 1 <= static_cast<int>(backreferences.size())) {
                    // Capture the matched text for this group
                    std::string matched_text = input_line.substr(input_pos, end_pos - input_pos);
                    backreferences[group_index - 1] = matched_text;  // fixed index
                }
            }
            results = group_results;
        }
        
        // If no results, restore previous capture state
        if (results.empty()) {
            backreferences = saved_captures;
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
std::vector<int> match_pattern(const std::string& input_line, int input_pos, const std::string& pattern, int pattern_pos, int group_index) {
    std::vector<int> results;

    // Base case: reached end of pattern
    if (pattern_pos >= static_cast<int>(pattern.length())) {
        results.push_back(input_pos);
        return results;
    }
    
    // Handle start anchor
    if (pattern[pattern_pos] == '^') {
        if (input_pos != 0) {
            return results;
        }
        return match_pattern(input_line, input_pos, pattern, pattern_pos + 1, group_index);
    }
    
    // Handle end anchor
    if (pattern[pattern_pos] == '$') {
        if (input_pos != input_line.length()) {
            return results;
        }
        return match_pattern(input_line, input_pos, pattern, pattern_pos + 1, group_index);
    }
    
    // Handle backreferences (multi-character) with recursive resolution
    if (pattern[pattern_pos] == '\\' && pattern_pos + 1 < static_cast<int>(pattern.length()) && std::isdigit(pattern[pattern_pos + 1])) {
        
        int backref_num = pattern[pattern_pos + 1] - '0';
        if (backref_num > 0 && backref_num <= static_cast<int>(backreferences.size()) && !backreferences[backref_num - 1].empty()) {
            // Resolve backreference recursively to handle nested references
            std::string backref_text = resolve_backref(backreferences[backref_num - 1], backreferences);
            
            // Check if the resolved backreference matches at current position
            if (input_pos + static_cast<int>(backref_text.length()) <= static_cast<int>(input_line.length()) && 
                input_line.substr(input_pos, backref_text.length()) == backref_text) {
                // Match found, continue with rest of pattern
                return match_pattern(input_line, input_pos + backref_text.length(), pattern, pattern_pos + 2, group_index);
            }
            // No match
            return results;
        }
        // Invalid or empty backreference
        return results;
    }
    
    // Get element length and check for quantifier
    int elem_len = get_element_length(pattern, pattern_pos);
    int next_pattern_pos = pattern_pos + elem_len;
    
    // Skip quantifier if present
    if (next_pattern_pos < static_cast<int>(pattern.length()) && (pattern[next_pattern_pos] == '?' || pattern[next_pattern_pos] == '+')) {
        next_pattern_pos++;
    }
    
    // Get all possible positions after matching current element
    std::vector<int> possible_positions = match_quantifier(input_line, input_pos, pattern, pattern_pos, group_index);
    
    // Continue matching from each possible position
    for (int next_input_pos : possible_positions) {
        std::vector<int> remaining = match_pattern(input_line, next_input_pos, pattern, next_pattern_pos, group_index);
        results.insert(results.end(), remaining.begin(), remaining.end());
    }
    return results;
}

// Match complete string against pattern
bool match_string(const std::string& input_line, const std::string& pattern) {

    int group_count = count_groups(pattern);
    if (group_count > 0) {
        backreferences.assign(group_count, ""); // Clear the list and give it fixed-size storage
    } else {
        backreferences.clear();
    }

    // Check if pattern has start anchor
    bool has_start_anchor = !pattern.empty() && pattern[0] == '^';
    
    if (has_start_anchor) {
        // Must match from the beginning
        std::vector<int> results = match_pattern(input_line, 0, pattern, 0, 0);
        return !results.empty();
    } else {
        // Can match anywhere in the input
        for (int i = 0; i <= static_cast<int>(input_line.length()); i++) {
            if (group_count > 0) {
                backreferences.assign(group_count, ""); // Clear the list and give it fixed-size storage
            } else {
                backreferences.clear();
            }
            std::vector<int> results = match_pattern(input_line, i, pattern, 0, 0);
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

    // Expected usage: 
    // ./program -E pattern (read from stdin)
    // ./program -E pattern filename... (read from files)
    // ./program -r -E pattern directory (recursive search in directory)
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [-r] -E pattern [filename|directory]" << std::endl;
        return 1;
    }

    // Parse command line arguments
    bool recursion_flag = (std::strcmp(argv[1], "-r") == 0) ? true : false;
    int arg_offset = (recursion_flag) ? 1 : 0;
    
    std::string flag = argv[1 + arg_offset];
    std::string pattern = argv[2 + arg_offset];

    // Validate extended regex flag
    if (flag != "-E") {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    std::string input_line;
    
    // Match pattern against input
    try {
        if (argc > 3 && !recursion_flag) {
            // Read from files - process each line
            int line_count = 0;
            bool multiple_files = (argc > 4);
            
            for (int i = 3; i < argc; i++) {
                std::ifstream file(argv[i]);
                if (!file.is_open()) {
                    std::cerr << "Error: Could not open file '" << argv[i] << "'" << std::endl;
                    continue; // Continue with other files instead of exiting
                }
                
                // Process each line in the file
                while (std::getline(file, input_line)) {
                    bool match_found = match_string(input_line, pattern);
                    
                    if (match_found) {
                        if (multiple_files) std::cout << argv[i] << ":" << input_line << std::endl;
                        else std::cout << input_line << std::endl;
                        line_count++;
                    }
                }
                file.close();
            }
            return (line_count > 0) ? 0 : 1;
        } 
        else if (recursion_flag) {
            // Recursive directory search
            std::string directory_path = argv[4];
            int line_count = 0;
            
            // Store matches with file path for sorting
            std::vector<std::pair<std::string, std::string>> matches;

            try {
                // Recursively iterate through all files in the directory
                for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
                    // Skip directories, only process regular files
                    if (!entry.is_regular_file()) {
                        continue;
                    }
                    
                    std::ifstream file(entry.path());
                    if (!file.is_open()) {
                        std::cerr << "Warning: Could not open file '" << entry.path() << "'" << std::endl;
                        continue; // Continue with other files
                    }

                    // Process each line in the file
                    while (std::getline(file, input_line)) {
                        bool match_found = match_string(input_line, pattern);
                        
                        if (match_found) {
                            // Store as pair: (filepath, matched_line)
                            matches.push_back({entry.path().string(), input_line});
                            line_count++;
                        }
                    }
                    file.close();
                }
                
                // Sort by file path first, then by line content
                std::sort(matches.begin(), matches.end());
                
                // Print sorted results
                for (const auto& match : matches) {
                    std::cout << match.first << ":" << match.second << std::endl;
                }
                
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Filesystem error: " << e.what() << std::endl;
                return 1;
            }
            // Return 0 if matches found, 1 if not
            return (line_count > 0) ? 0 : 1;
        }
        else {
            // Read from stdin - process single line
            std::getline(std::cin, input_line);
            
            // Match pattern against input
            bool match_found = match_string(input_line, pattern);
            // Stdin mode: return 0 if match found, 1 if not (opposite of match result)
            return !match_found;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}