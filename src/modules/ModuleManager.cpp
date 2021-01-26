#include "ModuleManager.hpp"

#include <memory>

#include "../utils/Packet.hpp"

using namespace std;

void ModuleManager::push_back(Module *module) {
	if(!modules.empty()) {
		auto left_module = dynamic_cast<ModuleHasRight<shared_ptr<Packet>>*>(modules.back());
		auto new_module = dynamic_cast<ModuleHasLeft<shared_ptr<Packet>>*>(module);

		left_module->setRightModule(new_module);
		new_module->setLeftModule(left_module);
	}

	modules.push_back(module);
}