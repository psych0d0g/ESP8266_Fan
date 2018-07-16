#ifndef header_h
#define header_h

// Wifi Hostname
const char* host = "PhilipsFan";

TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, +120};    // Daylight time = UTC + 2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, +60}; // Standard time = UTC +1 hours

int ntp_interval = 5000;

long autooff_time = 0;

boolean triggered = false;

long lastTimeUpdate;
//Pin assignments
const uint8_t GPIOPIN[4] = {D5,D6,D7,D2};  // Fan buttons

#endif