// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Author: Raymond Lum
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "uSimCTD.h"

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Constructor
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CuSimCTD::CuSimCTD()
{
	dfC = 0.1;
	dfT = 0.2;
	dfS = 0.3;

        dfLat   =  41.0;
        dfLong  = -71.0;
        dfDepth =  10.0;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Destructor
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CuSimCTD::~CuSimCTD()
{
	if (fsMods_Bin_File != NULL)
		fclose(fsMods_Bin_File);
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Upon startup
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

bool CuSimCTD::OnStartUp()
{
	// CMOOSApp::OnStartUp();
	MOOSTrace("uSimCTD starting....\n");
	
	string sVal;
	int    FileSize, idx, count;
	
	// ++++++++++++++++++++++++++++++
	// Read variables from .moos file
	// ++++++++++++++++++++++++++++++
	
	if(m_MissionReader.GetValue("Mods_Bin_File",sVal))
	{
		sMods_Bin_File = sVal.data();
		MOOSTrace("	=> Mods_Bin_File	= %s\n",sMods_Bin_File);
	}
	else
	{
		MOOSTrace("uSimCTD:: message type not set - FAIL\n");        
		return false;
	}
	
	// ++++++++++++++++++++++++++++++
	// Check if bathy bin file exists
	// ++++++++++++++++++++++++++++++
	
	if ( (fsMods_Bin_File = fopen(sMods_Bin_File, "rb")) == NULL )
	{
		MOOSTrace("uSimCTD:: mods bin file not found - FAIL\n");    
		return false;
	}
        
	// check file size & # of data points
	
	fseek(fsMods_Bin_File,0,SEEK_END);
	FileSize   = ftell(fsMods_Bin_File);
	NDataPoint = (int)(FileSize / (6*8.0));	// xyzcts
	MOOSTrace("	=> # points in bin file	= %d\n", NDataPoint );
	
	// reset pointer
	
	fseek(fsMods_Bin_File,0,SEEK_SET);
        
	// populate bathy database array
	
	dfMods_DB = (double**) calloc( (size_t)NDataPoint, (size_t)(sizeof(double*)) );
	for (idx = 0; idx<NDataPoint; idx++)
	{
		dfMods_DB[idx] = (double*) calloc( (size_t)6, (size_t)(sizeof(double)));
		count          = fread ((double*)dfMods_DB[idx], (size_t)sizeof(double), (size_t)6, fsMods_Bin_File);
		
		if ((idx==0)||(idx==1)||(idx==(NDataPoint-2))||(idx==(NDataPoint-1)))
		{
			if (count!=6)
			{
				MOOSTrace("uSimCTD:: Error in reading binary file - FAIL.\n");
				return false;
			}
			MOOSTrace("	=> Line %d:	%f, %f, %f, %f, %f, %f\n",idx,dfMods_DB[idx][0],dfMods_DB[idx][1],dfMods_DB[idx][2],dfMods_DB[idx][3],dfMods_DB[idx][4],dfMods_DB[idx][5]);	// xyzcts
		}
	}
	
	return(true);
	
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Upon connection to server
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

bool CuSimCTD::OnConnectToServer()
{
	
	m_Comms.Register("NAV_LONG", 0);	// to subscribes list
	m_Comms.Register("NAV_LAT",  0);	// to subscribes list
	m_Comms.Register("NAV_DEPTH",0);	// to subscribes list
	return(true);
	
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Iterative process
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

bool CuSimCTD::Iterate()
{
	
	char buff[STRLEN];
	
	GetCtsfrLatLong();
	
	sprintf(buff, "LongLat(%.5f,%.5f)deg, Depth(%.3f), C=%.2fm/s, T=%.3fdegC, S=%.3fPSU from datafile(%s)",dfLong,dfLat,dfDepth, dfC,dfT,dfS, sMods_Bin_File);
	m_Comms.Notify("CTD1",buff);			// to publishes list
	
	m_Comms.Notify("CTD_SOUND_VELOCITY",dfC);	// to publishes list
	m_Comms.Notify("CTD_TEMPERATURE",   dfT);	// to publishes list
	m_Comms.Notify("CTD_SALINITY",      dfS);	// to publishes list
	
	return(true);
	
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Upon new mail
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

bool CuSimCTD::OnNewMail(MOOSMSG_LIST &NewMail)
{
	
	MOOSMSG_LIST::iterator p;
	
	for(p = NewMail.begin(); p != NewMail.end(); p++)
	{
		CMOOSMsg &Msg = *p;
		
		if (Msg.m_sKey == "NAV_LONG")
		{
        		dfLong = Msg.m_dfVal;
			// MOOSTrace("NAV_LONG = %f\n",dfLong);
		}
		if (Msg.m_sKey == "NAV_LAT")
		{
	        	dfLat = Msg.m_dfVal;
			// MOOSTrace("NAV_LAT = %f\n",dfLat);
		}
		if (Msg.m_sKey == "NAV_DEPTH")
		{
	        	dfDepth = Msg.m_dfVal;
			// MOOSTrace("NAV_DEPTH = %f\n",dfDepth);
		}		
		
	}
	
	return(true);
	
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Get CTD data based on Lat/Long location
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

char CuSimCTD::GetCtsfrLatLong()
{
		

	double dfLongMatch, dfLatMatch, dfDepthMatch;
	int    idx, idx_min;
	double dist, dist_min;
	
	// search for minimum euclidean distance
	
	idx_min  = 0;
	dist_min = 50.0;
	
	for (idx = 0; idx<NDataPoint; idx++)
	{
		dist = pow( (dfMods_DB[idx][0] - dfLong), 2.0 ) + pow( (dfMods_DB[idx][1] - dfLat), 2.0 ) + pow( (dfMods_DB[idx][2] - dfDepth), 2.0 );
		if (dist < dist_min)
		{
			dist_min = dist;
			idx_min  = idx;
		}
	}
	
	dfLongMatch  = dfMods_DB[idx_min][0];
	dfLatMatch   = dfMods_DB[idx_min][1];
	dfDepthMatch = dfMods_DB[idx_min][2];
	
	dfC = dfMods_DB[idx_min][3];
	dfT = dfMods_DB[idx_min][4];
	dfS = dfMods_DB[idx_min][5];
	
	printf("Match found at Long: %f, Lat: %f, Depth: %f, Sound Vel: %f, Temp: %f, Sal: %f\n",dfLongMatch,dfLatMatch,dfDepthMatch, dfC,dfT,dfS);
	
}
