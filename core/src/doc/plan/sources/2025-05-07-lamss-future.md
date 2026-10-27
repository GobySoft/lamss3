# Reshuffling lamss repos

I propose we refactor lamss into a core repository with several optional git submodules (virtual ocean, autonomy, sonar, attic, ...).

This will hopefully allow users to more easily select the parts of lamss they want to use for a particular application without having to compile all of it (unless they need all of it...).

 Here's my first pass attempt for that classification.

## lamss (core libraries and general purpose apps):
- lib_lamss_protobuf
- pMOOSLink (lamss)
- pCoroner (lamss)
- iMOOS2SQL (lamss)
- pViewMessage (lamss)
- pInitializer (lamss)

## lamss-virtual-ocean (lamss/virtual_ocean):
- uSVP_OceanModel (lamss-shared)
- uSimCTD_OceanModel (lamss-shared)
- uSimADCP_OceanModel (lamss-shared)
- uFldOceanEnvSensor (lamss)
- uSimModemNetwork (lamss-shared)
- uSimModemRange (lamss-shared)
- uSimModemSignal (lamss-shared)
- uSimNoiseCovariance (lamss-shared)
- iNetSimManager (lamss-shared)
- iNoaaBathy? (lamss)
- iMseas? (lamss)
- iMseasBathy? (lamss)
- iCTD_OceanModem? (lamss)
- iGdemSoundSpeed? (lamss)
- uSimBathy? (lamss)
- uSimCTD? (lamss)
- uSimGPS? (lamss)
- uSimSoundVelocityMSEAS? (lamss-shared)
 
## lamss-autonomy (lamss/autonomy):

- uMVS_Bluefin (lamss)
- uSimTargets
- iCTD? (lamss)
- iDurip (lamss-internal)
- iLamssFrontSeat (lamss)
- pFrontSeatData (lamss)
- iEmbeddedFrontseat (lamss-shared)
- pEnvtGrad? (lamss)
- pGateway? (lamss)
- pOctaverMIT? (lamss)
	+ Brings in annoying dependency (octave-dev)
- uSimArrayMooring (lamss)
- uSimMarine_DUTM? (lamss)
	+ Can this be incorporated upstream (moos-ivp)?
- iHydroMAN_LAMSS (lamss-shared)
- pDepthAdapt? (lamss-shared)
- pLamssMissionManager (lamss-shared)
- pPBNodeReport? (lamss-shared)
- SRToolbox? (lamss-shared)
- uSimShipNMEA (lamss-shared)
- uSimTrackDVL (lamss-shared)
- uTDA? (lamss-shared)
- uTopsideSetup? (lamss-shared)
- behavior libs

## lamss-sonar (lamss/sonar):

No idea which of these are deprecated and which are actively used/usable.

- pActiveGeoTracker? (lamss-shared)
- pActiveRangeTracker? (lamss-shared)
- pActiveSonarProcessor? (lamss-shared)
- pBearingTracker? (lamss-shared)
- pGeoTracker? (lamss-shared)
- pLamssAEL? (lamss-shared)
- pLamssBearingTracker? (lamss-shared)
- pLamssCBF? (lamss-shared)
- pLamssTrackManager? (lamss-shared)
- pSonarPingControl? (lamss-shared)
- pTetraHoming? (lamss-shared)
- uSimAcoBeaconRngBrg? (lamss-shared)
- uSimActiveContacts? (lamss-shared)
- uSimActiveSonar (lamss-shared)
- uSimAEL? (lamss-shared)
- uSimPassiveSonar? (lamss-shared)
- uSimSelfNoise? (lamss-shared)
- uSimSourceArray?  (lamss-shared)
- uSimTargetBearings? (lamss-shared)
- uSimTargets? (lamss-shared)
- uSimTowedArray? (lamss-shared)

## lamss-icex (lamss/icex):
- uSimHydroholeNav (lamss-shared)
- uSimICEXMotion (lamss-shared)
- uSimICEXTracker (lamss-shared)
- iICEXTracker (lamss-shared)
- goby/icex_buoy_multiplexer (lamss-shared)
- goby/icex_optimizer (lamss-shared)
- goby/icex_tracker (lamss-shared)

## lamss-attic (deprecated apps, but provide if someone wants them they can init the submodule unneeded):

I would be happy to remove anything that's truly useless as well.

- from lamss:
	- i3DM_GX1
	- i3DM_GX2
	- iArduinoDIO
	- iBackSeatServer
	- iDAS
	- iGSC_16AO16
	- iHuxley
	- iKayakControlBoard
	- iLineBasedComms
	- iMetaCompass
	- iModemSim
	- iMultiStaticSonar?
	- iNMEASpeedTransducer
	- iOS5000
	- iPWMController
	- iSerialNMEA
	- iWebsite
	- iWifi
	- MOOSBlink
	- pFilterCurrent
	- pHelmToSlocum
	- pHystereticThermostat
	- pImagePro
	- pMailReader
	- pMissionMonitor
	- pRBTracker
	- pROI
	- pSFilterCurrent
	- uNafconMessageViewer
	- uSimSlocum
	- uTPB
	- zLoggerNG
- from lamss-shared:
	- HydroMANSimUtilities? 
		+ Part of HydroMAN distro?
	- HydroMANToolbox
		+ Part of HydroMAN distro?
	- iMOOSGateway
	- pClusterPriority
	- pCommsStateTracker
- from lamss-internal:
	+ Everything except as listed above

# Documentation

I would love to see training topics around:

- Running virtual ocean for acomms sim
- Running virtual ocean for sonar sim (for different sonar types)
- Autonomy (pLamssMissionManager) missions

I would be happy to defer:
- Documentation for "attic" applications
- Documentation for "icex" applications (realistically this would mostly like be me picking this up again).

# Licensing

I would suggest LGPL for libraries/GPL for apps. This is the process for "disclosing" software officially via MIT TLO: https://tlo.mit.edu/researchers-mit-community/protect/software-open-source-protection

# Reformatting code

I suggest we settle on a clang-format style and develop the appropriate .clang-format file (e.g., https://github.com/GobySoft/goby3/blob/3.0/.clang-format), and then run clang-format on the entire project.

# Maintenance plan

## DevOps

We should also create Debian/Ubuntu packages for the various submodules, and do continuous/release builds of the software using e.g., CircleCI or Github Actions. 

## Maintainers / POC

I would suggest we delegate submodules to different maintainers to spread the load, e.g. 2-3 people for each of:

- lamss (core)
- virtual ocean
- automomy
- icex


# Updating preprocessing mission config (missions-lamss)

I would like to make missions-lamss part of lamss (e.g., lamss/missions).

I would think doing the preprocessing in Python would be better than the `gpp` tool currently used.
