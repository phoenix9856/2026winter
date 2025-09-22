//构建专家库
#include "buildExpertLib.h"

CEMA::FindTargetQueue buildExpertLib()
{
    CEMA::FindTargetQueue expertLib;

    // === AN/MPQ-53 (Patriot) ===
    {
        CEMA::RadarParas radar;
            radar.radarId = 1001;
        radar.edwParas.edwID = 1;

        CEMA::RadarMode mode;
        mode.modeNo = 1;
        mode.pwModulationParas.PWModuType   = 1;   // 固定 PW
        mode.pwModulationParas.pw_value     = 150; // µs
        mode.priModulationParas.PRIModuType = 1;   // 固定 PRI
        mode.priModulationParas.pri_value   = 2222.22; // µs
        mode.priModulationParas.pri_min     = 2222.22;
        mode.priModulationParas.pri_max     = 2222.22;
        mode.cfModulationParas.RFModuType   = 1;   // 固定载频
        mode.cfModulationParas.fc_value     = 5250; // MHz
        mode.cfModulationParas.fc_min       = 5250;
        mode.cfModulationParas.fc_max       = 5250;

        // radar.edwParas.modeParas = mode;
        radar.edwParas.modeParasSet.push_back(mode);

        // radar.edwParas.modeParas = radar.edwParas.modeParasSet.front();

        expertLib.TargetArray.push_back(radar);
    }

    // === AN/SPY-1 (Aegis) ===
    {
        CEMA::RadarParas radar;
        radar.radarId = 1002;
        radar.edwParas.edwID = 2;

        std::vector<float> pws = {6.4, 12.7, 25.0, 51.0}; // µs
        std::vector<float> pris = {320, 635, 1250, 2550};
        for (size_t i = 0; i < pws.size(); ++i)
        {
            CEMA::RadarMode mode;
            mode.modeNo = static_cast<int>(i+1);
            mode.pwModulationParas.PWModuType   = 1;
            mode.pwModulationParas.pw_value     = pws[i];
            mode.priModulationParas.PRIModuType = 1;
            mode.priModulationParas.pri_value   = pris[i]; // µs
            mode.priModulationParas.pri_min     = pris[i];
            mode.priModulationParas.pri_max     = pris[i];
            mode.cfModulationParas.RFModuType   = 1; // 频率捷变
            mode.cfModulationParas.fc_min       = 3100; // MHz
            mode.cfModulationParas.fc_max       = 3500; // MHz


                // radar.edwParas.modeParas = mode; // 默认一个
            radar.edwParas.modeParasSet.push_back(mode);
        }
        // radar.edwParas.modeParas = radar.edwParas.modeParasSet.front();

        expertLib.TargetArray.push_back(radar);
    }

    // === AN/SPS-48E ===
    {
        CEMA::RadarParas radar;
        radar.radarId = 1003;
        radar.edwParas.edwID = 3;

        std::vector<double> pws = {3.0, 9.0, 27.0}; // µs
        std::vector<std::pair<float,float>> prf_ranges = {
            {330, 2750}, {1250, 2000}
        };//363.64-3030.30 ,500-800

        int modeNo = 1;
        for (auto pw : pws)
        {
            for (auto prf : prf_ranges)
            {
                CEMA::RadarMode mode;
                mode.modeNo = modeNo++;
                mode.pwModulationParas.PWModuType = 1;
                mode.pwModulationParas.pw_value   = pw;
                mode.priModulationParas.PRIModuType = 1; // 频率范围
                mode.priModulationParas.pri_min   = static_cast<float>(1e6 / prf.second); // µs
                mode.priModulationParas.pri_max   = static_cast<float>(1e6 / prf.first);  // µs
                mode.cfModulationParas.RFModuType = 1;
                mode.cfModulationParas.fc_min     = 2900;
                mode.cfModulationParas.fc_max     = 3100;
                // if (radar.edwParas.modeParasSet.empty())
                //     radar.edwParas.modeParas = mode;
                radar.edwParas.modeParasSet.push_back(mode);
            }
        }
        // radar.edwParas.modeParas = radar.edwParas.modeParasSet.front();

        expertLib.TargetArray.push_back(radar);
    }


    return expertLib;
}