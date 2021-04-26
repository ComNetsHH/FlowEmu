#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <string>
#include <map>
#include <memory>

#include "Module.hpp"
#include "Path.hpp"
#include "../utils/Mqtt.hpp"

#include <boost/asio.hpp>
#include <json/json.h>

class ModuleManager {
	public:
		ModuleManager(boost::asio::io_service &io_service, Mqtt &mqtt);
		~ModuleManager();

		void addModule(std::string id, std::shared_ptr<Module> module, bool publish = true);
		void updateModule(std::string id, Json::Value json_root, bool publish = true);
		void removeModule(std::string id, bool publish = true);
	private:
		boost::asio::io_service &io_service;
		Mqtt &mqtt;

		std::map<std::string, std::shared_ptr<Module>> modules;
		std::list<Path> paths;
};

#endif
