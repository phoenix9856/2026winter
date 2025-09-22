#include "RadarProcess.hpp"
#include "class_loader/class_loader.hpp"
#include <boost/smart_ptr.hpp>
#include "EMPT/SignalConverter.hpp"
#include "EMPT/RadarIdentificationItf.hpp"
#include "EMPT/RadarAlgoItf.h"
#include "Logging.hpp"
#include "buildExpertLib.h"

class RadarProcessImpl : public RadarProcess {
public:
    virtual std::string exec(IQDataSampleInfo const & sampleInfo) {
        std::string ROOT_PATH = 
            std::getenv("COMM_PLUGIN_BASE_DIR") == nullptr ? "." : std::getenv("COMM_PLUGIN_BASE_DIR");

        if (!radar_iq2pdw_loader_) {
            radar_iq2pdw_loader_ = boost::make_shared<class_loader::ClassLoader>( ROOT_PATH + "/lib/libradar_iq2pdw.so");
        }
        
        if (!radar_iq2pdw_plugin_) {
            radar_iq2pdw_plugin_ = radar_iq2pdw_loader_->createInstance<SignalConverterItf>("SignalConverterImpl");
            if (!radar_iq2pdw_plugin_) {
                ISN_ERROR_LOG("create detection plugin failed, quit job");
                throw std::runtime_error("create detection plugin failed, quit job");
            }
            //radar_iq2pdw_plugin_->initialize();
        }
        CEMA::PDWArray pdws;
        ISN_ERROR_LOG("init radar iq2pdw ok");
        if (radar_iq2pdw_plugin_->exec(sampleInfo, pdws, 3000) != 0) {
            ISN_ERROR_LOG("convert failed, quit job");
            throw std::runtime_error("convert failed, quit job");
        }
        ISN_ERROR_LOG("converted pdw signal size:" << pdws.aPDWArray.size());

        if (!radarsorting_cls_loader_) {
            radarsorting_cls_loader_ = boost::make_shared<class_loader::ClassLoader>( ROOT_PATH + "/lib/libradar_signal_sorting.so");
        }

        if (!radar_sorting_plugin_) {
            radar_sorting_plugin_ = radarsorting_cls_loader_->createInstance<RadarAlgoItf>("MyRadarAlgorithm");
            if (!radar_sorting_plugin_) {
                //ISN_ERROR_LOG("create radar sorting plugin failed, quit job");
                throw std::runtime_error("create radar sorting plugin failed, quit job");
            }
            //radar_iq2pdw_plugin_->initialize();
        }
        CEMA::FindTargetQueue  output;        
        //ISN_ERROR_LOG("init radar sorting ok");
        if (radar_sorting_plugin_->process(pdws, output) != 0) {
            //ISN_ERROR_LOG("radar sorting failed, quit job");
            throw std::runtime_error("radar sorting failed, quit job");
        }
        CEMA::FindTargetQueue expertLib = buildExpertLib();
        ISN_INFO_LOG("data size:" << output.TargetArray.size() << "< expertlis:" << expertLib.TargetArray.size());
        
        if (!radar_identify_cls_loader_) {
            radar_identify_cls_loader_ = boost::make_shared<class_loader::ClassLoader>( ROOT_PATH + "/lib/libradar_identification.so");
        }

        if (!radar_identify_plugin_) {
            radar_identify_plugin_ = radar_identify_cls_loader_->createInstance<RadarIdentificationItf>("RadarIdentificationImpl");
            if (!radar_identify_plugin_) {
                ISN_ERROR_LOG("create radar identify plugin failed, quit job");
                throw std::runtime_error("create radar identify plugin failed, quit job");
            }
            //radar_identify_plugin_->initialize();
        }
        std::vector<radar::MatchResult> results;     
        ISN_ERROR_LOG("init radar identify ok");
        if (radar_identify_plugin_->run(output, expertLib, results) != 0) {
            ISN_ERROR_LOG("radar identify failed, quit job");
            throw std::runtime_error("radar sorting failed, quit job");
        }
        ISN_ERROR_LOG("radar identify signal size:" << results.size());

        return 0;
    }
private:
    boost::shared_ptr<class_loader::ClassLoader> radar_iq2pdw_loader_;
    boost::shared_ptr<SignalConverterItf> radar_iq2pdw_plugin_;
    boost::shared_ptr<class_loader::ClassLoader> radarsorting_cls_loader_;
    boost::shared_ptr<RadarAlgoItf> radar_sorting_plugin_;
    boost::shared_ptr<class_loader::ClassLoader> radar_identify_cls_loader_;
    boost::shared_ptr<RadarIdentificationItf> radar_identify_plugin_;
};

std::shared_ptr<RadarProcess> RadarProcess::create() {
    return std::shared_ptr<RadarProcess>(new RadarProcessImpl());
}
