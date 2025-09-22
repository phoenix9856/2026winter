#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "EMPT/IQ.h"
#include "EMPT/Radar.h"
#include "CEMA/cema_api_struct_v0.3.h"
#include "EMPT/SimulatorParams.h"
#include "EMPT/NodeCommon.h"

namespace isn
{

    void fromJson(nlohmann::json const &json, CommParam &out)
    {
        out.symbolRate = json["symbolRate"];
        out.moduleType = json["moduleType"];
    }

    void fromJson(nlohmann::json const &json, radar::Param &node)
    {
        node.pulseWidth = json["pulseWidth"];
        node.pri = json["pri"];
        node.mode = json["mode"];
        node.u = json["u"];
        node.r = json["r"];
    }

    void fromJson(nlohmann::json const &json, IntfParam &node)
    {
        node.power = json["power"];
        node.singleToneDiffFc = json["singleToneDiffFc"];
        if (json["multiToneDiffFc"].is_array())
        {
            for (auto item : json["multiToneDiffFc"])
            {
                node.multiToneDiffFc.push_back(item.get<double>());
            }
        }
        node.sweepSpeed = json["sweepSpeed"];
        node.fcDiffStart = json["fcDiffStart"];
        node.fd = json["fd"];
        node.interfereType = json["interfereType"];
    }

    void fromJson(nlohmann::json const &json, SourceParam &out)
    {
        out.radioFreq = json["radioFreq"];
        out.gainTransmit = json["gainTransmit"];
        out.sigType = json["sigType"];
        out.deviceId = json["deviceId"];
        out.pos.lon = json["pos"]["lon"];
        out.pos.lat = json["pos"]["lat"];
        out.pos.height = json["pos"]["height"];
    }

    void fromJson(nlohmann::json const &json, RcvParam &rcv)
    {
        rcv.radioFreq = json["radioFreq"];
        rcv.sampleFreq = json["sampleFreq"];
        rcv.duration = json["duration"];
        rcv.gainReciver = json["gainReciver"];
        rcv.snr = json["snr"];
        rcv.rcvTime = json["rcvTime"];
        rcv.deviceId = json["deviceId"];
        rcv.pos.lon = json["pos"]["lon"];
        rcv.pos.lat = json["pos"]["lat"];
        rcv.pos.height = json["pos"]["height"];
    }

    void fromJson(nlohmann::json const &json, SimModelParam &out)
    {
        if (json["rcvParams"].is_array())
        {
            for (auto rcvParam : json["rcvParams"])
            {
                RcvParam rcv;
                fromJson(rcvParam, rcv);
                out.rcvParams.push_back(rcv);
            }
        }

        if (json["srcParams"].is_array())
        {
            for (auto srcParam : json["srcParams"])
            {
                SourceParam src;
                fromJson(srcParam, src);
                switch (src.sigType)
                {
                case Communication:
                    fromJson(srcParam["sigParam"], src.sigParam.commParam);
                    break;
                case Radar:
                    fromJson(srcParam["sigParam"], src.sigParam.radarParam);
                    break;
                case Interference:
                    fromJson(srcParam["sigParam"], src.sigParam.intfParam);
                    break;
                default:
                    throw std::string("unknow sig type:" + std::to_string(src.sigType));
                }
                // src.moduleType = srcParam["moduleType"];
                src.pos.lon = srcParam["pos"]["lon"];
                src.pos.lat = srcParam["pos"]["lat"];
                src.pos.height = srcParam["pos"]["height"];
                out.srcParams.push_back(src);
            }
        }
        out.currentDeviceId = json["currentDeviceId"];
    }

    void toJson(CommParam const &node, nlohmann::json &json)
    {
        json["symbolRate"] = node.symbolRate;
        json["moduleType"] = node.moduleType;
    }

    void toJson(radar::Param const &node, nlohmann::json &json)
    {
        json["pulseWidth"] = node.pulseWidth;
        json["pri"] = node.pri;
        json["mode"] = node.mode;
        json["u"] = node.u;
        json["r"] = node.r;
    }

    void toJson(IntfParam const &node, nlohmann::json &json)
    {
        json["power"] = node.power;
        json["singleToneDiffFc"] = node.singleToneDiffFc;
        json["multiToneDiffFc"] = nlohmann::json::array();
        for (auto item : node.multiToneDiffFc)
        {
            json["multiToneDiffFc"].push_back(item);
        }
        json["sweepSpeed"] = node.sweepSpeed;
        json["fcDiffStart"] = node.fcDiffStart;
        json["fd"] = node.fd;
        json["interfereType"] = node.interfereType;
    }
    void toJson(SimModelParam const &node, nlohmann::json &json)
    {

        json["rcvParams"] = nlohmann::json::array();
        json["srcParams"] = nlohmann::json::array();
        json["currentDeviceId"] = nlohmann::json::object();
        auto rcvParams = nlohmann::json::array();
        for (auto rcv : node.rcvParams)
        {
            auto rcvParam = nlohmann::json::object();
            rcvParam["radioFreq"] = rcv.radioFreq;
            rcvParam["sampleFreq"] = rcv.sampleFreq;
            rcvParam["duration"] = rcv.duration;
            rcvParam["gainReciver"] = rcv.gainReciver;
            rcvParam["snr"] = rcv.snr;
            rcvParam["rcvTime"] = rcv.rcvTime;
            rcvParam["deviceId"] = rcv.deviceId;
            auto posNode = nlohmann::json::object();
            posNode["lon"] = rcv.pos.lon;
            posNode["lat"] = rcv.pos.lat;
            posNode["height"] = rcv.pos.height;
            rcvParam["pos"] = posNode;
            rcvParams.push_back(rcvParam);
        }
        json["rcvParams"] = rcvParams;

        auto srcParams = nlohmann::json::array();
        for (auto src : node.srcParams)
        {
            auto srcParam = nlohmann::json::object();
            srcParam["radioFreq"] = src.radioFreq;
            srcParam["gainTransmit"] = src.gainTransmit;
            srcParam["sigType"] = src.sigType;
            srcParam["deviceId"] = src.deviceId;
            auto param = nlohmann::json::object();
            switch (src.sigType)
            {
            case Communication:
                toJson(src.sigParam.commParam, param);
                break;
            case Radar:
                toJson(src.sigParam.radarParam, param);
                break;
            case Interference:
                toJson(src.sigParam.intfParam, param);
                break;
            default:
                throw std::string("unknow sig type:" + std::to_string(src.sigType));
            }
            srcParam["sigParam"] = param;
            // srcParam["moduleType"] = src.moduleType;
            srcParam["pos"]["lon"] = src.pos.lon;
            srcParam["pos"]["lat"] = src.pos.lat;
            srcParam["pos"]["height"] = src.pos.height;
            srcParams.push_back(srcParam);
        }
        json["srcParams"] = srcParams;
        json["currentDeviceId"] = node.currentDeviceId;
    }
    int toJson(Pos const &info, nlohmann::json &out)
    {
        out["lon"] = info.lon;
        out["lat"] = info.lat;
        out["height"] = info.height;
        return 0;
    }

    int fromJson(std::string const &strJson, Pos &info)
    {
        nlohmann::json out = nlohmann::json::parse(strJson);
        info.lon = out["lon"];
        info.lat = out["lat"];
        info.height = out["height"];
        return 0;
    }

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

    int toJson(IdentifyResultComm const &info, nlohmann::json &out)
    {
        out["signalId"] = info.signalId;
        out["signalType"] = info.signalType;
        out["moduleType"] = info.moduleType;
        nlohmann::json paramObj = nlohmann::json::object();
        paramObj["frequency"] = info.para.frequency;
        paramObj["power"] = info.para.power;
        paramObj["bandWidth"] = info.para.bandWidth;
        paramObj["snr"] = info.para.snr;
        out["para"] = paramObj;
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

    int toJson(IdentifyResultInterfere const &info, nlohmann::json &out)
    {
        out["signalId"] = info.signalId;
        out["signalType"] = info.signalType;
        out["interfereType"] = info.interfereType;
        nlohmann::json paramObj = nlohmann::json::object();
        paramObj["frequency"] = info.para.frequency;
        paramObj["power"] = info.para.power;
        paramObj["bandWidth"] = info.para.bandWidth;
        paramObj["snr"] = info.para.snr;
        out["para"] = paramObj;
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

    int toJson(SignalSource const &info, nlohmann::json &out)
    {
        out["posLat"] = info.pos.lat;
        out["posLon"] = info.pos.lon;
        out["posHeight"] = info.pos.height;
        out["frequency"] = info.freq;
        out["power"] = info.power;
        out["antennaPattern"] = info.antennaPattern;
        return 0;
    }

    int fromJson(nlohmann::json &out, SignalSource &info)
    {
        info.pos.lon = out["posLat"];
        info.pos.lat = out["posLon"];
        info.pos.height = out["posHeight"];
        info.freq = out["frequency"];
        info.power = out["power"];
        info.antennaPattern = out["antennaPattern"];
        return 0;
    }

    int fromJson(nlohmann::json &out, std::vector<CEMA::RadarParas> & params)
    {
        if (out.is_array()){
            for (auto &item : out) {
                CEMA::RadarParas info;
                info.radarId = item["radarId"];
                info.edwParas.edwID = item["edwID"];
                if (item["modes"].is_array()) {
                    for (auto &modeJson : item["modes"]) {
                        CEMA::RadarMode mode;
                        mode.priModulationParas.PRIModuType = modeJson["pri_module_type"];
                        if (modeJson["pri"].is_array() && modeJson["pri"].size() >= 1) {
                            if (modeJson["pri"].size() == 1) {
                                mode.priModulationParas.pri_value = modeJson["pri"].at(0);
                                mode.priModulationParas.pri_min = modeJson["pri"].at(0);
                                mode.priModulationParas.pri_max = modeJson["pri"].at(0);
                            }
                            else {
                                mode.priModulationParas.pri_min = modeJson["pri"].at(0);
                                mode.priModulationParas.pri_max = modeJson["pri"].at(1);
                            }
                        }
                        if (modeJson["pw"].is_array() && modeJson["pw"].size() >= 1) {
                            if (modeJson["pw"].size() == 1) {
                                mode.pwModulationParas.pw_value = modeJson["pw"].at(0);
                                mode.pwModulationParas.pw_min = modeJson["pw"].at(0);
                                mode.pwModulationParas.pw_max = modeJson["pw"].at(0);
                            }
                            else {
                                mode.pwModulationParas.pw_min = modeJson["pw"].at(0);
                                mode.pwModulationParas.pw_max = modeJson["pw"].at(1);
                            }
                        }
                        if (modeJson["fc"].is_array() && modeJson["fc"].size() >=1)
                        {
                            if (modeJson["fc"].size() == 1) {
                                mode.cfModulationParas.fc_value = modeJson["fc"].at(0);
                                mode.cfModulationParas.fc_min = modeJson["fc"].at(0);
                                mode.cfModulationParas.fc_max = modeJson["fc"].at(0);
                            }   
                            else {
                                mode.cfModulationParas.fc_min = modeJson["fc"].at(0);
                                mode.cfModulationParas.fc_max = modeJson["fc"].at(1);
                            }
                        }
                        info.edwParas.modeParasSet.push_back(mode);
                    }
                }
                params.push_back(info);
            }
        }

        return 0;
    }

    int toJson(std::vector<radar::MatchResult> const &rs, nlohmann::json &out)
    {
        out = nlohmann::json::array();
        for (auto const &r : rs)
        {
            nlohmann::json v = nlohmann::json::object();
            v["unknownId"] = r.unknownId;
            v["matchedId"] = r.matchedId;
            v["score"] = r.score;
            v["matched"] = r.matched;
            out.push_back(v);
        }
        return 0;
    }

    int fromJson(nlohmann::json & out, DeployParamInfo &param)
    {
        if (out.is_object())
        {
            for (auto &area_json : out["areas"])
            {
                DeployAreaInfo area;
                area.type = area_json["type"];
                for (auto &pos_json : area_json["verticles"])
                {
                    Pos pos;
                    pos.lon = pos_json["lon"];
                    pos.lat = pos_json["lat"];
                    pos.height = pos_json["height"];
                    area.verticles.push_back(pos);
                }
                param.areas.push_back(area);
            }
            for (auto &rcvParam_json : out["rcvDeviceParams"])
            {
                RcvDeviceParam rcvParam;
                for (auto &deviceId_json : rcvParam_json["deviceIds"])
                {
                    rcvParam.deviceIds.push_back(deviceId_json);
                }
                param.rcvDeviceParams.push_back(rcvParam);
            }
            for (auto &rcvParam_json : out["srcDeviceParams"])
            {
                SrcDeviceParam srcParam;
                srcParam.radioFreq = rcvParam_json["radioFreq"];
                srcParam.power = rcvParam_json["power"];
                srcParam.bandWidth = rcvParam_json["bandWidth"];
                param.srcDeviceParams.push_back(srcParam);
            }
        }
        return 0;
    }
    int toJson(DeployResultInfo const &resultInfo, nlohmann::json &out)
    {
        out = nlohmann::json::array();
        for (auto const &deployInfo : resultInfo)
        {
            nlohmann::json deployInfo_json = nlohmann::json::object();
            deployInfo_json["deviceId"] = deployInfo.deviceId;
            nlohmann::json pos_json = nlohmann::json::object();
            pos_json["lon"] = deployInfo.pos.lon;
            pos_json["lat"] = deployInfo.pos.lat;
            pos_json["height"] = deployInfo.pos.height;
            deployInfo_json["pos"] = pos_json;
            out.push_back(deployInfo_json);
        }
        return 0;
    }
}
