#include <iostream>
#include <string>

bool match_pattern(const std::string& input_line, const std::string& pattern) {
    if (pattern.length() == 1) {
        return input_line.find(pattern) != std::string::npos;
    }
    else if (pattern == R"(\d)"){ // Using raw string literal to avoid escaping the backslash, can also use \\d
        return input_line.find_first_of("1234567890") != std::string::npos; // Better than looping through all the numbers
    }
    else if (pattern == "\\w") {
        return (input_line.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") != std::string::npos) || (match_pattern(input_line, "\\d"));
    }
    else if (pattern.starts_with("[") && pattern.ends_with("]")) {
        if (pattern.starts_with("[^")) {
            int length = pattern.length();
            std::string set = pattern.substr(2, length-3);
            for (char c : set) {
                if (input_line.find(c) != std::string::npos) {  // For all the chars, none should be in the input line
                    length -= 1;
                    std::cout << "Length: " << length << std::endl
                }
            }
            return length!=0; // If length is greater than 0, then at least one char didn't match
        }
        return input_line.find_first_of(pattern.substr(1, pattern.length()-2)) != std::string::npos; // -2 because substr takes (pos, length)
    }
    else {
        throw std::runtime_error("Unhandled pattern " + pattern);
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

    // Uncomment this block to pass the first stage
    
    std::string input_line;
    std::getline(std::cin, input_line); // To get the complete input line with spaces
    
    try {
        if (match_pattern(input_line, pattern)) {
            return 0;
        } else {
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
