#include <MOOS/libMOOS/MOOSLib.h>
#include "GeoTracker.h"

int main(int argc ,char * argv[])
{
	char * sMissionFile = "Mission.moos";
	char * sMOOSName = "pGeoTracker";

	switch(argc)
	{
	case 3:
	  sMOOSName = argv[2];
	case 2:
	  sMissionFile = argv[1];
	}

    CGeoTracker Tracker;

    Tracker.Run(sMOOSName,sMissionFile);


    return 0;
}
