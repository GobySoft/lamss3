#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "SimTargets.h"

int main(int argc, char *argv[])
{
	const char *sMissionFile = "uSimTargets.moos";
	
	if(argc > 1) {
		sMissionFile = argv[1];
	}
	
	CSimTargets uSimTargets;
	
	uSimTargets.Run("uSimTargets", sMissionFile);

	return 0;
}

