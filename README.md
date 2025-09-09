# Grep Implementation

A feature-rich implementation of the `grep` command-line tool in C++, supporting extended regular expressions and pattern matching functionality.

## Overview

This project implements a custom version of the popular Unix `grep` utility that can search for patterns in text using regular expressions. The implementation supports many advanced regex features including character classes, quantifiers, groups with alternation, and anchors.

## Features

- **Basic Pattern Matching**: Search for literal strings and simple patterns
- **Character Classes**: Support for `[abc]`, `[^abc]`, and character ranges
- **Escape Sequences**: 
  - `\d` - matches any digit (0-9)
  - `\w` - matches word characters (alphanumeric + underscore)
  - `\.` - matches literal dot and other escaped characters
- **Wildcards**: `.` matches any single character
- **Quantifiers**: 
  - `?` - matches 0 or 1 occurrence (optional)
  - `+` - matches 1 or more occurrences
- **Groups and Alternation**: `(pattern1|pattern2)` for complex matching
- **Anchors**: 
  - `^` - matches start of line
  - `$` - matches end of line
- **Extended Regular Expression Support**: Uses `-E` flag for extended regex syntax

## Prerequisites

- **C++ Compiler**: GCC or Clang with C++23 support
- **CMake**: Version 3.13 or higher
- **Operating System**: Linux, macOS, or Windows with appropriate development tools

## Building the Project

1. **Clone the repository** (if not already done):
   ```bash
   git clone https://github.com/hardaatbaath/grep_implimentation.git
   cd grep_implimentation
   ```

2. **Build using CMake**:
   ```bash
   cmake -B build -S .
   cmake --build ./build
   ```

   The executable will be created at `./build/exe`.

## Running the Program

### Basic Usage

The program follows the standard grep interface:

```bash
./build/exe -E "pattern" < input_file
```

Or with piped input:
```bash
echo "text to search" | ./build/exe -E "pattern"
```

### Quick Start Script

For convenience, you can also use the provided shell script:

```bash
chmod +x your_program.sh
echo "hello world" | ./your_program.sh -E "hello"
```

## Usage Examples

### Basic String Matching
```bash
echo "hello world" | ./build/exe -E "hello"
# Exit code 0 (match found)

echo "hello world" | ./build/exe -E "goodbye" 
# Exit code 1 (no match)
```

### Character Classes
```bash
echo "abc123" | ./build/exe -E "[0-9]+"
# Matches one or more digits

echo "Hello" | ./build/exe -E "[A-Z]"
# Matches uppercase letters
```

### Escape Sequences
```bash
echo "test123" | ./build/exe -E "\\d+"
# Matches one or more digits

echo "hello_world" | ./build/exe -E "\\w+"
# Matches word characters
```

### Wildcards and Quantifiers
```bash
echo "cat" | ./build/exe -E "c.t"
# Matches: c followed by any character, then t

echo "color" | ./build/exe -E "colou?r"
# Matches both "color" and "colour"
```

### Groups and Alternation
```bash
echo "cat" | ./build/exe -E "(cat|dog)"
# Matches either "cat" or "dog"

echo "hello" | ./build/exe -E "(hi|hello|hey)"
# Matches any of the greetings
```

### Anchors
```bash
echo "hello world" | ./build/exe -E "^hello"
# Matches "hello" at start of line

echo "world" | ./build/exe -E "world$"
# Matches "world" at end of line
```

## Project Structure

```
.
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── your_program.sh         # Convenience script for running locally
├── src/
│   └── Server.cpp          # Main implementation file
└── build/                  # Build directory (created after building)
    └── exe                 # Compiled executable
```

## How It Works

The grep implementation uses a recursive pattern matching algorithm that:

1. **Parses** the input pattern into recognizable elements (characters, classes, groups, etc.)
2. **Matches** each element against the input text using backtracking
3. **Handles** quantifiers by exploring multiple possible match lengths
4. **Supports** alternation by trying each alternative in groups
5. **Returns** exit code 0 for matches found, 1 for no matches

## Development

This project was developed as part of the CodeCrafters "Build Your Own grep" challenge, implementing a fully functional grep alternative from scratch.

### Key Implementation Details

- **Pattern Parsing**: Handles complex nested structures and escape sequences
- **Backtracking Algorithm**: Explores all possible matches efficiently  
- **Memory Management**: Uses modern C++ features for safe string handling
- **Error Handling**: Provides clear error messages for invalid patterns

## Contributing

Feel free to submit issues, feature requests, or pull requests to improve the implementation.

## License

This project is part of a learning exercise and is provided as-is for educational purposes.
