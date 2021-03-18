#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "Module.hpp"
#include "Path.hpp"
#include "../utils/Mqtt.hpp"

class ModuleManager {
	public:
		ModuleManager(Mqtt &mqtt);

		void addModule(std::string id, std::shared_ptr<Module> module);
		void removeModule(std::string id);
	private:
		Mqtt &mqtt;

		std::map<std::string, std::shared_ptr<Module>> modules;
		std::list<Path> paths;
};

#endif
