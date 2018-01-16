// Get date and time and return it as a string
#include <ctime>	// For time
#include <string>	// For string manipulation
#include <sstream>	// For conversion to string

using namespace std;

string date_time_str;		// Return string

short years_since = 1900;  // CTime quotes since 1900 so add this to year for real year
string year;
string month;
string day;
string hour;
string minute;
string second;


string get_datetime() {
	// Get current date/time
	time_t now = time(0);

	// Convert to string
	char* dt = ctime(&now);

	//Convert now to tm struct for UTC
	tm *gmtm = gmtime(&now);

	// Extract year
	year = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_year + years_since))->str();

	// Extract month and ensure it has a length of 2
	month = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_mon + 1))->str();
	if (month.length() < 2) { month = "0" + month; }

	// Extract day and ensure it has a length of 2
	day = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_mday))->str();
	if (day.length() < 2) { day = "0" + day; }

	// Extract hour 
	hour = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_hour))->str();
	if (hour.length() < 2) { hour = "0" + hour; }

	// Extract minute
	minute = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_min))->str();
	if (minute.length() < 2) { minute = "0" + minute; }

	// Extract minute
	second = static_cast<ostringstream*>(&(ostringstream() << gmtm->tm_sec))->str();
	if (second.length() < 2) { second = "0" + second; }
	
	// Concatenate final return string
	date_time_str = year + "-" + month + "-" + day + "_T" + hour + minute + second;

	return date_time_str;
}