#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// Forward declarations for mutual recursion
vector<int> matchPattern(const string& text, int pos, const string& pattern, int pat_idx);
vector<int> matchGroup(const string& text, int pos, const string& group_content);

// Function 1: Match at a specific position for character-level patterns
bool matchPosition(const string& text, int pos, const string& pattern, int pat_idx) {
    if (pos >= text.length()) {
        return false;
    }
    
    char c = text[pos];
    
    // Check for escape sequences
    if (pattern[pat_idx] == '\\' && pat_idx + 1 < pattern.length()) {
        char next = pattern[pat_idx + 1];
        if (next == 'd') {
            return isdigit(c);
        } else if (next == 'w') {
            return isalnum(c) || c == '_';
        } else {
            // Escaped character - match literally
            return c == next;
        }
    }
    
    // Match any character
    if (pattern[pat_idx] == '.') {
        return true;
    }
    
    // Match literal character
    return c == pattern[pat_idx];
}

// Helper: Parse character class [abc] or [^abc]
pair<string, bool> parseCharClass(const string& pattern, int start) {
    int i = start + 1;  // Skip '['
    bool negated = false;
    
    if (i < pattern.length() && pattern[i] == '^') {
        negated = true;
        i++;
    }
    
    string chars;
    while (i < pattern.length() && pattern[i] != ']') {
        chars += pattern[i];
        i++;
    }
    
    return {chars, negated};
}

// Helper: Match character against character class
bool matchCharClass(char c, const string& char_class, bool negated) {
    bool found = char_class.find(c) != string::npos;
    return negated ? !found : found;
}

// Helper: Find matching closing parenthesis
int findClosingParen(const string& pattern, int start) {
    int depth = 1;
    int i = start + 1;
    
    while (i < pattern.length() && depth > 0) {
        if (pattern[i] == '\\') {
            i += 2;  // Skip escaped character
            continue;
        }
        if (pattern[i] == '(') depth++;
        else if (pattern[i] == ')') depth--;
        i++;
    }
    
    return i - 1;  // Position of closing paren
}

// Helper: Find matching closing bracket
int findClosingBracket(const string& pattern, int start) {
    int i = start + 1;
    
    // Skip potential '^'
    if (i < pattern.length() && pattern[i] == '^') {
        i++;
    }
    
    while (i < pattern.length() && pattern[i] != ']') {
        if (pattern[i] == '\\') {
            i += 2;  // Skip escaped character
            continue;
        }
        i++;
    }
    
    return i;  // Position of closing bracket
}

// Helper: Get the length of current pattern element
int getElementLength(const string& pattern, int idx) {
    if (idx >= pattern.length()) return 0;
    
    if (pattern[idx] == '\\') {
        return 2;  // Escape sequence
    } else if (pattern[idx] == '[') {
        return findClosingBracket(pattern, idx) - idx + 1;
    } else if (pattern[idx] == '(') {
        return findClosingParen(pattern, idx) - idx + 1;
    } else {
        return 1;  // Single character
    }
}

// Function 2: Handle quantifiers (?, +) and repetition
vector<int> matchQuantifier(const string& text, int pos, const string& pattern, int pat_idx) {
    vector<int> results;
    
    if (pat_idx >= pattern.length()) {
        results.push_back(pos);
        return results;
    }
    
    // Get current element length
    int elem_len = getElementLength(pattern, pat_idx);
    if (elem_len == 0) {
        results.push_back(pos);
        return results;
    }
    
    // Check if there's a quantifier after this element
    char quantifier = '\0';
    if (pat_idx + elem_len < pattern.length()) {
        char next = pattern[pat_idx + elem_len];
        if (next == '?' || next == '+') {
            quantifier = next;
        }
    }
    
    // Handle groups
    if (pattern[pat_idx] == '(') {
        int close = findClosingParen(pattern, pat_idx);
        string group_content = pattern.substr(pat_idx + 1, close - pat_idx - 1);
        
        if (quantifier == '?') {
            // Match 0 times
            results.push_back(pos);
            
            // Match 1 time
            vector<int> group_results = matchGroup(text, pos, group_content);
            results.insert(results.end(), group_results.begin(), group_results.end());
            
        } else if (quantifier == '+') {
    // Must match at least once
    vector<int> first_match = matchGroup(text, pos, group_content);
    for (int end_pos : first_match) {
        // push one repetition
        results.push_back(end_pos);

        // allow recursion: try more repetitions
        vector<int> more = matchQuantifier(text, end_pos, pattern, pat_idx);
        results.insert(results.end(), more.begin(), more.end());
    }
}
 else {
            // No quantifier - match exactly once
            results = matchGroup(text, pos, group_content);
        }
        
        return results;
    }
    
    // Handle character classes
    if (pattern[pat_idx] == '[') {
        int close = findClosingBracket(pattern, pat_idx);
        auto [char_class, negated] = parseCharClass(pattern, pat_idx);
        
        if (pos < text.length() && matchCharClass(text[pos], char_class, negated)) {
            if (quantifier == '?') {
                results.push_back(pos);      // Match 0 times
                results.push_back(pos + 1);  // Match 1 time
            } else if (quantifier == '+') {
                // Match 1 or more times
                int current = pos;
                while (current < text.length() && matchCharClass(text[current], char_class, negated)) {
                    current++;
                    results.push_back(current);
                }
            } else {
                // No quantifier - match exactly once
                results.push_back(pos + 1);
            }
        } else if (quantifier == '?') {
            // Optional and didn't match - that's ok
            results.push_back(pos);
        }
        
        return results;
    }
    
    // Handle regular characters, ., \d, \w
    bool matches = matchPosition(text, pos, pattern, pat_idx);
    
    if (quantifier == '?') {
        // Match 0 times
        results.push_back(pos);
        
        // Match 1 time if possible
        if (matches) {
            results.push_back(pos + 1);
        }
    } else if (quantifier == '+') {
        // Must match at least once
        if (matches) {
            int current = pos + 1;
            results.push_back(current);
            
            // Continue matching while possible
            while (current < text.length() && matchPosition(text, current, pattern, pat_idx)) {
                current++;
                results.push_back(current);
            }
        }
    } else {
        // No quantifier - must match exactly once
        if (matches) {
            results.push_back(pos + 1);
        }
    }
    
    return results;
}

// Function 3: Handle groups with alternation (|)
vector<int> matchGroup(const string& text, int pos, const string& group_content) {
    vector<int> results;
    
    // Split by | but respect nested groups
    vector<string> alternatives;
    string current;
    int depth = 0;
    
    for (int i = 0; i < group_content.length(); i++) {
        char c = group_content[i];
        
        if (c == '\\' && i + 1 < group_content.length()) {
            current += c;
            current += group_content[++i];
        } else if (c == '(') {
            depth++;
            current += c;
        } else if (c == ')') {
            depth--;
            current += c;
        } else if (c == '|' && depth == 0) {
            alternatives.push_back(current);
            current = "";
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        alternatives.push_back(current);
    }
    
    // Try each alternative
    for (const string& alt : alternatives) {
        vector<int> alt_results = matchPattern(text, pos, alt, 0);
        results.insert(results.end(), alt_results.begin(), alt_results.end());
    }
    
    return results;
}

// Main recursive matching function
vector<int> matchPattern(const string& text, int pos, const string& pattern, int pat_idx) {
    vector<int> results;
    
    // Base case: reached end of pattern
    if (pat_idx >= pattern.length()) {
        results.push_back(pos);
        return results;
    }
    
    // Handle anchors
    if (pattern[pat_idx] == '^') {
        if (pos != 0) {
            return results;  // Empty results - no match
        }
        return matchPattern(text, pos, pattern, pat_idx + 1);
    }
    
    if (pattern[pat_idx] == '$') {
        if (pos != text.length()) {
            return results;  // Empty results - no match
        }
        return matchPattern(text, pos, pattern, pat_idx + 1);
    }
    
    // Get element length and check for quantifier
    int elem_len = getElementLength(pattern, pat_idx);
    int next_idx = pat_idx + elem_len;
    
    // Check if there's a quantifier
    if (next_idx < pattern.length() && (pattern[next_idx] == '?' || pattern[next_idx] == '+')) {
        next_idx++;  // Skip quantifier
    }
    
    // Get possible positions after matching current element (with quantifier if any)
    vector<int> possible_positions = matchQuantifier(text, pos, pattern, pat_idx);
    
    // For each possible position, continue matching the rest of the pattern
    for (int next_pos : possible_positions) {
        vector<int> remaining = matchPattern(text, next_pos, pattern, next_idx);
        results.insert(results.end(), remaining.begin(), remaining.end());
    }
    
    return results;
}

// Main match function - checks if pattern matches anywhere in text
bool match(const string& text, const string& pattern) {
    // Check if pattern has start anchor
    bool has_start_anchor = !pattern.empty() && pattern[0] == '^';
    
    if (has_start_anchor) {
        // Must match from the beginning
        vector<int> results = matchPattern(text, 0, pattern, 0);
        return !results.empty();
    } else {
        // Can match anywhere in the text
        for (int i = 0; i <= text.length(); i++) {
            vector<int> results = matchPattern(text, i, pattern, 0);
            if (!results.empty()) {
                return true;
            }
        }
        return false;
    }
}

// Find all matches in text
vector<tuple<int, int, string>> findAll(const string& text, const string& pattern) {
    vector<tuple<int, int, string>> matches;
    bool has_start_anchor = !pattern.empty() && pattern[0] == '^';
    
    if (has_start_anchor) {
        vector<int> results = matchPattern(text, 0, pattern, 0);
        for (int end : results) {
            if (end > 0) {
                matches.push_back({0, end, text.substr(0, end)});
            }
        }
    } else {
        for (int i = 0; i < text.length(); i++) {
            vector<int> results = matchPattern(text, i, pattern, 0);
            for (int end : results) {
                if (end > i) {
                    matches.push_back({i, end, text.substr(i, end - i)});
                }
            }
        }
    }
    
    return matches;
}

// Grep function - search for pattern in text lines
int grep(const string& pattern, const string& text, bool show_line_numbers = false) {
    istringstream stream(text);
    string line;
    int line_num = 1;
    bool any_match = false;

    while (getline(stream, line)) {
        if (match(line, pattern)) {
            any_match = true;
            if (show_line_numbers) {
                cout << line_num << ":" << line << endl;
            } else {
                cout << line << endl;
            }
        }
        line_num++;
    }
    return any_match ? 1 : 0;
}


// Test function
void runTests() {
    cout << "Running test cases..." << endl;
    
    string test_text = 
        "I see 3 cats, 2 dogs and 1 cow\n"
        "I see 5 cows\n"
        "I see 1 cat and 2 dogs\n"
        "Hello world\n"
        "Test 123\n"
        "cat dog cow";
    
    vector<pair<string, string>> patterns = {
        {"\\d+", "Digits"},
        {"cat|dog", "Alternation"},
        {"cats?", "Optional s"},
        {"\\w+", "Word characters"},
        {"[aeiou]", "Vowels"},
        {"[^aeiou]", "Non-vowels"},
        {"^I see", "Start anchor"},
        {"cow$", "End anchor"},
        {"(cat|dog)s?", "Group with alternation and optional"},
        {"^I see (\\d (cat|dog|cow)s?(, | and )?)+$", "Complex nested pattern"}
    };
    
    cout << "Test text:" << endl;
    cout << test_text << endl;
    cout << string(50, '-') << endl;
    
    for (const auto& [pattern, description] : patterns) {
        cout << "\nPattern: " << pattern << " (" << description << ")" << endl;
        
        istringstream stream(test_text);
        string line;
        
        while (getline(stream, line)) {
            if (match(line, pattern)) {
                cout << "  ✓ '" << line << "'" << endl;
                
                auto matches = findAll(line, pattern);
                for (const auto& [start, end, matched] : matches) {
                    cout << "    Match at [" << start << ":" << end << "]: '" << matched << "'" << endl;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // if (argc < 2) {
    //     // Run tests if no arguments
    //     runTests();
    //     return 0;
    // }
    
    string pattern = R"(^I see (\d (cat|dog|cow)s?(, | and )?)+$)";
    string text = "I see 1 cat, 2 dogs and 3 cows";
    
    cout << grep(pattern, text, true);
    
    return 0;
}