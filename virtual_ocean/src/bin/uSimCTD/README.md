uSimCTD by Raymond Lum Aug 5, 2011
----------------------------------

The present version of uSimCTD reads the CTD file in XYZCTS text format.  An example of the file is included in "data/CTD.xyzcts".  The six columns in the file correspond to
- x in longitude      (decimal degrees) (e.g.  -70.673300)
- y in latitude       (decimal degrees) (e.g.   42.490000)
- z in positive depth (m)               (e.g     3.000000)
- c in sound speed    (m/s)		(e.g. 1496.217102)
- t in temperature    (degree C)	(e.g.   12.779000)
- s in salinity       (PSU)		(e.g.   32.164000)

This XYZCTS text file can be converted from MODS files using "tools/uMODSReadConvert.m" in Matlab.  In addition to data conversion, the Matlab script also visualizes the CTD data and the locations at which the CTD casts were made.

The MODS files are essentialy the compilation of historical experimental CTD casts that fit closest to the planned operational box area and months.  These files are provided by the MIT Multidisciplinary Simulation, Estimation, and Assimilation Systems (MSEAS) Group (see http://mseas.mit.edu).  The points of contact are Drs Patrick Haley and Wayne Leslie.

The "CTD_Txt_File" in the MOOS configuration block for uSimCTD (see "uSimCTD.moos" for example) must be to pointed to this converted XYZCTS text file.  For example,

	"CTD_Txt_File = /home/raylum/lamss/src/moos/uSimCTD/data/CTD.xyzcts"

Backward Compatibility
----------------------

The present version of uSimCTD still maintains backward compatibility to read the CTD file in XYZCTS binary format.  An example of the file is included in "data/CTD.xyzcts.bin".  The XYZCTS binary file is a by-product of the conversion from MODS files to XYZCTS text file using "tools/uMODSReadConvert.m" in Matlab.

In this case, the "CTD_Bin_File" in the MOOS configuration block must be pointed to the XYZCTS binary file.  For example,

	"CTD_Bin_File = /home/raylum/lamss/src/moos/uSimCTD/data/CTD.xyzcts.bin"

The RECOMMENDED settings for the MOOS configuration is to define either "CTD_Txt_File" (in XYZCTS text format) OR "CTD_Bin_File" (in XYZCTS binary format).  If the specified file is missing, uSimCTD will return error.

In order to allow a universal plug to be used for the uSimCTD MOOS configuration for all the past, present and future cruises, both "CTD_Txt_File" AND "CTD_Bin_File" can be defined CONCURRENTLY (this is not recommended since it can potentially cause confusion in figuring out which CTD file is actually used).  The "CTD_Txt_File" is first searched and used if available.  If it is missing, "CTD_Bin_File" is subsequently searched and used if available.  In the event that both specified files are missing, uSimCTD will return error.
