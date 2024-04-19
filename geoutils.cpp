/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <proj.h>

#include "geoutils.h"

class CoordinateConverter {
public:
  CoordinateConverter(const char *srcCRS, const char *dstCRS);
  ~CoordinateConverter();
  
  void convertCoordinate(const double lat, const double lon, const double altitude, double *northing, double *easting, double *altOut);
  
  bool isValid() const { return m_proj != nullptr; }
  
protected:
  PJ_CONTEXT *m_ctx;
  PJ *m_proj;
};

const char *BNG = "+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=400000 +y_0=-100000 +ellps=airy +units=m +no_defs +type=crs";
const char *WEBMERCATOR = "+proj=webmerc +datum=WGS84";

CoordinateConverter g_converter("+proj=longlat +datum=WGS84", BNG);


CoordinateConverter::CoordinateConverter(const char *srcCRS, const char *dstCRS)
{
  m_ctx = proj_context_create();
  m_proj = proj_create_crs_to_crs(m_ctx, srcCRS, dstCRS, 0);
  if(!m_proj) {
    printf("Cannot create coordinate conversion for: \"%s\" to \"%s\"\n",
	   srcCRS, dstCRS);
    exit(0);
  }
}

CoordinateConverter::~CoordinateConverter()
{
  proj_context_destroy(m_ctx);
}

void CoordinateConverter::convertCoordinate(const double lat, const double lon, const double altitude, double *easting, double *northing, double *altOut)
{
  PJ_COORD coordIn;
  coordIn.xyz.x = lon;
  coordIn.xyz.y = lat;
  coordIn.xyz.z = altitude;
  PJ_COORD coordOut = proj_trans(m_proj, PJ_FWD, coordIn);
  *northing = coordOut.xyz.y;
  *easting = coordOut.xyz.x;
  if(altOut)
    *altOut = coordOut.xyz.z;
}

// -------------------------------------
// -------------------------------------

void ConvertCoordinate(const double lat, const double lon, const double altitude, double *northing, double *easting, double *altOut)
{
  g_converter.convertCoordinate(lat, lon, altitude, northing, easting, altOut);
}


