#pragma once
#include <fstream>

enum FileState {
	FILE_SUCCESS = 0,
	FILE_FAIL = -1,
};

//void write_line(std::ofstream &file_ID, double distance, double angle, int quality);
void write_line(std::ofstream &file_ID, unsigned short distance, float angle, char quality);
void write_line_ASCII(std::ofstream &file_ID, unsigned short distance, float angle, char quality);
int read_network_file();