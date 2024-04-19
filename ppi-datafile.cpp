/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <vector>
#include <functional>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <map>

#include "ppi-display.h"
#include "ppi-datafile.h"
#include "geoutils.h"


// the name of the file containing the list of points for the region polyline.
static const char *REGIONMAP_FILENAME = "./region-outline.txt";
// the name of file containing the airline code database.
static const char *AIRLINES_FILENAME = "./airlines.txt";
// the name of the file containing the points of interest.
static const char *POI_FILENAME = "./poi.txt";

// The polyline which shows the coastline or something to improve the display context.
static std::vector<XY> g_regionMap;

// The database of airline codes to human names.
static std::map<std::string, std::vector<std::string>> g_airlines;

// The X position in map coordinates of the receiving station.
double g_homeEasting = 0.0;
// The Y position in map coordinates of the receiving station.
double g_homeNorthing = 0.0;


// The map points of interest, typically airfields or beacone to improve the display context.
static std::vector<POI> g_poi;

const std::vector<XY>& GetRegionOutline()
{
  return g_regionMap;
}


const std::vector<POI>& GetPOI()
{
  return g_poi;
}

const char *GetAirlineFromCode(const std::string& code)
{
  auto itr = g_airlines.find(code);
  if(itr != g_airlines.end())
    return itr->second[0].c_str();
  else
    return "";
}

// Loads the polyline which shows the coastline or other static map line.
bool LoadRegionMap()
{
  std::ifstream infile(REGIONMAP_FILENAME);
  if(!infile) {
    printf("Failed to open '%s'\n", REGIONMAP_FILENAME);
    return false;
  }

  for(std::string line; std::getline(infile, line);) {
    double lat, lon;
    sscanf(line.c_str(), "%lf, %lf", &lat, &lon);
    double east, north;
    ConvertCoordinate(lat, lon, 0, &east, &north, nullptr);
    XY xy;
    MapToScreen(east, north, xy.x, xy.y);
    g_regionMap.push_back(xy);
  }

  printf("Read %i points for region map\n", int(g_regionMap.size()));
  return (g_regionMap.size() > 0);
}

/// Loads a text file, splits each line into records and then callback to perform work on that data. 
bool LoadTextData(const std::string& filename, char delimiter, int numField, std::function<void(const std::vector<std::string>&)> func)
{
  std::ifstream infile(filename);
  if(!infile) {
    printf("Failed to open '%s'\n", filename.c_str());
    return false;
  }

  std::vector<std::string> record(numField);

  for(std::string line; std::getline(infile, line); ) {
    std::stringstream strm;
    strm << line;
    for(int n = 0; n<numField; n++) {
      std::string dataItem;
      std::getline(strm, dataItem, delimiter);
      record[n] = dataItem;
    }
    func(record);
  }
  return true;
}

/// Loads the airline codes database.
bool LoadAirlines()
{
  LoadTextData(AIRLINES_FILENAME, '|', 3, [](const std::vector<std::string>& record) {
    g_airlines[record[0]] = {record[1], record[2]};
  });

  if(g_airlines.size() == 0) {
    printf("Failed to read any airline data\n");
    return false;
  }
  return true;
}

/// Loads the point of interest. Note that the first item is the receiving station and this position is used to
/// position the map display and ranging rings.
bool LoadPOI()
{
  LoadTextData(POI_FILENAME, '|', 3, [](const std::vector<std::string>& record) {
    const double lat = atof(record[1].c_str());
    const double lon = atof(record[2].c_str());
    double easting, northing;
    ConvertCoordinate(lat, lon, 0.0, &easting, &northing, nullptr);

    // the first record is the receiving station position.
    if(g_poi.size() == 0) {
      g_homeEasting = easting;
      g_homeNorthing = northing; 
      g_regionWidthX = RegionSizeMiles / MetersToMiles;
      g_regionWidthY = g_regionWidthX;    
      g_regionMinX = easting - (g_regionWidthX * 0.5);
      g_regionMinY = northing - (g_regionWidthY * 0.5);
    }

    XY pos;
    MapToScreen(easting, northing, pos.x, pos.y);    
    g_poi.push_back({record[0], pos});
  });

  if(g_poi.size() < 1) {
    printf("'%s' should contain at least one point\n", POI_FILENAME);
    return false;
  }
  return true;
}

bool LoadPPIDataFiles()
{
   if(!LoadPOI())
    return false;
    
  if(!LoadAirlines())
    return false;
    
  if(!LoadRegionMap())
    return false;

  printf("Loaded files\n");

  return true;
}
