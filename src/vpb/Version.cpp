#include <vpb/Version>
#include <string>
#include <stdio.h>

const char* vpbGetVersion()
{
    static char vpb_version[256];
    static int vpb_version_init = 1;
    if (vpb_version_init)
    {
        if (VPB_VERSION_REVISION==0)
        {
            sprintf(vpb_version,"%d.%d.%d",VPB_VERSION_MAJOR,VPB_VERSION_MINOR,VPB_VERSION_RELEASE);
        }
        else
        {
            sprintf(vpb_version,"%d.%d.%d-%d",VPB_VERSION_MAJOR,VPB_VERSION_MINOR,VPB_VERSION_RELEASE,VPB_VERSION_REVISION);
        }

        vpb_version_init = 0;
    }
    
    return vpb_version;
}


const char* vpbGetLibraryName()
{
    return "VirualPlanetBuilder Library";
}
