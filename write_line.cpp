// Simple function to write a line to a file when given three arguments
// This write function is designed specifically for RPLIDAR recording
#include <iostream>
#include <fstream>
#include "write_line.h"

using namespace std;

//void write_line(ofstream &file_ID, double distance, double angle, int quality)
void write_line(ofstream &file_ID, float distance, float angle, char quality)
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