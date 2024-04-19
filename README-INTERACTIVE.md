# Dump1090 Extended Interactive Readme

## About

This version of `dump1090` has an extended interactive mode which renders a map view of the received data into the Linux framebuffer and/or renders into
onto a series of NeoRing LEDS and an LCD display. (see rpi-radar.png)

The program started with the framebuffer display and then added the NeoRings when I found them at a bargain price!

It is particularly designed to support the Raspberry PI and when connected to a 1280 x 1024 display gives a nice view. If you are outside of the UK you will want to choose a different projection for the map as described below in the section _Map Projection_.

The intended use-case is allow to the system to be left on in the background without a network connection giving a continuously updating view that
can be used to know at a glance what a passing aircraft is.

The framebuffer view consists of:

- Text list of aircraft which are colored by range to the receiving station.
- Translation of the flight code to airline in the text list.
- Ranging rings at 15 mile intervals surrounding the receiving station.
- A polyline showing the coastline or some other map data
- A series of fixed points of interest such as nearby airfields or beacons.
- Each aircraft at the correct map location with a code label
- A track direction indicator line for each aircraft

## Modifying

You may want to tune the view for your particular location and hardware.

The following datafiles can be modified:

- `poi.txt` - Contains a '|' (pipe) seperated list of records of points of interest. The first record is the geographic position of the receiving station. And there must be at least one record.
- `region-outline.txt` - Contains the geographic points of a polyline to add context to the view, for example the local coastline. Each point is a latitude and longitude seperated by a comma.
- `airlines.txt` - Contains a '|' (pipe) seperated list of records which map an airline code to an airline name and callsign.

`ppi-display.cpp` contains the whole display. There is a section of constants containing such values as the font size and ranging ring configuration. There are various hard-coded colours too that you could change.

`ppi-neoring.cpp` contains the NeoRing and LCD display.

# Map projection
`geoutils.cpp` - This contains the constant `BNG` (British National Grid) which is the map projection used. There is also the definition `WEBMERCATOR` in there for using the Web Mercator (spherical, Google Earth) projection which supports the entire world, but a national scale specific map projection like `BNG` will be more accurate.

If you are using the software outside the UK then change the map projection either to Web Mercator or something better. All the data files store their data in geographic (lat, long) coordinates so trying different map projections should not be painful. https://epsg.io/ is a good place to find a suitable map projection, UTM for your zone would be a good start.