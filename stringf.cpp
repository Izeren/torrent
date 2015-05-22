#include "stringf.h"

const char *WHITE_SPACES = " \t\n";

bool is_white_space(char c) {
	return (c == ' ' || c == '\t' || c == '\n');
}

void del_extra_white_spaces(std::string &s) {
	int left = s.find_first_not_of(WHITE_SPACES);
	int right = s.find_last_not_of(WHITE_SPACES);
	int len = right - left + 1;
	if (len == 0) {
		s = "";
		return;
	}
	else {
		s = s.substr(left, len);
		std::string *s1 = new std::string;
		s1->push_back(s[0]);
		for (int i = 1; i < len; ++i) {
			if (!(is_white_space(s[i]) && is_white_space(s[i - 1]))) {
				(*s1) += s[i];
			}
		}
		s = *s1;
		delete s1;
	}
}

void parse_by_white_spaces(std::string &s, list &arr) {
	int current_position = 0;
	int new_position = s.find_first_of(WHITE_SPACES, current_position);
	while (new_position != std::string::npos) {

		int cur_len = new_position - current_position;
		arr.push_back(s.substr(current_position, cur_len));
		current_position = new_position + 1;
		new_position = s.find_first_of(WHITE_SPACES, current_position);

	}
	arr.push_back(s.substr(current_position));
}


void print_list(const list &li, std::ostream &out) {
	int size = li.size();
	out << "Number of words: " << size << "\n";
	if (size == 0) {
		return;
	}
	for (int i = 0; i < size; ++i) {
		out << "$" << li[i] << "$\n";
	}
	out << "__end_of_dict__\n";
}

int find_word(const list &li, std::string &word, int position) {
	int size = li.size();
	if (size == 0) {
		return -1;
	}
	else {
		for (int i = position; i < size; ++i) {
			if (li[i] == word) {
				return i;
			}
		}
		return -1;
	}
}

void parse_list_by_word(list_arr_t &li_arr, list &li, std::string &word) {
	int current_position = 0;
	int new_position = find_word(li, word, current_position);
	while (new_position != -1) {
		li_arr.push_back(list(li.begin() + current_position, li.begin() + new_position));
		current_position = new_position + 1;
		new_position = find_word(li, word, current_position);
	}
	li_arr.push_back(list(li.begin() + current_position, li.end()));
}

void write_str(int fd, std::string str) {
	int len = str.size();
	char *buffer = new char[len];
	for (int i = 0; i < len; ++i) {
		buffer[i] = str[i];
	}

	write(fd, buffer, len);

	delete []buffer;
}

std::string itos(int Number) {
	std::string result;
	While (Number) {
		result += Number % 10;
		Number /= 10;
	}
	std::reverse(result.begin(), result.end());
	return result;
}