#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include "class_loader/class_loader.hpp"

class NodeDeploy;
//class class_loader::ClassLoader;

std::shared_ptr<NodeDeploy> createNodeDeployPlugin();

bool isNodeDeployPluginLoaded();

void reloadNodeDeployPlugin();
