#ifndef __JSON_UTILS_EX_HPP_
#define __JSON_UTILS_EX_HPP_

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "EMPT/IQ.h"
#include "EMPT/Radar.h"
#include "CEMA/cema_api_struct_v0.3.h"

namespace isnext {

    int toJson(IQDetectResult const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IQDetectResult & info);

    int toJson(IdentifyResultCommArray const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IdentifyResultComm & info);
    
    int toJson(IdentifyResultInterfereArray const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IdentifyResultInterfere & info);

    int toJson(TargetLoc const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, TargetLoc & info);

}
#endif 
