#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "nlohmann/json.hpp"
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sched.h>
#include "pcie_console.h"

#include "EMPT/comm_recog_itf.h"
#include <iostream>
#include "class_loader/class_loader.hpp"
#include "EMPT/IQ.h"
#include "CEMA/cema_api_struct_v0.3.h"
#include "Logging.hpp"
#include "Python.h"
#include <glog/logging.h>
#include <chrono>
#include "RadarProcess.hpp"
#include "JsonUtilsEx.hpp"
class SignalDetector{
private:
    std::shared_ptr<class_loader::ClassLoader> cls_loader_;
    boost::shared_ptr<comm_recog::comm_recog_itf> plugin_;

public:
    SignalDetector() = default;
    ~SignalDetector(){

    }

    int detect(IQDataSampleInfo const & dataSample){
        try {

            if (!cls_loader_) {
                cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libSignalDetection.so");
            }

            if (!plugin_) {
                plugin_ = cls_loader_->createInstance<comm_recog::comm_recog_itf>("SignalDetection");
                if (!plugin_) {
                    ISN_ERROR_LOG("create detect plugin failed, quit job");
                    throw std::runtime_error("create  detect plugin failed, quit job");
                }
                plugin_->initialize();
            }
        }catch (const std::exception& ex) {
            ISN_ERROR_LOG("request for identify exception:" << ex.what());
            return -1;
        }
        FreqLibArray  inputLib;
        TimeFreqFigure  figure;
        IQDetectResult output;
        ISN_ERROR_LOG("start to detect");
        if (plugin_->detectSignals(dataSample, inputLib, output, figure) != 0) {
            ISN_ERROR_LOG("detect signal error");
            return -2;
        }
        ISN_ERROR_LOG("detect signal success");
        ISN_INFO_LOG("detect signal size:" << output.signalInfos.size());
        nlohmann::json out;
        isnext::toJson(output,out);
        std::cout << out.dump(4) << std::endl;
        return 0;
    }

};

class SignalIdentify{
private:
    std::shared_ptr<class_loader::ClassLoader> separate_cls_loader_;
    boost::shared_ptr<comm_recog::comm_recog_itf> separate_plugin_;
    std::shared_ptr<class_loader::ClassLoader> identification_cls_loader_;
    boost::shared_ptr<comm_recog::comm_recog_itf> identification_plugin_;

public:
    SignalIdentify() = default;
    ~SignalIdentify(){
    }
    int exec(IQDataSampleInfo const & dataSample){
        try {
            if (!separate_cls_loader_) {
                separate_cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libDetectAndSperate.so");
            }

            if (!separate_plugin_) {
                separate_plugin_ = separate_cls_loader_->createInstance<comm_recog::comm_recog_itf>("DetectAndSperate");
                if (separate_plugin_ == nullptr) {
                    std::cout << "end to constrcut data sample" << std::endl;
                    return -2;
                 }
            }

            if(!identification_cls_loader_){
                identification_cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libSignalIdentification.so");
            }
            if(!identification_plugin_){
                identification_plugin_ = identification_cls_loader_->createInstance<comm_recog::comm_recog_itf>("SignalIdentificationImpl");
                if (identification_plugin_ == nullptr) {
                    std::cout << "create Identifyion plugin failed, quit job" << std::endl;
                    return -2;
                }
                identification_plugin_->initialize();
            }
        }catch (const std::exception& ex) {
            std::cout << "request for identify exception:" << ex.what() << std::endl;
            return -1;
        }

        IQDetectResult sperate_output;
        IQAfterDepartArray departArray;
        std::cout << "start to sperate" << std::endl;
        if (separate_plugin_->seperateForIdentify(dataSample, &sperate_output, departArray) != 0) {
            std::cout << "sperate signal error" << std::endl;
            return -2;
        }
        std::cout << "sperate signal success" << std::endl;

        IdentifyResult identifyResult;
        if (identification_plugin_->identifySignals(departArray, identifyResult.identifyResultComm, identifyResult.identifyResultInterfere) != 0) {
            std::cout << "Identify signal error" << std::endl;
            return -2;
        }
        std::cout << "Identify signal success" << std::endl;
        if(!identifyResult.identifyResultComm.commresult.empty()){
            nlohmann::json out = nlohmann::json::array();
            isnext::toJson(identifyResult.identifyResultComm, out);
            std::cout << out.dump(4) << std::endl;
            std::cout<< "comm signal size:" << identifyResult.identifyResultComm.commresult.size() << std::endl;
            for (auto const & item : identifyResult.identifyResultComm.commresult) {
                std::cout<< "signal   id :"  << item.signalId << std::endl;
                std::cout<< "signal type :"  << item.signalType << std::endl;
                std::cout<< "module Type  :" << item.moduleType.c_str() << std::endl;
                std::cout<< "module Type  :" << item.mt << std::endl;
                std::cout<< "frequency    :" << item.para.frequency << std::endl;
                std::cout<< "power        :" << item.para.power << std::endl;
                std::cout<< "bandwidth    :" << item.para.bandWidth << std::endl;
                std::cout<< "quality      :" << item.para.quality << std::endl;
                std::cout<< "snr          :" << item.para.snr << std::endl;
            }
        }else if (!identifyResult.identifyResultInterfere.interfereresult.empty()) {
            std::cout<< "Intra signal size:" << identifyResult.identifyResultInterfere.interfereresult.size() << std::endl;
            nlohmann::json out = nlohmann::json::array();
            isnext::toJson(identifyResult.identifyResultInterfere, out);
            std::cout << out.dump(4) << std::endl;
            for (auto const & item : identifyResult.identifyResultInterfere.interfereresult) {
                std::cout<< "signal   id :"  << item.signalId << std::endl;
                std::cout<< "signal type :"  << item.signalType << std::endl;
                std::cout<< "interfere Type  :" << item.interfereType.c_str() << std::endl;
                std::cout<< "interfere Type  :" << item.it << std::endl;
                std::cout<< "frequency    :" << item.para.frequency << std::endl;
                std::cout<< "power        :" << item.para.power << std::endl;
                std::cout<< "bandwidth    :" << item.para.bandWidth << std::endl;
                std::cout<< "quality      :" << item.para.quality << std::endl;
                std::cout<< "snr          :" << item.para.snr << std::endl;
            }
        } else {
            std::cout << "not support" << std::endl;
            return -2;
        }
        return 0;
    }

};

int main(int argc, char * argv[]){
    google::InitGoogleLogging(argv[0]);
    if (argc < 2) {
        ISN_ERROR_LOG("please input function number [1]:detection [2]:identification [3]:radar");
        return -3;
    }
    std::cout << argv[1] << std::endl;
    int choice = std::stoi(argv[1]);

    printf("start main...\n");
    pcie_console Pcie_Console;
    unsigned char* sendbuff = NULL;
    unsigned int read_len = 8 * 1024 * 1024;

    posix_memalign((void**)&sendbuff, 1024/*alignment*/, read_len);//开一块8M的内存空间
    struct timeval Last_time;
    struct tm* area;

    gettimeofday(&Last_time, 0);
    area = localtime(&(Last_time.tv_sec));
    //char *time= asctime(area);
    //char *time= mktime(area);
    char* filename = new char[150];//= NULL
    sprintf(filename, "/home/ubuntu/QT_project/SampleDate/SampleDate_%d-%d-%d_%d:%d:%d.bin", 1900 + area->tm_year, 1 + area->tm_mon, area->tm_mday, area->tm_hour, area->tm_min, area->tm_sec);

    int file_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);//or S_IRWXU | S_IRWXG | S_IRWXO
    if (file_fd < 0)
        std::cerr << " failed to creat file" << std::endl;
    else
        std::cout << " creat file at" << filename << std::endl;

    //pcie_test();
    ISN_ERROR_LOG("begin to read data from file");
    Pcie_Console.RawDataSample(sendbuff, read_len, 0x02);//0x01 zhong pin data; 0x02:IQ data;

    //write sample data to the file
    write(file_fd, sendbuff, read_len);
    close(file_fd);
//    sendbuff = NULL;
    ISN_ERROR_LOG("begin to constrcut data sample");
    IQDataSampleInfo dataSample;
    dataSample.data.aIQDataArray.resize(read_len/4);
    auto dst = dataSample.data.aIQDataArray.begin();
    for(unsigned char * begin = (unsigned char*) sendbuff; begin < (unsigned char*)(sendbuff + read_len); ) {
        dst->IData = *(short*)begin;
        dst->QData = *(short*)(begin + 2);
//        std::cout << (short*)(begin) << "," << "dst->IQData" << "," << dst->QData << std::endl;
        begin += 4;
        dst++;
    }
    ISN_ERROR_LOG("end to constrcut data sample");

    dataSample.radioFreq = 5000000000;
    dataSample.sampleBand = 600 * 1e6;
    dataSample.sampleFreq = 2.4 * 1e9;
    dataSample.sampleTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock().now().time_since_epoch()).count();
    dataSample.signalType = Radar;

    switch (choice) {
    case 1:
        {
            SignalDetector SignalDetectorResult;
            std::cout << SignalDetectorResult.detect(dataSample) << endl;
        }
        break;
    case 2:
        {
            SignalIdentify SignalIdentifyRsesult;
            std::cout << SignalIdentifyRsesult.exec(dataSample) << endl;
        }
        break;
    case 3 : {
        std::shared_ptr<RadarProcess> processor = RadarProcess::create();
        std::string info = processor->exec(dataSample);
        ISN_INFO_LOG("radar info:" << info);
    } 
    	break;
    default:
        std::cout << "Only perform signal acquisition!" << std::endl;
        break;
    }

    delete sendbuff;
   FINI_LOGGER();
    std::cout << "finish main..." << std::endl;
}
