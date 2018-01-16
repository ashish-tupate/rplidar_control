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
#include <fstream>					//Tom's additions for writing to file
#include <iostream>
#include <math.h>
#include "write_line.h"				//Writing line of lidar data to file
#include "get_datetime.h"			//Retrieving the system date/time string for filename

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

	//////////////////////////////////////////////////////////////
    // TOM EDITS TO SAVE DATA
	float angle;
	float distance;
	float quality;
	bool write_to_file = true;
	std::ofstream outf;

	// ---------------------------------------------------------------
	// Make new directory for lidar data files, based on system date
	//std::string file_path("c:\\users\\tw9616\\documents\\phd\\ee placement\\lidar\\rplidar_a2m6\\vc2017 test\\sample.dat");

	std::string data_path("./Data/");					// String holding directory path name
	std::string file_path;
	std::string file_name = get_datetime() + ".dat";	// File name definition
	file_path = data_path + file_name;					// Path to file
	
	boost::filesystem::path data_dir(data_path);		// Object holding directory for saving data
	if (boost::filesystem::create_directory(data_dir))	// Create directory if not already present
	{
		std::cerr << "Directory Created: " << data_path << std::endl;
	}
	// ---------------------------------------------------------------

    if (write_to_file)  // Open file where data is written and add header
		{
			//using namespace std;
			outf.open(file_path, std::ios::binary);
			outf << "distance(mm)\tangle(degrees)\tquality\n";
			//outf.close();
		}

	float holder_int;			// Holder for the integer part
	unsigned short angle_int;	// Interger part of angle reading
	unsigned short angle_dec;	// Fractional (decimal) part of angle reading
	unsigned char quality_byte;	// One byte for quality data
	////////////////////////////////////////////////////////////////

	// fetech result and print it out...
	while (1) {
        rplidar_response_measurement_node_t nodes[360*2];
        size_t   count = _countof(nodes);

        op_result = drv->grabScanData(nodes, count);

        if (IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
			
            for (int pos = 0; pos < (int)count ; ++pos) {
				angle = (nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f;
				distance = nodes[pos].distance_q2 / 4.0f;
				quality = nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;

				quality_byte = static_cast<unsigned char>(quality);	// Cast quality to a 1 byte integer

				// Maybe just leave angle as float rather than messing around with q9 conversion
				// I was going to have a 9-bit integer and 15-bit decimal part, to use 3 bytes
				// But might as well just use 4 bytes as floatand save like this
				//angle_dec =	// Want to turn the decimal part into an integer
				//angle_int = static_cast<unsigned short>(holder_int);	// Take integer part of angle


				//if (write_to_file) {write_line(outf, distance, angle, quality);}  // Send data to be written
				write_line(outf, distance, angle, quality_byte);	// Old version

				/*	if (my_angle > 100 && my_angle < 101)
				{
					printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
						(nodes[pos].sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ? "S " : "  ",
						(nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f,
						nodes[pos].distance_q2 / 4.0f,
						nodes[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
				}*/
            }

			// Closing file where data is written
			if (write_to_file) {
				/*outf.close();
				write_to_file = false;*/
				outf.flush();  // Flush buffer to file after each full scan
			}
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

