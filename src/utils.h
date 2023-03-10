#pragma once

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>


std::string readFile(const char* filename);

bool fileExists(const char* filename);

#endif
