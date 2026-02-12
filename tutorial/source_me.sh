# First detect w/ hostname
case $(hostname -f) in

  adecflow0[12].acorn.wcoss2.ncep.noaa.gov)  MACHINE_ID=acorn ;; ### acorn
  alogin0[12].acorn.wcoss2.ncep.noaa.gov)    MACHINE_ID=acorn ;; ### acorn
  clogin0[1-9].cactus.wcoss2.ncep.noaa.gov)  MACHINE_ID=wcoss2 ;; ### cactus01-9
  clogin10.cactus.wcoss2.ncep.noaa.gov)      MACHINE_ID=wcoss2 ;; ### cactus10
  dlogin0[1-9].dogwood.wcoss2.ncep.noaa.gov) MACHINE_ID=wcoss2 ;; ### dogwood01-9
  dlogin10.dogwood.wcoss2.ncep.noaa.gov)     MACHINE_ID=wcoss2 ;; ### dogwood10

  gaea6[1-8])          MACHINE_ID=gaeac6 ;; ### gaea61-68
  gaea6[1-8].ncrc.gov) MACHINE_ID=gaeac6 ;; ### gaea61-68

  hfe0[1-9]) MACHINE_ID=hera ;; ### hera01-09
  hfe1[0-2]) MACHINE_ID=hera ;; ### hera10-12
  hecflow01) MACHINE_ID=hera ;; ### heraecflow01

  ufe0[1-9]) MACHINE_ID=ursa ;; ### ursa01-09
  ufe1[0-2]) MACHINE_ID=ursa ;; ### ursa10-12
  uecflow01) MACHINE_ID=ursa ;; ### ursaecflow01

  s4-submit.ssec.wisc.edu) MACHINE_ID=s4 ;; ### s4

  fe[1-8]) MACHINE_ID=jet ;; ### jet01-8
  tfe[12]) MACHINE_ID=jet ;; ### tjet1-2

  Orion-login-[1-4].HPC.MsState.Edu) MACHINE_ID=orion ;; ### orion1-4

  [Hh]ercules-login-[1-4].[Hh][Pp][Cc].[Mm]s[Ss]tate.[Ee]du) MACHINE_ID=hercules ;; ### hercules1-4

  login[1-4].stampede2.tacc.utexas.edu) MACHINE_ID=stampede ;; ### stampede1-4

  login0[1-2].expanse.sdsc.edu) MACHINE_ID=expanse ;; ### expanse1-2

  discover3[1-5].prv.cube) MACHINE_ID=discover ;; ### discover31-35
  *) MACHINE_ID=UNKNOWN ;;  # Unknown platform
esac

if [ "$MACHINE_ID" != "ursa" ]; then
  echo "ERROR: This script is intended to be run on the ursa cluster only, for now. Detected MACHINE_ID=$MACHINE_ID"
  exit 1
fi

case $MACHINE_ID in
  ursa)
    module use /contrib/spack-stack/spack-stack-1.9.2/envs/ue-oneapi-2024.2.1/install/modulefiles/Core
    module load stack-oneapi/2024.2.1
    module load stack-intel-oneapi-mpi/2021.13
    module load intel-oneapi-mkl/2024.2.1
    module load stack-python/3.11.7
    module load cmake/3.30.2
    module load hdf5/1.14.3
    module load netcdf-c/4.9.2
    module load eckit/1.28.3
    module load atlas/0.40.0
    module load ncview/2.1.9
    module load netcdf-cxx4/4.3.1
    ;;
  *)
    echo "ERROR: No module commands defined for MACHINE_ID=$MACHINE_ID. Please add the appropriate module load commands to this script."
    exit 1
    ;;
esac
