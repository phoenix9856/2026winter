#include "class_loader/class_loader.hpp"
#include "EMPT/IQ.h"
#include "CEMA/cema_api_struct_v0.3.h"
#include "src/RestServer.hpp"
#include "src/Config.hpp"
#include "src/Logging.hpp"
#include "src/Utils.hpp"
#include "src/JsonUtils.hpp"

#include "NodeDeployWrapper.h"
#include "NodeDeploy.h"

//extern void ISN_ERROR_LOG = (const char* message);


std::shared_ptr<class_loader::ClassLoader> cls_loader_;
boost::shared_ptr<NodeDeploy> plugin_;

std::shared_ptr<NodeDeploy> creatNodeDeployPlugin() {
	if (!cls_loader_) {
        cls_loader_ = std::make_shared<class_loader::ClassLoader>("/opt/waveform/lib/libnode_deploy.so");
	}
    if (!plugin_) {
        plugin_ = cls_loader_->createInstance<NodeDeploy>("NodeDeployImpl");
        if (!plugin_) {
            ISN_ERROR_LOG("create node deploy plugin failed, quit job");
            throw std::runtime_error("create  node deploy plugin failed, quit job");
        }
        //plugin_->initialize();
    }

//    return plugin_;
}

bool isNodeDeployPluginLoaded() {
    return plugin_ != nullptr;
}

void reloadNodeDeployPlugin() {
    plugin_.reset();
    cls_loader_.reset();
}
