#include <iostream>
#include <string>


bool match_digits(const std::string& inputs, const std::string& pattern) {
    for (auto input : inputs) {
        if (isdigit(input)) {
            return true;
        }
    }
    return false;
}

bool match_alphanum(const std::string& inputs, const std::string& pattern){
    for (auto c : inputs) {
        if (isalnum(c)) {
            return true;
        }
    }
    return false;
}

bool match_group(const std::string& input_line, const std::string& pattern) {
    return input_line.find_first_of(pattern) != std::string::npos;
    //returns the position of the first occurrence of any character that is present in the argument string
}

bool match_pattern(const std::string& input_line, const std::string& pattern, bool start_of_line = true) {
    if(pattern.size()==0) return true;
    if(pattern[0]=='$') {
        if(input_line.empty()) {
            return true; 
        }
        else{
            return false;
        }   
    }
    if(input_line.size()==0) return false;

    if (pattern[0] == '(' && pattern.find('|') != std::string::npos) {
        std::string subpattern1= pattern.substr(1, pattern.find('|') - 1);
        std::string subpattern2 = pattern.substr(pattern.find('|') + 1, pattern.find(')') - pattern.find('|') - 1);
        return match_pattern(input_line, subpattern1, start_of_line) || match_pattern(input_line, subpattern2, start_of_line);
    }

    if (pattern.size() > 1 &&pattern[1] == '+') {
        char target = pattern[0];
        size_t i = 0;
        while (i < input_line.size() && input_line[i] == target) {
            i++;
        }
        return i > 0 && match_pattern(input_line.substr(i), pattern.substr(2), false);
    }
    if (pattern.size() > 1 && pattern[1] == '?') {
        if (pattern[0] == input_line[0]) {
            return match_pattern(input_line.substr(1), pattern.substr(2), false) ||
                   match_pattern(input_line, pattern.substr(2), false);
        } else {
            return match_pattern(input_line, pattern.substr(2), false);
        }
    }
    if (pattern[0] == '.') {
        return match_pattern(input_line.substr(1), pattern.substr(1), false);
    }   
    if (pattern.substr(0,2) == "\\d") {
        if(isdigit(input_line[0])){
            return match_pattern(input_line.substr(1), pattern.substr(2), false);
        }
        return match_pattern(input_line.substr(1), pattern, false);
    }

    else if (pattern.substr(0,2) == "\\w") {
        if(isalnum(input_line[0])){
            return match_pattern(input_line.substr(1), pattern.substr(2), false);
        }
        return match_pattern(input_line.substr(1), pattern, false);
    }

    else if(pattern[0]=='['){
        auto first = pattern.find(']');
        bool neg = pattern[1]=='^';
        if(neg){
            if(!match_group(input_line,pattern.substr(2,first-1))){
                return match_pattern(input_line.substr(1),pattern.substr(first+1), false);
            }
            return false;
        }
        if(match_group(input_line,pattern.substr(1,first-1))){
            return match_pattern(input_line.substr(1),pattern.substr(first+1), false);
        }
        else
        return false;
    }

    if (pattern[0] == input_line[0]) {
        return match_pattern(input_line.substr(1), pattern.substr(1), false);
    }
    else if(start_of_line){
        return false;
    }
    return match_pattern(input_line.substr(1), pattern, false);
}

// Update match_patterns function
bool match_patterns(const std::string& input_line, const std::string& pattern) {
    if (pattern[0] == '^') {
        return match_pattern(input_line, pattern.substr(1), true);
    }
    
    return match_pattern(input_line, pattern, false);
}

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here" << std::endl;

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