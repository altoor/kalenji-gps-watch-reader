#include "GPX.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

namespace output
{
	REGISTER_OUTPUT(GPX);

	void GPX::dumpContent(std::ostream& mystream, const Session *session, std::map<std::string, std::string> &configuration)
	{
		bool gpxdata_ext = configuration["gpx_extensions"].find("gpxdata") != std::string::npos;
		bool gpxtpx_ext = configuration["gpx_extensions"].find("gpxtpx") != std::string::npos;
		bool has_extension =  gpxdata_ext || gpxtpx_ext;
		// Latitude and longitude retrieved from the GPS has 6 decimals and can have 2 digits before decimal point
		mystream.precision(8);
		mystream << "<?xml version=\"1.0\"?>" << std::endl;
		mystream << "<gpx version=\"1.1\"" << std::endl;
		mystream << "     creator=\"Kalenji Reader\"" << std::endl;
		mystream << "     xmlns=\"http://www.topografix.com/GPX/1/1\"" << std::endl;
		mystream << "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
		if(gpxdata_ext)
		{
			mystream << "     xmlns:gpxdata=\"http://www.cluetrust.com/XML/GPXDATA/1/0\"" << std::endl;
		}
		if(gpxtpx_ext)
		{
			mystream << "     xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"" << std::endl;
		}
		mystream << "     xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1" << std::endl;
		mystream << "                          http://www.topografix.com/GPX/1/1/gpx.xsd\">" << std::endl;

		// TODO: Improve metadata (add link among others)
		mystream << "  <metadata>" << std::endl;
		mystream << "    <name>" << session->getName() << "</name>" << std::endl;
		mystream << "    <time>" << session->getBeginTime() << "</time>" << std::endl;
		mystream << "  </metadata>" << std::endl;

		mystream << "  <trk>" << std::endl;
		mystream << "    <trkseg>" << std::endl;
		std::list<Point*> points = session->getPoints();
		for(const auto& point : points)
		{
			mystream << "      <trkpt ";
			mystream << point->getLatitude().toStream("lat=\"", "\" ");
			mystream << point->getLongitude().toStream("lon=\"", "\" ");
			mystream << ">" << std::endl;
			mystream << point->getAltitude().toStream("        <ele>", "</ele>") << std::endl;
			mystream << "        <time>" << point->getTimeAsString() << "</time>" << std::endl;
			if(has_extension)
			{
				mystream << "        <extensions>" << std::endl;
			}
			if(gpxdata_ext)
			{
				mystream << point->getHeartRate().toStream("          <gpxdata:hr>", "</gpxdata:hr>\n");
			}
			if(gpxtpx_ext)
			{
				mystream << point->getHeartRate().toStream("          <gpxtpx:TrackPointExtension><gpxtpx:hr>", "</gpxtpx:hr></gpxtpx:TrackPointExtension>\n");
			}
			if(has_extension)
			{
				mystream << "        </extensions>" << std::endl;
			}
			mystream << "      </trkpt>" << std::endl;
		}
		mystream << "    </trkseg>" << std::endl;
		mystream << "  </trk>" << std::endl;
		if(gpxdata_ext)
		{
			mystream << "  <extensions>" << std::endl;
			int nbLap = 0;
			std::list<Lap*> laps = session->getLaps();
			for(const auto& lap : laps)
			{
				// TODO: Remove this check (could be moved to the getters) ?
				if(lap->getStartPoint() == nullptr || lap->getEndPoint() == nullptr)
				{
					std::cerr << "Oups ! I've got a lap without ";
					if(lap->getStartPoint() == nullptr)
						std::cerr << "start";
					else
						std::cerr << "end";
					std::cerr << " point: (" << lap->getFirstPointId() << " - " << lap->getLastPointId() << "). This shouldn't happen ! Report a bug ..." << std::endl;
				}
				else
				{
					nbLap++;
					mystream << "    <gpxdata:lap>" << std::endl;
					mystream << "      <gpxdata:index>" << nbLap << "</gpxdata:index>" << std::endl;
					if(lap->getStartPoint()->getLatitude().isDefined() && lap->getStartPoint()->getLongitude().isDefined())
					{
						mystream << "      <gpxdata:startPoint lat=\"" << lap->getStartPoint()->getLatitude() << "\" lon=\"" << lap->getStartPoint()->getLongitude() << "\"/>" << std::endl;
					}
					if(lap->getEndPoint()->getLatitude().isDefined() && lap->getEndPoint()->getLongitude().isDefined())
					{
						mystream << "      <gpxdata:endPoint lat=\"" << lap->getEndPoint()->getLatitude() << "\" lon=\"" << lap->getEndPoint()->getLongitude() << "\" />" << std::endl;
					}
					mystream << "      <gpxdata:startTime>" << lap->getStartPoint()->getTimeAsString() << "</gpxdata:startTime>" << std::endl;
					mystream << "      <gpxdata:elapsedTime>" << lap->getDuration() << "</gpxdata:elapsedTime>" << std::endl;
					mystream << lap->getCalories().toStream("      <gpxdata:calories>", "</gpxdata:calories>\n");
					mystream << "      <gpxdata:distance>" << lap->getDistance() << "</gpxdata:distance>" << std::endl;
					mystream << lap->getAvgSpeed().toStream("      <gpxdata:summary name=\"AverageSpeed\" kind=\"avg\">", "</gpxdata:summary>\n");
					mystream << lap->getMaxSpeed().toStream("      <gpxdata:summary name=\"MaximumSpeed\" kind=\"max\">", "</gpxdata:summary>\n");
					mystream << lap->getAvgHeartrate().toStream("      <gpxdata:summary name=\"AverageHeartRateBpm\" kind=\"avg\">", "</gpxdata:summary>\n");
					mystream << lap->getMaxHeartrate().toStream("      <gpxdata:summary name=\"MaximumHeartRateBpm\" kind=\"max\">", "</gpxdata:summary>\n");
					// I didn't find a way to differentiate manual lap taking versus automatic (triggered by time or distance)
					// This is the correct syntax, but pytrainer doesn't support it
					//mystream << "      <gpxdata:trigger kind=\"" << configuration["trigger"] << "\" />" << std::endl;
					mystream << "      <gpxdata:trigger>" << configuration["trigger"] << "</gpxdata:trigger>" << std::endl;
					// What can I tell about this ?! Mandatory when using gpxdata (as the two previous one) so I put it with a default value ...
					mystream << "      <gpxdata:intensity>active</gpxdata:intensity>" << std::endl;
					mystream << "    </gpxdata:lap>" << std::endl;
				}
			}
			mystream << "  </extensions>" << std::endl;
		}
		mystream << "</gpx>" << std::endl;
	}
}
