/*
 *  RPLIDAR
 *  Ultra Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2016 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header

// ---------------------------------------------------------------------------------
// Tom's Additions
#include <fstream>				//Tom's additions for writing to file
#include <iostream>
#include <math.h>
#include "file_io.h"			//Read/write functions
#include "get_datetime.h"		//Retrieving the system date/time string for filename
#include "socket.h"				//Socket access functions

// Use Boost library
// In project properties >> Linker >> General >> Additional Library Directories 
// add ;C:\Program Files\Boost\boost_1_66_0\stage\lib\;
// C/C++ >> General >> Additional Library Directories
// add ;C:\Program Files\Boost\boost_1_66_0\;
// Built filesystem requirement using ".\b2 runtime-link=static --with-filesystem" in boost directory
#include "boost\filesystem.hpp"		
// --------------------------------------------------------------------------------------




#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace rp::standalone::rplidar;

bool checkRPLIDARHealth(RPlidarDriver * drv)
{
    u_result     op_result;
    rplidar_response_device_health_t healthinfo;


    op_result = drv->getHealth(healthinfo);
    if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        printf("RPLidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, rplidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want rplidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

#include <signal.h>
bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}

int main(int argc, const char * argv[]) {
	


	const char * opt_com_path = NULL;
    _u32         opt_com_baudrate = 115200;
    u_result     op_result;

	short motor_pwm = 660;  // Define pwm to set motor

    //printf("Ultra simple LIDAR data grabber for RPLIDAR.\n"
           //"Version: "RPLIDAR_SDK_VERSION"\n");

    // read serial port from the command line...
    if (argc>1) opt_com_path = argv[1]; // or set to a fixed value: e.g. "com3" 

    // read baud rate from the command line if specified...
    if (argc>2) opt_com_baudrate = strtoul(argv[2], NULL, 10);


    if (!opt_com_path) {
#ifdef _WIN32
        // use default com port
        opt_com_path = "\\\\.\\com3";
#else
        opt_com_path = "/dev/ttyUSB0";
#endif
    }

    // create the driver instance
    RPlidarDriver * drv = RPlidarDriver::CreateDriver(RPlidarDriver::DRIVER_TYPE_SERIALPORT);
    
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }


    // make connection...
    if (IS_FAIL(drv->connect(opt_com_path, opt_com_baudrate))) {
        fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
            , opt_com_path);
		RPlidarDriver::DisposeDriver(drv);
		return 0;
        //goto on_finished;	// Don't like goto so have replaced with the above 2 lines which finish the script
    }

    rplidar_response_device_info_t devinfo;

	// retrieving the device info
    ////////////////////////////////////////
    op_result = drv->getDeviceInfo(devinfo);

    if (IS_FAIL(op_result)) {
        fprintf(stderr, "Error, cannot get device info.\n");
		RPlidarDriver::DisposeDriver(drv);
		return 0;
        //goto on_finished;  // Don't like goto so have replaced with the above 2 lines which finish the script
    }

    // print out the device serial number, firmware and hardware version number..
    printf("RPLIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);



    // check health...
    if (!checkRPLIDARHealth(drv)) {
		RPlidarDriver::DisposeDriver(drv);
		return 0;
        //goto on_finished;	// Don't like goto so have replaced with the above 2 lines which finish the script
    }

	signal(SIGINT, ctrlc);
    
	drv->startMotor();
    // start scan...
    drv->startScan();

	// Set motor PWM
	drv->setMotorPWM(motor_pwm);

	float frequency = 0;		// Frequency value - gets edited by getFrequency()
	bool is4kmode = true;		// Let it know if it is in 4k mode
	bool inExpressMode = true;	// Whether we are uses express mode of not


	//////////////////////////////////////////////////////////////
    // TOM EDITS TO SAVE DATA
	short no_of_scans = 100;		// Number of scans saved to a singe file
	float angle;
	unsigned short distance;
	float quality;
	unsigned char quality_byte;		// One byte for quality data
	bool write_to_file = true;
	std::ofstream outf;
	std::ofstream outf_ASCII;

	// Make new directory for lidar data files, based on system date
	//std::string data_path("./Data/");					// String holding directory path name
	std::string date_time = get_datetime();				// Get date_time string
	std::string data_path;
	data_path = "./" + date_time.substr(0, 10) + "/";	// Date directory
	std::string file_path;
	std::string file_path_ASCII;

	boost::filesystem::path data_dir(data_path);		// Object holding directory for saving data
	if (boost::filesystem::create_directory(data_dir))	// Create directory if not already present
	{
		std::cerr << "Directory Created: " << data_path << std::endl;
	}

	// Socket setup
	int port;
	SOCKET sock;
	
	port = read_network_file();		// Get port 
	sock = open_sock(port);			// Open socket - connect to server
	printf("Opened socket!\n");
	////////////////////////////////////////////////////////////////

	// fetech result and print it out...
	while (1) {

		// -------------------------------------------------------------------------------------------
		// Open new file for new scan
		std::string file_name = get_datetime() + ".dat";	// File name definition
		file_path = data_path + file_name;					// Path to file

		// ASCII tests
		std::string file_name_ASCII = get_datetime() + "_ASCII.dat";
		file_path_ASCII = data_path + file_name_ASCII;

		// Open file where data is written and add header
		if (write_to_file)  
			{
				//using namespace std;
				outf.open(file_path, std::ios::binary);
				outf << "distance(mm)_" << sizeof(distance) << "B_\tangle(degrees)_" << sizeof(angle) << "B_\tquality_" << sizeof(quality_byte) << "B_\n";
				//outf.close();

				// ASCII TESTS
				outf_ASCII.open(file_path_ASCII);
				outf_ASCII << "distance(mm)_" << sizeof(distance) << "B_\tangle(degrees)_" << sizeof(angle) << "B_\tquality_" << sizeof(quality_byte) << "B_\n";
			}
		// ---------------------------------------------------------------

		
		// Take 10 seconds of scans (or 100 scans as is set up) and save to file
		for (int scan_num = 0; scan_num < no_of_scans; scan_num++) {
			rplidar_response_measurement_node_t nodes[360*2];
			size_t   count = _countof(nodes);

			op_result = drv->grabScanData(nodes, count);

			if (IS_OK(op_result)) {
				drv->ascendScanData(nodes, count);

				for (int pos = 0; pos < (int)count; ++pos) {
					angle = (nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f;
					distance = static_cast<unsigned short>(nodes[pos].distance_q2 / 4.0f);
					quality = nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;

					quality_byte = static_cast<unsigned char>(quality);	// Cast quality to a 1 byte integer

					//write_line(outf, distance, angle, quality_byte);				// Write data
					//write_line_ASCII(outf_ASCII, distance, angle, quality_byte);	// Write data

					// Socket send data
					send_data(sock, distance, angle, quality_byte);

					/*	if (my_angle > 100 && my_angle < 101)
					{
						printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
							(nodes[pos].sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ? "S " : "  ",
							(nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f,
							nodes[pos].distance_q2 / 4.0f,
							nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
					}*/
				}

				
			}

        }

		// Closing file where data is written
		if (write_to_file) {
			outf.close();  // Close file after 100 scans have been taken
			outf_ASCII.close();
		}

        if (ctrl_c_pressed){ 
			break;
		}
    }

    drv->stop();
    drv->stopMotor();
    // done!
//on_finished:	// Don't like goto so this is new defunct
    RPlidarDriver::DisposeDriver(drv);
    return 0;
}

