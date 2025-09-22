#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "EMPT/IQ.h"
#include "EMPT/Radar.h"
#include "CEMA/cema_api_struct_v0.3.h"

namespace isnext {
    int toJson(IQDetectResult const &info, nlohmann::json &out)
    {
        nlohmann::json arrObj = nlohmann::json::array();
        for (auto &item : info.signalInfos)
        {
            nlohmann::json obj = nlohmann::json::object();
            obj["centerFreq"] = item.centerFreq;
            obj["bandWidth"] = item.bandWidth;
            obj["energy"] = item.energy;
            obj["snr"] = item.snr;
            obj["sampleFreq"] = item.sampleFreq;
            obj["sampleBand"] = item.sampleBand;
            obj["detectTime"] = item.detectTime;
            obj["type"] = item.type;
            arrObj.push_back(obj);
        }
        out["signalInfos"] = arrObj;
        return 0;
    }

    int fromJson(std::string const &strJson, IQDetectResult &info)
    {
        nlohmann::json rootObj = nlohmann::json::parse(strJson);
        if (rootObj["signalInfos"].is_array())
        {
            for (auto &item : rootObj["signalInfos"])
            {
                SignalInfo obj;
                obj.centerFreq = item["centerFreq"];
                obj.bandWidth = item["bandWidth"];
                obj.energy = item["energy"];
                obj.snr = item["snr"];
                obj.sampleFreq = item["sampleFreq"];
                obj.sampleBand = item["sampleBand"];
                obj.detectTime = item["detectTime"];
                obj.type = item["type"];
                info.signalInfos.push_back(std::move(obj));
            }
        }
        return 0;
    }

    int toJson(IdentifyResultCommArray const & input, nlohmann::json & jsonOut)
    {
        for (auto const & info : input.commresult) {
            nlohmann::json out;
            out["signalId"] = info.signalId;
            out["signalType"] = info.signalType;
            out["moduleType"] = info.moduleType;
            nlohmann::json paramObj = nlohmann::json::object();
            paramObj["frequency"] = info.para.frequency;
            paramObj["power"] = info.para.power;
            paramObj["bandWidth"] = info.para.bandWidth;
            paramObj["snr"] = info.para.snr;
            out["para"] = paramObj;
            jsonOut.push_back(out);
        }
        return 0;
    }

    int fromJson(std::string const &strJson, IdentifyResultComm &info)
    {
        nlohmann::json rootObj = nlohmann::json::parse(strJson);
        info.signalId = rootObj["signalId"];
        info.signalType = rootObj["signalType"];
        info.moduleType = rootObj["moduleType"];
        nlohmann::json paramObj = rootObj["para"];
        info.para.frequency = paramObj["frequency"];
        info.para.power = paramObj["power"];
        info.para.bandWidth = paramObj["bandWidth"];
        info.para.snr = paramObj["snr"];
        return 0;
    }

    int toJson(IdentifyResultInterfereArray const &input, nlohmann::json &jsonOut)
    {
        for (auto const & info : input.interfereresult) {
            nlohmann::json out;
            out["signalId"] = info.signalId;
            out["signalType"] = info.signalType;
            out["interfereType"] = info.interfereType;
            nlohmann::json paramObj = nlohmann::json::object();
            paramObj["frequency"] = info.para.frequency;
            paramObj["power"] = info.para.power;
            paramObj["bandWidth"] = info.para.bandWidth;
            paramObj["snr"] = info.para.snr;
            out["para"] = paramObj;
            jsonOut.push_back(out);
        }
        return 0;
    }

    int fromJson(std::string const &strJson, IdentifyResultInterfere &info)
    {
        nlohmann::json rootObj = nlohmann::json::parse(strJson);
        info.signalId = rootObj["signalId"];
        info.signalType = rootObj["signalType"];
        info.interfereType = rootObj["interfereType"];
        nlohmann::json paramObj = rootObj["para"];
        info.para.frequency = paramObj["frequency"];
        info.para.power = paramObj["power"];
        info.para.bandWidth = paramObj["bandWidth"];
        info.para.snr = paramObj["snr"];
        return 0;
    }

    int toJson(TargetLoc const &info, nlohmann::json &out)
    {
        out["locResultLon"] = info.targetLoc.lon;
        out["locResultLat"] = info.targetLoc.lat;
        out["locResultHeight"] = info.targetLoc.height;
        out["confidence"] = info.confidence;
        return 0;
    }

    int fromJson(std::string const &strJson, TargetLoc &info)
    {
        nlohmann::json rootObj = nlohmann::json::parse(strJson);
        info.confidence = rootObj["confidence"];
        info.targetLoc.lon = rootObj["locResultLon"];
        info.targetLoc.lat = rootObj["locResultLat"];
        info.targetLoc.height = rootObj["locResultHeight"];
        return 0;
    }



}
