// *****************************************************************************
// Author: Raymond Lum
// Date:   Aug 3, 2011
// *****************************************************************************

#include "uSimCTD.h"

// *****************************************************************************
// Constructor
// *****************************************************************************

CuSimCTD::CuSimCTD()
{
    dfLat = 41.0;
    dfLong = -71.0;
    dfDepth = 10.0;

    dfLongMatch = 42.0;
    dfLatMatch = -72.0;
    dfDepthMatch = 11.0;

    dfC = 0.1;
    dfT = 0.2;
    dfS = 0.3;
}

// *****************************************************************************
// Destructor
// *****************************************************************************

CuSimCTD::~CuSimCTD()
{
    for (int idx = 0; idx < NDataPoint; idx++) { delete[] dfCTD_DB[idx]; }
    delete[] dfCTD_DB;
}

// *****************************************************************************
// Upon connection to server
// *****************************************************************************

bool CuSimCTD::OnConnectToServer()
{
    m_Comms.Register("NAV_LONG", 0);  // to subscribes list
    m_Comms.Register("NAV_LAT", 0);   // to subscribes list
    m_Comms.Register("NAV_DEPTH", 0); // to subscribes list
    return (true);
}

// *****************************************************************************
// Upon startup
// *****************************************************************************

bool CuSimCTD::OnStartUp()
{
    // CMOOSApp::OnStartUp();
    MOOSTrace("uSimCTD starting....\n");

    string sVal;

    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Read variables from .moos file
    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (m_MissionReader.GetConfigurationParam("CTD_Txt_File", sCTD_Txt_File) &&
        !m_MissionReader.GetConfigurationParam("CTD_Bin_File", sCTD_Bin_File))
    {
        cCTD_File_Type = (char)0; // text file is defined
        MOOSTrace("	=> CTD_Txt_File   = %s\n", sCTD_Txt_File.c_str());
    }
    else if (!m_MissionReader.GetConfigurationParam("CTD_Txt_File", sCTD_Txt_File) &&
             m_MissionReader.GetConfigurationParam("CTD_Bin_File", sCTD_Bin_File))
    {
        cCTD_File_Type = (char)1; // binary file is defined
        MOOSTrace("	=> CTD_Bin_File   = %s\n", sCTD_Bin_File.c_str());
    }
    else if (m_MissionReader.GetConfigurationParam("CTD_Txt_File", sCTD_Txt_File) &&
             m_MissionReader.GetConfigurationParam("CTD_Bin_File", sCTD_Bin_File))
    {
        cCTD_File_Type = (char)2; // text & binary file are defined
        MOOSTrace("	=> CTD_Txt_File   = %s\n", sCTD_Txt_File.c_str());
        MOOSTrace("	=> CTD_Bin_File   = %s\n", sCTD_Bin_File.c_str());
    }
    else
    {
        MOOSTrace("uSimCTD:: CTD_Txt/Bin_File not specified in mission file - FAIL\n");
        return (false);
    }

    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Check if CTD file exists
    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (cCTD_File_Type == 2) // text & binary file are defined
    {
        fsCTD_File.open(sCTD_Txt_File.c_str(), ifstream::in); // try opening text file
        if (fsCTD_File.is_open())
        {
            cCTD_File_Type = (char)0;
            MOOSTrace("	=> Using CTD text file ...\n");
        }
        else
        {
            //fsCTD_File.close();
            fsCTD_File.open(sCTD_Bin_File.c_str(), ifstream::in | ifstream::binary);
            // try opening binary file
            if (fsCTD_File.is_open())
            {
                cCTD_File_Type = (char)1;
                MOOSTrace("	=> Using CTD binary file ...\n");
            }
            else
            {
                MOOSTrace("uSimCTD:: CTD text & binary files not found - FAIL\n");
                return (false);
            }
        }
    }

    if (cCTD_File_Type == 0)
    {
        // --- XYZCTS text format

        if (!fsCTD_File.is_open())
        {
            fsCTD_File.open(sCTD_Txt_File.c_str(), ifstream::in);
            // open as text file if it has not been opened
        }
        if (fsCTD_File.is_open())
        {
            MOOSTrace("	=> Reading CTD text file %s ...\n", sCTD_Txt_File.c_str());
        }
        else
        {
            MOOSTrace("uSimCTD:: CTD text file not found - FAIL\n");
            return (false);
        }

        // check # of data points (i.e. # of lines)

        NDataPoint = (int)0;
        while (std::getline(fsCTD_File, sCTD_DB_buffer)) { NDataPoint++; }
        fsCTD_File.clear();            // clear eofbit
        fsCTD_File.seekg(0, ios::beg); // rewind pointer to beginning again
        MOOSTrace("	=> # of data points in CTD text file = %d\n", NDataPoint);
    }
    else if (cCTD_File_Type == 1)
    {
        // --- XYZCTS binary format

        if (!fsCTD_File.is_open())
        {
            fsCTD_File.open(sCTD_Bin_File.c_str(), ifstream::in | ifstream::binary);
            // open as binary file if it has not been opened
        }
        if (fsCTD_File.is_open())
        {
            MOOSTrace("	=> Reading CTD binary file %s ...\n", sCTD_Bin_File.c_str());
        }
        else
        {
            MOOSTrace("uSimCTD:: CTD binary file not found - FAIL\n");
            return (false);
        }

        // check # of data points

        fsCTD_File.seekg(0, ios::end);
        NDataPoint =
            (int)(fsCTD_File.tellg() /
                  (6 *
                   8.0)); // uCTDDataTxtBin saves each data point (consisting of 6 fields) in float64
        fsCTD_File.seekg(0, ios::beg); // rewind pointer to beginning again
        MOOSTrace("	=> # of data points in CTD binary file = %d\n", NDataPoint);
    }

    // populate CTD database array

    dfCTD_DB = new double*[NDataPoint];
    for (int idx = 0; idx < NDataPoint; idx++)
    {
        dfCTD_DB[idx] = new double[6];

        if (cCTD_File_Type == 0)
        {
            // --- XYZCTS text format
            fsCTD_File >> dfCTD_DB[idx][0] >> dfCTD_DB[idx][1] >> dfCTD_DB[idx][2]; // xyz
            fsCTD_File >> dfCTD_DB[idx][3] >> dfCTD_DB[idx][4] >> dfCTD_DB[idx][5]; // cts
        }
        else if (cCTD_File_Type == 1)
        {
            // --- XYZCTS binary format
            fsCTD_File.read((char*)dfCTD_DB[idx], (size_t)sizeof(double) * 6);
        }

        if ((idx == 0) || (idx == 1) || (idx == (NDataPoint - 2)) || (idx == (NDataPoint - 1)))
        {
            MOOSTrace("	=> Line %d:	%f, %f, %f / %f, %f, %f\n", idx + 1, dfCTD_DB[idx][0],
                      dfCTD_DB[idx][1], dfCTD_DB[idx][2], dfCTD_DB[idx][3], dfCTD_DB[idx][4],
                      dfCTD_DB[idx][5]);
            // screen dump to show that CTD data have been correctly loaded
        }
    }

    // close file

    fsCTD_File.close();

    return (true);
}

// *****************************************************************************
// Upon new mail
// *****************************************************************************

bool CuSimCTD::OnNewMail(MOOSMSG_LIST& NewMail)
{
    MOOSMSG_LIST::iterator p;

    for (p = NewMail.begin(); p != NewMail.end(); p++)
    {
        CMOOSMsg& Msg = *p;

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

    return (true);
}

// *****************************************************************************
// Iteration
// *****************************************************************************

bool CuSimCTD::Iterate()
{
    stringstream buff;

    GetCtsfrLongLatDepth();

    MOOSTrace("Vehicle @ Long: %f, Lat: %f, Depth: %f, match found at Long: %f, Lat: %f, Depth: "
              "%f\n, Sound Vel: %f, Temp: %f, Sal: %f\n",
              dfLong, dfLat, dfDepth, dfLongMatch, dfLatMatch, dfDepthMatch, dfC, dfT, dfS);

    buff << "LongLat(" << setprecision(5) << dfLong << "," << setprecision(5) << dfLat << ")deg, "
         << "Depth(" << setprecision(3) << dfDepth << ")m, " << "C=" << setprecision(2) << dfC
         << "m/s, " << "T=" << setprecision(3) << dfT << "degC, " << "S=" << setprecision(3) << dfS
         << "PSU";

    m_Comms.Notify("CTD1", buff.str()); // to publishes list

    m_Comms.Notify("CTD_SOUND_VELOCITY", dfC); // to publishes list
    m_Comms.Notify("CTD_TEMPERATURE", dfT);    // to publishes list
    m_Comms.Notify("CTD_SALINITY", dfS);       // to publishes list

    return (true);
}

// *****************************************************************************
// Get CTD data based on Long/Lat/Depth location
// *****************************************************************************

void CuSimCTD::GetCtsfrLongLatDepth()
{
    // search using Euclidean distance measure for nearest neighbor
    // for simplicity, each component contributing to distance measure is not normalized
    // no interpolation/extrapolation performed (to be included later)

    int idx, idx_min = 0;
    double dist2, dist2_min = LARGEVAL;

    for (idx = 0; idx < NDataPoint; idx++)
    {
        dist2 = pow((dfCTD_DB[idx][0] - dfLong), 2.0) + pow((dfCTD_DB[idx][1] - dfLat), 2.0) +
                pow((dfCTD_DB[idx][2] - dfDepth), 2.0); // each component is not normalized
        if (dist2 < dist2_min)
        {
            dist2_min = dist2;
            idx_min = idx;
        }
    }

    dfLongMatch = dfCTD_DB[idx_min][0];
    dfLatMatch = dfCTD_DB[idx_min][1];
    dfDepthMatch = dfCTD_DB[idx_min][2];

    dfC = dfCTD_DB[idx_min][3];
    dfT = dfCTD_DB[idx_min][4];
    dfS = dfCTD_DB[idx_min][5];
}
