// Simple function to write a line to a file when given three arguments
// This write function is designed specifically for RPLIDAR recording
#include <iostream>
#include <fstream>
#include "file_io.h"
#include <string>

using namespace std;

//void write_line(ofstream &file_ID, double distance, double angle, int quality)
void write_line(ofstream &file_ID, unsigned short distance, float angle, char quality)
{
	if (!file_ID)
	{
		cout << "ERROR WRITING TO FILE!!!" << endl;
		//return FileState::FILE_FAIL;
	}
	file_ID.write((char*)&distance, sizeof(distance));
	file_ID.write((char*)&angle, sizeof(angle));
	file_ID.write((char*)&quality, sizeof(quality));
	
	//file_ID << distance << "\t" << angle << "\t" << quality << "\n";
	//return FileState::FILE_SUCCESS;
}

void write_line_ASCII(ofstream &file_ID, unsigned short distance, float angle, char quality)
{
	if (!file_ID)
	{
		cout << "ERROR WRITING TO FILE!!!" << endl;
		//return FileState::FILE_FAIL;
	}

	file_ID << distance << "\t" << angle << "\t" << static_cast<int>(quality) << "\r\n";
	//return FileState::FILE_SUCCESS;
}

int read_network_file()
{
	string port_identifier("port=");				// Identifier that precedes port number in file
	string file_path("./network/network_LSP.cfg");	// File path for socket connection
	ifstream net_file(file_path);					// Create file object

	if (!net_file) { return FileState::FILE_FAIL; }

	// Read line of file which will contain port number
	string port_line;
	getline(net_file, port_line);

	// Find port identifier in line and extract the port number
	size_t pos = port_line.find(port_identifier);
	string port_str;
	port_str = port_line.replace(pos, port_identifier.length(), "");

	// Convert port number to int
	int port;
	port = stoi(port_str);

	return port;
}