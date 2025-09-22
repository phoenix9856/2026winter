//#include <QCoreApplication>

//int main(int argc, char *argv[])
//{
//    QCoreApplication a(argc, argv);

//    return a.exec();
//}
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
#include "src/Config.hpp"
#include "src/Logging.hpp"
#include "src/Utils.hpp"
#include "src/JsonUtils.hpp"
#include "Python.h"
//#include "JsonUtilsEx.hpp"




int main(){
    printf("start main...\n");
//    Py_InitializeEx(0);
//    if (!Py_IsInitialized()) {
//        LOG(ERROR)  << "Python init failed" << std::endl;
//        return -1;
//    }
//    PyThreadState * thread_state = PyEval_SaveThread();

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
    char* filename;//= NULL
    sprintf(filename, "/home/ubuntu/QT_project/SampleDate/SampleDate_%d-%d-%d_%d:%d:%d.bin", 1900 + area->tm_year, 1 + area->tm_mon, area->tm_mday, area->tm_hour, area->tm_min, area->tm_sec);

    int file_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);//or S_IRWXU | S_IRWXG | S_IRWXO
    if (file_fd < 0)
        perror(" failed to creat file");
    else
        printf(" creat file at %s\n", filename);

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


//    dataSample.centerFreq = 5000000000;


    int a;
    std::cin << a << endl;
    switch (a) {
    case 1:
        std::shared_ptr<class_loader::ClassLoader> cls_loader_;
        boost::shared_ptr<comm_recog::comm_recog_itf> plugin_;
        try {
            if (!cls_loader_) {
                cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libSignalDetection.so");
            }

            if (!plugin_) {
                plugin_ = cls_loader_->createInstance<comm_recog::comm_recog_itf>("SignalDetection");
                if (!plugin_) {
                    ISN_ERROR_LOG("create node deploy plugin failed, quit job");
                    throw std::runtime_error("create  node deploy plugin failed, quit job");
                }
                plugin_->initialize();
            }
        }catch (const std::exception& ex) {
            ISN_ERROR_LOG("request for identify exception:" << ex.what());
            return -1;
        }
        dataSample.radioFreq = 5000000000;
        dataSample.sampleBand = 600 * 1e6;
        dataSample.sampleFreq = 2.4 * 1e9;
        dataSample.signalType = Communication;
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
        for (int i = 0; i < output.signalInfos.size(); i++) {
            SignalInfo const & signal = output.signalInfos[i];
            std::cout << signal.bandWidth << std::endl;
            std::cout << signal.sampleBand << std::endl;
            std::cout << signal.centerFreq << std::endl;
            std::cout << signal.energy << std::endl;
            std::cout << signal.snr << std::endl;
            std::cout << signal.type << std::endl;
        }
        plugin_.reset();
        cls_loader_.reset();
        break;
    case 2:
        std::shared_ptr<class_loader::ClassLoader> separate_cls_loader_;
        boost::shared_ptr<comm_recog::comm_recog_itf> separate_plugin_;
        std::shared_ptr<class_loader::ClassLoader> identification_cls_loader_;
        boost::shared_ptr<comm_recog::comm_recog_itf> identification_plugin_;
        try {
            if (!separate_cls_loader_) {
                separate_cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libDetectAndSperate.so");
            }

            if (!separate_plugin_) {
                separate_plugin_ = separate_cls_loader_->createInstance<comm_recog::comm_recog_itf>("DetectAndSperate");
                if (separate_plugin_ == nullptr) {
                    ISN_ERROR_LOG("create detection plugin failed, quit job");
                    throw std::runtime_error("create detection plugin failed, quit job");
                }
//                plugin_->initialize();
            }

            if(!identification_cls_loader_){
                identification_cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libSignalIdentification.so");
            }
            if(!identification_plugin_){
                identification_plugin_ = separate_cls_loader_->createInstance<comm_recog::comm_recog_itf>("SignalIdentificationImpl");
                if (identification_plugin_ == nullptr) {
                    ISN_ERROR_LOG("create Identifyion plugin failed, quit job");
                    throw std::runtime_error("create Identifyion plugin failed, quit job");
                }
                identification_plugin_->initialize();
            }
        }catch (const std::exception& ex) {
            ISN_ERROR_LOG("request for identify exception:" << ex.what());
            return -1;
        }

        dataSample.radioFreq = 5000000000;
        dataSample.sampleBand = 600 * 1e6;
        dataSample.sampleFreq = 2.4 * 1e9;
        dataSample.signalType = Communication;
//        FreqLibArray  inputLib;
//        TimeFreqFigure  figure;
        IQDetectResult output;
        IQAfterDepartArray departArray;
        ISN_ERROR_LOG("start to detect");
        if (separate_plugin_->seperateForIdentify(dataSample, output, departArray) != 0) {
            ISN_ERROR_LOG("sperate signal error");
            return -2;
        }
        ISN_ERROR_LOG("sperate signal success");

        IdentifyResultCommArray commResults;
        IdentifyResultInterfereArray interfResults;
        if (identification_plugin_->identifySignals(departArray, commResults, interfResults) != 0) {
            ISN_ERROR_LOG("Identify signal error");
            return -2;
        }
        ISN_ERROR_LOG("Identify signal success");
//        ISN_INFO_LOG("Identify signal size:" << IdentifyResult.identifyResultComm.commresult.size();
//        for (int i = 0; i < commResults.commResults.size(); i++) {
//            IdentifyResult const & identificationresult = commResults.commresult[i];
//            std::cout << identificationresult << std::endl;
//            std::cout << signal.sampleBand << std::endl;
//            std::cout << signal.centerFreq << std::endl;
//            std::cout << signal.energy << std::endl;
//            std::cout << signal.snr << std::endl;
//            std::cout << signal.type << std::endl;
//        }
        if(!IdentifyResult.identifyResultComm.commresult.empty()){
            ISN_INFO_LOG("comm signal size:" << IdentifyResult.identifyResultComm.commresult.size());
            for(int i = 0; i < IdentifyResult.identifyResultComm.commresult.size(); i ++){
                IdentifyResultComm const & identificationCommResult = IdentifyResult.identifyResultComm.commresult[i];
                std::cout << "signal id:" < identificationCommResult.signalId << std::endl;
                std::cout << "signal id:" < identificationCommResult.signalType << std::endl;
                std::cout << "signal id:" < identificationCommResult.moduleType.c_str() << std::endl;
                std::cout << "signal id:" < identificationCommResult.mt << std::endl;
                std::cout << "signal id:" < identificationCommResult.para.frequency << std::endl;
                std::cout << "signal id:" < identificationCommResult.para.power << std::endl;
                std::cout << "signal id:" < identificationCommResult.para.bandWidth << std::endl;
                std::cout << "signal id:" < identificationCommResult.para.quality << std::endl;
                std::cout << "signal id:" < identificationCommResult.para.snr << std::endl;
                std::cout << "signal id:" < identificationCommResult.signalId << std::endl;
                ISN_INFO_LOG("signal type :"  << item.signalType);
                ISN_INFO_LOG("module Type  :" << item.moduleType.c_str());
                ISN_INFO_LOG("module Type  :" << item.mt);
                ISN_INFO_LOG("frequency    :" << item.para.frequency);
                ISN_INFO_LOG("power        :" << item.para.power);
                ISN_INFO_LOG("bandwidth    :" << item.para.bandWidth);
                ISN_INFO_LOG("quality      :" << item.para.quality);
                ISN_INFO_LOG("snr          :" << item.para.snr);

            }

        }
        separate_plugin_.reset();
        separate_cls_loader_.reset();
        identification_plugin_.reset();
        identification_cls_loader_.reset();
        break;
    default:
        break;
    }

//    std::shared_ptr<class_loader::ClassLoader> cls_loader_;
//    boost::shared_ptr<comm_recog::comm_recog_itf> plugin_;
//    try {
//        if (!cls_loader_) {
//            cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libSignalDetection.so");
//        }

//        if (!plugin_) {
//            plugin_ = cls_loader_->createInstance<comm_recog::comm_recog_itf>("SignalDetection");
//            if (!plugin_) {
//                ISN_ERROR_LOG("create node deploy plugin failed, quit job");
//                throw std::runtime_error("create  node deploy plugin failed, quit job");
//            }
//            plugin_->initialize();
//        }
//    }catch (const std::exception& ex) {
//        ISN_ERROR_LOG("request for identify exception:" << ex.what());
//        return -1;
//    }
//    dataSample.radioFreq = 5000000000;
//    dataSample.sampleBand = 600 * 1e6;
//    dataSample.sampleFreq = 2.4 * 1e9;
//    dataSample.signalType = Communication;
//    FreqLibArray  inputLib;
//    TimeFreqFigure  figure;
//    IQDetectResult output;
//    ISN_ERROR_LOG("start to detect");
//    if (plugin_->detectSignals(dataSample, inputLib, output, figure) != 0) {
//        ISN_ERROR_LOG("detect signal error");
//        return -2;
//    }
//    ISN_ERROR_LOG("detect signal success");
//    ISN_INFO_LOG("detect signal size:" << output.signalInfos.size());
////    nlohmann::json out;s
////    isnext::toJson(output,out);

//    for (int i = 0; i < output.signalInfos.size(); i++) {
//        SignalInfo const & signal = output.signalInfos[i];
//        std::cout << signal.bandWidth << std::endl;
//        std::cout << signal.sampleBand << std::endl;
//        std::cout << signal.centerFreq << std::endl;
//        std::cout << signal.energy << std::endl;
//        std::cout << signal.snr << std::endl;
//        std::cout << signal.type << std::endl;
//    }
    delete sendbuff;
//    plugin_.reset();
//    cls_loader_.reset();
//    PyEval_RestoreThread(thread_state);
//    if (Py_IsInitialized()) {
//        Py_Finalize();
//    }
    printf("finish main...\n");
}
