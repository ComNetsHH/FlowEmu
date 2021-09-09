/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2021 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
		void removeModule(const std::string &id, bool publish = true, bool publish_paths = true, bool force = true);
		void updateModules(const Json::Value &json_root, bool publish = true);
		std::shared_ptr<Module> getModule(const std::string &id);

		void addPath(const Json::Value &json_root, bool publish = true);
		void addPath(Path path, bool publish = true);
		std::list<Path>::iterator removePath(std::list<Path>::iterator it, bool publish = true);
		void updatePaths(const Json::Value &json_root, bool publish = true);

		Json::Value serializeModules();
		Json::Value serializePaths();
		Json::Value serialize();
		void deserialize(const Json::Value &json_root, bool publish = true);

		void publishFiles(const std::string &path);
		void loadFromFile(const std::string &filename);
		void saveToFile(const std::string &filename);

	private:
		boost::asio::io_service &io_service;
		Mqtt &mqtt;

		std::map<std::string, std::shared_ptr<Module>> modules;
		std::list<Path> paths;
};

#endif
