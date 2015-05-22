#ifndef STRINGF
#define STRINGF

#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

typedef std::vector<std::string> list;
typedef std::vector<list> list_arr_t;

void write_str(int fd, std::string str);
// void write_str(int fd, const char *buffer, int size);

void del_extra_white_spaces(std::string &s);
void parse_by_white_spaces(std::string &s, list &arr);

void print_list(const list &li, std::ostream &out);

int find_word(const list &li, std::string &word, int position = 0);

void parse_list_by_word(list_arr_t &li_arr, list &li, std::string &word);

std::string itos(int Number);

#endif //STRINGF