#ifndef __JSON_UTILS_HPP_
#define __JSON_UTILS_HPP_

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "EMPT/IQ.h"
#include "EMPT/Radar.h"
#include "EMPT/NodeCommon.h"
#include "CEMA/cema_api_struct_v0.3.h"
#include "EMPT/SimulatorParams.h"

namespace isn {

    void fromJson(nlohmann::json const & json, SimModelParam & out);
    void fromJson(nlohmann::json const & json, SourceParam & node);
    void fromJson(nlohmann::json const & json, radar::Param & node);
    void fromJson(nlohmann::json const & json, CommParam & out);
    void fromJson(nlohmann::json const & json, IntfParam & node);
	void fromJson(nlohmann::json const & json, RcvParam & out);
    void toJson(SimModelParam const & node, nlohmann::json & json);
    void toJson(CommParam const & node, nlohmann::json & json);
    void toJson(radar::Param const & node, nlohmann::json & json);
    void toJson(IntfParam const & node, nlohmann::json & json);    

    int toJson(IQDetectResult const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IQDetectResult & info);

    int toJson(IdentifyResultComm const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IdentifyResultComm & info);
    
    int toJson(IdentifyResultInterfere const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, IdentifyResultInterfere & info);

    int toJson(TargetLoc const & info, nlohmann::json & out);
    int fromJson(std::string const & strJson, TargetLoc & info);

    int toJson(SignalSource const & info, nlohmann::json & out);
    int fromJson(nlohmann::json & out, SignalSource & info);

    int fromJson(nlohmann::json & out, std::vector<CEMA::RadarParas> & params);
    int toJson(std::vector<radar::MatchResult> const & rs, nlohmann::json & out);

    int fromJson(nlohmann::json & out, DeployParamInfo & param);
    int toJson(DeployResultInfo const & resultInfo, nlohmann::json & out);
}

#endif //__JSON_UTILS_HPP_

