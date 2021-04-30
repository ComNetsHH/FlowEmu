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

		void addModule(const std::string &id, const Json::Value &json_root, bool publish = true);
		void addModule(const std::string &id, std::shared_ptr<Module> module, bool publish = true);
		void updateModule(const std::string &id, const Json::Value &json_root, bool publish = true);
		void removeModule(const std::string &id, bool publish = true, bool publish_paths = true);
		void updateModules(const Json::Value &json_root, bool publish = true);

		void addPath(const Json::Value &json_root, bool publish = true);
		void addPath(Path path, bool publish = true);
		std::list<Path>::iterator removePath(std::list<Path>::iterator it, bool publish = true);
		void updatePaths(const Json::Value &json_root, bool publish = true);

		Json::Value serializeModules();
		Json::Value serializePaths();
		Json::Value serialize();
		void deserialize(const Json::Value &json_root, bool publish = true);

		void loadFromFile(const std::string &filename);
		void saveToFile(const std::string &filename);

	private:
		boost::asio::io_service &io_service;
		Mqtt &mqtt;

		std::map<std::string, std::shared_ptr<Module>> modules;
		std::list<Path> paths;
};

#endif
