#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#include <string>
#include <vector>

const std::string whitespaces_std (" \t\n\r");

inline std::string strip_comments (const std::string &line) {
	return line.substr (0, line.find ('#'));
}

inline std::string strip_whitespaces (const std::string &line, std::string whitespaces = whitespaces_std) {
	std::string result (line);
	if (result.find_first_of (whitespaces) != std::string::npos) {
		result = result.substr (result.find_first_not_of (whitespaces), result.size());
	}

	while (whitespaces.find (result[result.size() - 1]) != std::string::npos) {
		result = result.substr (0, result.size() - 1);
	}
	return result;
}

inline std::string tolower (const std::string &line) {
	std::string result (line);
	for (int i = 0; i < line.size(); i++) 
		result[i] = tolower(result[i]);

	return result;
}

inline std::string trim_line (const std::string &line) {
	return tolower (strip_whitespaces (strip_comments (line)));
}

inline std::vector<std::string> tokenize (const std::string &line_in, std::string delimiter=whitespaces_std) {
	std::vector<std::string> result;
	std::string line = line_in;

	while (line.find_first_of (delimiter) != std::string::npos) {
		std::string token = line.substr (0, line.find_first_of (delimiter));
		line = line.substr (token.size() + 1, line.size());
		if (token.size() > 0)
			result.push_back (token);
	}

	if (line.size() > 0)
		result.push_back (line);

	return result;
}

/** Counts the number of occurrences of a list of characters.
 *
 * \param line_in The hay-stack to search for.
 * \param characters The needles.
 * \return The sum of occurrences of all needles found in the hay-stack.
 */
inline int count_char (const std::string &line_in, const std::string characters) {
	int count = 0;
	size_t index = 0;
	size_t char_pos = line_in.find_first_of (characters, index);

	while (char_pos != std::string::npos) {
		index = char_pos + 1;
		count ++;
		char_pos = line_in.find_first_of (characters, index) ;
	}

	return count;
}

#endif