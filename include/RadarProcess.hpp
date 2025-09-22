#ifndef __RADAR_PROCESS_HPP__
#define __RADAR_PROCESS_HPP__

#include <memory>
#include <EMPT/IQ.h>


class RadarProcess{
public:
    virtual std::string exec(IQDataSampleInfo const & sampleInfo) = 0;
    static std::shared_ptr<RadarProcess> create();
};

#endif 