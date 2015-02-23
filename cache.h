/*
* File name: cache.h
* File abstract: this is a programm to simulate cache coherent, 
*                this file defines the references to standard libraries,
*                global variables, structs, and public functions
* 
* Version: 1.0
* Author: Xiaoming Sun
* Date: 2014-04-27
*/

/*
* define cache heads and avoid duplicated defination
*/
#ifndef Cache_H
#define Cache_H

/*
* define references to standard head files
*/
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

enum unit_size{B,KB,MB,GB}; // enum type for unit of size
const char *UnitSizeNames[] = {"B", "KB", "MB", "GB"}; // name array for unit of size
enum unit_time{us,ns}; // enum type for unit of time
const char *UnitTimeNames[] = {"us", "ns"}; // name array for unit of time

/*
* struct size
*	data is a decimal number for size
* 	unit is enum type for unit, like bytes, megabytes
*/
struct obj_size
{
       unsigned long data;
       unit_size unit; 
};

/*
* struct size
*	data is a decimal number for time
* 	unit is enum type for unit, like us, ns
*/
struct obj_time
{
       unsigned long data;
       unit_time unit;
};

/* 
* This function is to set memory size
*	size is a struct size with number and unit
*/
void memorySize(obj_size size);

/* 
* This function is to set the number of chips, it is optional for 1 chip
*	number is int to describe the number of chip
*/
void numOfChips(int number);

/* 
* This function is to set the number of cores for each chip
*   chipID is the ID of chip to set, if no chipID, the same number of cores for all chips
*	number is int to describe the number of chip
*/
void numOfCores(int chipID, int number);

/* 
* This function is to set cache line size
*   size is a struct size with number and unit
*/
void cacheLineSize(obj_size size);

/* 
* This function is to set cache size for each chip
*   chipID is the ID of chip to be set, if no chipID, it is L2 for 1 chip, it is L3 for multiple chips
*   size is a struct size with number and unit
*/
void cacheSize(int chipID, obj_size size);

/* 
* This function is to set cache access speed for each chip
*   chipID is the ID of chip to be set, if no chipID, it is L2 for 1 chip, it is L3 for multiple chips
*   time is a struct time with number and unit
*/
void cacheAccessSpeed(int chipID, obj_time time);

/* 
* This function is to set cache replacement speed
*   time is a struct time with number and unit
*/
void replacementSpeed(obj_time time);

/* 
* This function is to set broadcast speed
*   time is a struct time with number and unit
*/
void broadcastSpeed(obj_time time);

/* 
* This function is to set meory access speed
*   time is a struct time with number and unit
*/
void memoryAccessSpeed(obj_time time);

/* 
* This function is to read data from specified address
*   chipID is the ID of chip which is sending the request
*   coreID is the ID of core which is sending the request
*   address is an address of memory
*   size is a struct size with number and unit
*/
void read(int chipID, int coreID, string address, obj_size size);

/* 
* This function is to write data to specified address
*   chipID is the ID of chip which is sending the request
*   coreID is the ID of core which is sending the request
*   address is an address of memory
*   size is a struct size with number and unit
*/
void write(int chipID, int coreID, string address, obj_size size);

#endif
