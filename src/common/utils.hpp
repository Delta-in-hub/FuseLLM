// https://github.com/tgalaj/strutil
#pragma once
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view> // Included for std::string_view
#include <vector>

//! The strutil namespace
namespace strutil {
/**
 * @brief Converts any datatype into std::string.
 *        Datatype must support << operator.
 * @tparam T
 * @param value - will be converted into std::string.
 * @return Converted value as std::string.
 */
template <typename T> static inline std::string to_string(T value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

/**
 * @brief Converts std::string_view into any datatype.
 *        Datatype must support << operator.
 * @tparam T
 * @param str - std::string_view that will be converted into datatype T.
 * @return Variable of datatype T.
 */
template <typename T> static inline T parse_string(std::string_view str) {
    T result;
    // Use std::string to construct istringstream as it doesn't support
    // string_view directly
    std::istringstream(std::string(str)) >> result;
    return result;
}

/**
 * @brief Converts a string to lower case.
 * @param str - std::string_view to be converted.
 * @return Lower case version of the input string as a new std::string.
 */
static inline std::string to_lower(std::string_view str) {
    std::string result(str.length(), '\0');
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char c) {
        return static_cast<unsigned char>(std::tolower(c));
    });
    return result;
}

/**
 * @brief Converts a string to upper case.
 * @param str - std::string_view to be converted.
 * @return Upper case version of the input string as a new std::string.
 */
static inline std::string to_upper(std::string_view str) {
    std::string result(str.length(), '\0');
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char c) {
        return static_cast<unsigned char>(std::toupper(c));
    });
    return result;
}

/**
 * @brief Converts the first character of a string to uppercase letter and
 * lowercases all other characters, if any.
 * @param str - input string_view to be capitalized.
 * @return A string with the first letter capitalized and all other characters
 * lowercased.
 */
static inline std::string capitalize(std::string_view str) {
    if (str.empty()) {
        return "";
    }
    std::string result = to_lower(str);
    result.front() = static_cast<char>(std::toupper(result.front()));
    return result;
}

/**
 * @brief Converts only the first character of a string to uppercase letter, all
 * other characters stay unchanged.
 * @param str - input string_view to be modified.
 * @return A string with the first letter capitalized.
 */
static inline std::string capitalize_first_char(std::string_view str) {
    if (str.empty()) {
        return "";
    }
    std::string result(str);
    result.front() = static_cast<char>(std::toupper(result.front()));
    return result;
}

/**
 * @brief Checks if input std::string_view str contains specified substring.
 * @param str - std::string_view to be checked.
 * @param substring - searched substring.
 * @return True if substring was found in str, false otherwise.
 */
static inline bool contains(std::string_view str, std::string_view substring) {
    return str.find(substring) != std::string_view::npos;
}

/**
 * @brief Checks if input std::string_view str contains specified character.
 * @param str - std::string_view to be checked.
 * @param character - searched character.
 * @return True if character was found in str, false otherwise.
 */
static inline bool contains(std::string_view str, const char character) {
    return str.find(character) != std::string_view::npos;
}

/**
 * @brief Compares two std::string_views ignoring their case.
 *        This C++17 version avoids creating new strings, making it more
 * efficient.
 * @param str1 - std::string_view to compare
 * @param str2 - std::string_view to compare
 * @return True if str1 and str2 are equal ignoring case, false otherwise.
 */
static inline bool compare_ignore_case(std::string_view str1,
                                       std::string_view str2) {
    return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(),
                      [](char a, char b) {
                          return std::tolower(static_cast<unsigned char>(a)) ==
                                 std::tolower(static_cast<unsigned char>(b));
                      });
}

/**
 * @brief Trims (in-place) white spaces from the left side of std::string.
 * @param str - input std::string to remove white spaces from.
 */
static inline void trim_left(std::string &str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
                  return !std::isspace(ch);
              }));
}

/**
 * @brief Trims (in-place) white spaces from the right side of std::string.
 * @param str - input std::string to remove white spaces from.
 */
static inline void trim_right(std::string &str) {
    str.erase(std::find_if(str.rbegin(), str.rend(),
                           [](int ch) { return !std::isspace(ch); })
                  .base(),
              str.end());
}

/**
 * @brief Trims (in-place) white spaces from the both sides of std::string.
 * @param str - input std::string to remove white spaces from.
 */
static inline void trim(std::string &str) {
    trim_left(str);
    trim_right(str);
}

/**
 * @brief Trims white spaces from the left side of a string_view and returns a
 * new string.
 * @param str - input std::string_view to remove white spaces from.
 * @return Copy of input str with trimmed white spaces.
 */
static inline std::string trim_left_copy(std::string_view str) {
    auto first = std::find_if(str.begin(), str.end(),
                              [](int ch) { return !std::isspace(ch); });
    return std::string(first, str.end());
}

/**
 * @brief Trims white spaces from the right side of a string_view and returns a
 * new string.
 * @param str - input std::string_view to remove white spaces from.
 * @return Copy of input str with trimmed white spaces.
 */
static inline std::string trim_right_copy(std::string_view str) {
    auto last = std::find_if(str.rbegin(), str.rend(),
                             [](int ch) { return !std::isspace(ch); });
    return std::string(str.begin(), last.base());
}

/**
 * @brief Trims white spaces from both sides of a string_view and returns a new
 * string.
 * @param str - input std::string_view to remove white spaces from.
 * @return Copy of input str with trimmed white spaces.
 */
static inline std::string trim_copy(std::string_view str) {
    auto first = std::find_if(str.begin(), str.end(),
                              [](int ch) { return !std::isspace(ch); });
    auto last = std::find_if(str.rbegin(), str.rend(),
                             [](int ch) { return !std::isspace(ch); });
    return (last.base() < first) ? "" : std::string(first, last.base());
}

/**
 * @brief Replaces (in-place) the first occurrence of target with replacement.
 * @param str - input std::string that will be modified.
 * @param target - substring that will be replaced.
 * @param replacement - substring that will replace target.
 * @return True if replacement was successful, false otherwise.
 */
static inline bool replace_first(std::string &str, std::string_view target,
                                 std::string_view replacement) {
    const size_t start_pos = str.find(target);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, target.length(), replacement);
    return true;
}

/**
 * @brief Replaces (in-place) last occurrence of target with replacement.
 * @param str - input std::string that will be modified.
 * @param target - substring that will be replaced.
 * @param replacement - substring that will replace target.
 * @return True if replacement was successful, false otherwise.
 */
static inline bool replace_last(std::string &str, std::string_view target,
                                std::string_view replacement) {
    size_t start_pos = str.rfind(target);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, target.length(), replacement);
    return true;
}

/**
 * @brief Replaces (in-place) all occurrences of target with replacement.
 * @param str - input std::string that will be modified.
 * @param target - substring that will be replaced.
 * @param replacement - substring that will replace target.
 * @return True if any replacement was successful, false otherwise.
 */
static inline bool replace_all(std::string &str, std::string_view target,
                               std::string_view replacement) {
    if (target.empty()) {
        return false;
    }
    size_t start_pos = 0;
    bool found_substring = false;
    while ((start_pos = str.find(target, start_pos)) != std::string::npos) {
        str.replace(start_pos, target.length(), replacement);
        start_pos += replacement.length();
        found_substring = true;
    }
    return found_substring;
}

/**
 * @brief Checks if std::string_view str ends with specified suffix.
 * @param str - input std::string_view that will be checked.
 * @param suffix - searched suffix in str.
 * @return True if suffix was found, false otherwise.
 */
static inline bool ends_with(std::string_view str, std::string_view suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

/**
 * @brief Checks if std::string_view str ends with specified character.
 * @param str - input std::string_view that will be checked.
 * @param suffix - searched character in str.
 * @return True if ends with character, false otherwise.
 */
static inline bool ends_with(std::string_view str, const char suffix) {
    return !str.empty() && (str.back() == suffix);
}

/**
 * @brief Checks if std::string_view str starts with specified prefix.
 * @param str - input std::string_view that will be checked.
 * @param prefix - searched prefix in str.
 * @return True if prefix was found, false otherwise.
 */
static inline bool starts_with(std::string_view str, std::string_view prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

/**
 * @brief Checks if std::string_view str starts with specified character.
 * @param str - input std::string_view that will be checked.
 * @param prefix - searched character in str.
 * @return True if starts with character, false otherwise.
 */
static inline bool starts_with(std::string_view str, const char prefix) {
    return !str.empty() && (str.front() == prefix);
}

/**
 * @brief Splits input std::string_view str according to input delim.
 * @param str - std::string_view that will be splitted.
 * @param delim - the delimiter.
 * @return std::vector<std::string> that contains all splitted tokens.
 */
static inline std::vector<std::string> split(std::string_view str,
                                             const char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::string temp_str(str); // istringstream works with std::string
    std::stringstream ss(temp_str);

    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }

    // Match semantics of split(str,str)
    if (str.empty() || ends_with(str, delim)) {
        tokens.emplace_back();
    }

    return tokens;
}

/**
 * @brief Splits input std::string_view str according to input std::string_view
 * delim.
 * @param str - std::string_view that will be split.
 * @param delim - the delimiter.
 * @return std::vector<std::string> that contains all splitted tokens.
 */
static inline std::vector<std::string> split(std::string_view str,
                                             std::string_view delim) {
    size_t pos_start = 0, pos_end;
    std::vector<std::string> tokens;

    while ((pos_end = str.find(delim, pos_start)) != std::string_view::npos) {
        tokens.emplace_back(str.substr(pos_start, pos_end - pos_start));
        pos_start = pos_end + delim.length();
    }

    tokens.emplace_back(str.substr(pos_start));
    return tokens;
}

/**
 * @brief Splits input string using any delimiter in the given set.
 * @param str - std::string_view that will be split.
 * @param delims - the set of delimiter characters.
 * @return vector of resulting tokens.
 */
static inline std::vector<std::string> split_any(std::string_view str,
                                                 std::string_view delims) {
    std::vector<std::string> tokens;
    size_t pos_start = 0;
    for (size_t pos_end = 0; pos_end < str.length(); ++pos_end) {
        if (contains(delims, str[pos_end])) {
            tokens.emplace_back(str.substr(pos_start, pos_end - pos_start));
            pos_start = pos_end + 1;
        }
    }
    tokens.emplace_back(str.substr(pos_start));
    return tokens;
}

/**
 * @brief Joins all elements of a container of arbitrary datatypes
 *        into one std::string with delimiter delim.
 * @tparam Container - container type.
 * @param tokens - container of tokens.
 * @param delim - the delimiter.
 * @return std::string with joined elements of container tokens with delimiter
 * delim.
 */
template <typename Container>
static inline std::string join(const Container &tokens,
                               std::string_view delim) {
    std::ostringstream result;
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        if (it != tokens.begin()) {
            result << delim;
        }
        result << *it;
    }
    return result.str();
}

/**
 * @brief Inplace removal of all empty strings in a container of strings.
 *        Uses the erase-remove idiom, compatible with C++17.
 * @tparam Container - container type.
 * @param tokens - container of strings.
 */
template <template <typename, typename...> typename Container, typename... Args>
static inline void drop_empty(Container<std::string, Args...> &tokens) {
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                [](const std::string &s) { return s.empty(); }),
                 tokens.end());
}

/**
 * @brief Removes all empty strings from a copy of a container of strings.
 * @tparam Container - container type.
 * @param tokens - container of strings.
 * @return container of non-empty tokens.
 */
template <template <typename, typename...> typename Container, typename... Args>
static inline Container<std::string, Args...>
drop_empty_copy(Container<std::string, Args...> tokens) {
    drop_empty(tokens);
    return tokens;
}

/**
 * @brief Inplace removal of all duplicate items in a vector where order is not
 * maintained. Taken from: C++ Primer V5
 * @tparam T - arbitrary datatype.
 * @param tokens - vector of items.
 */
template <typename T>
static inline void drop_duplicate(std::vector<T> &tokens) {
    std::sort(tokens.begin(), tokens.end());
    auto end_unique = std::unique(tokens.begin(), tokens.end());
    tokens.erase(end_unique, tokens.end());
}

/**
 * @brief Removal of all duplicate items in a vector where order is not
 * maintained. Taken from: C++ Primer V5
 * @tparam T - arbitrary datatype.
 * @param tokens - vector of items.
 * @return vector of non-duplicate items.
 */
template <typename T>
static inline std::vector<T> drop_duplicate_copy(std::vector<T> tokens) {
    drop_duplicate(tokens);
    return tokens;
}

/**
 * @brief Creates new std::string with repeated n times substring str.
 * @param str - substring that needs to be repeated.
 * @param n - number of iterations.
 * @return std::string with repeated substring str.
 */
static inline std::string repeat(std::string_view str, unsigned n) {
    std::string result;
    result.reserve(str.length() * n);
    for (unsigned i = 0; i < n; ++i) {
        result.append(str);
    }
    return result;
}

/**
 * @brief Creates new std::string with repeated n times char c.
 * @param c - char that needs to be repeated.
 * @param n - number of iterations.
 * @return std::string with repeated char c.
 */
static inline std::string repeat(char c, unsigned n) {
    return std::string(n, c);
}

/**
 * @brief Sort input std::vector<T> strs in ascending order.
 * @param strs - std::vector<T> to be sorted.
 */
template <typename T>
static inline void sorting_ascending(std::vector<T> &strs) {
    std::sort(strs.begin(), strs.end());
}

/**
 * @brief Sort input std::vector<T> strs in descending order.
 * @param strs - std::vector<T> to be sorted.
 */
template <typename T>
static inline void sorting_descending(std::vector<T> &strs) {
    std::sort(strs.begin(), strs.end(), std::greater<T>());
}

/**
 * @brief Reverse input container strs in-place.
 * @param strs - container to be reversed.
 */
template <typename Container>
static inline void reverse_inplace(Container &strs) {
    std::reverse(strs.begin(), strs.end());
}

/**
 * @brief Returns a reversed copy of the input container.
 * @param strs - container to be reversed.
 * @return A reversed copy of the container.
 */
template <typename Container>
static inline Container reverse_copy(Container strs) {
    std::reverse(strs.begin(), strs.end());
    return strs;
}
} // namespace strutil