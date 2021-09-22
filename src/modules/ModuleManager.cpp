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

#include "ModuleManager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <regex>
#include <stdexcept>

#include "delay/FixedDelayModule.hpp"
#include "loss/GilbertElliotLossModule.hpp"
#include "loss/UncorrelatedLossModule.hpp"
#include "meter/DelayMeter.hpp"
#include "meter/ThroughputMeter.hpp"
#include "null/NullModule.hpp"
#include "queue/FifoQueueModule.hpp"
#ifdef MACHINE_LEARNING
#include "queue/DQLQueueModule.hpp"
#endif
#include "rate/BitrateRateModule.hpp"
#include "rate/FixedIntervalRateModule.hpp"
#include "rate/TraceRateModule.hpp"

using namespace std;

ModuleManager::ModuleManager(boost::asio::io_service &io_service, Mqtt &mqtt) : io_service(io_service), mqtt(mqtt) {
	mqtt.subscribeJson("set/module/+", [&](const string &topic, const Json::Value &json_root) {
		regex r("set\\/module\\/(\\w+)");
		smatch m;
		if(!(regex_search(topic, m, r) && m.size() == 2)) {
			return;
		}
		string node_id = m.str(1);

		if(!json_root.empty()) {
			if(modules.find(node_id) == modules.end()) {
				addModule(node_id, json_root);
			} else {
				updateModule(node_id, json_root);
			}
		} else {
			removeModule(node_id, true, true, false);
		}
	});

	mqtt.subscribeJson("set/paths", [&](const string &topic, const Json::Value &json_root) {
		updatePaths(json_root);
	});

	mqtt.subscribe("set/load", [&](const string &topic, const string &payload) {
		if(!regex_match(payload, regex("[\\w-]+"))) {
			cerr << "Invalid filename!" << endl;
			return;
		}

		loadFromFile("config/graphs/" + payload + ".json");
	});

	mqtt.subscribe("set/save", [&](const string &topic, const string &payload) {
		if(!regex_match(payload, regex("[\\w-]+"))) {
			cerr << "Invalid filename!" << endl;
			return;
		}

		saveToFile("config/graphs/" + payload + ".json");
	});

	publishFiles("config/graphs");

	mqtt.publish("get/paths", Json::arrayValue, true, true);
}

void ModuleManager::addModule(const string &id, const Json::Value &json_root, bool publish) {
	string type = json_root.get("type", "").asString();

	shared_ptr<Module> new_module;
	if(type == "fixed_delay") {
		new_module = make_shared<FixedDelayModule>(io_service, 50);
	} else if(type == "gilbert_elliot_loss") {
		new_module = make_shared<GilbertElliotLossModule>(io_service, 0.001, 0.001, 0, 100);
	} else if(type == "uncorrelated_loss") {
		new_module = make_shared<UncorrelatedLossModule>(10);
	} else if(type == "delay_meter") {
		new_module = make_shared<DelayMeter>(io_service);
	} else if(type == "throughput_meter") {
		new_module = make_shared<ThroughputMeter>(io_service);
	} else if(type == "null") {
		new_module = make_shared<NullModule>();
	#ifdef MACHINE_LEARNING
	} else if(type == "dql_queue") {
		new_module = make_shared<DQLQueueModule>(io_service, 100, 0.001);
	#endif
	} else if(type == "fifo_queue") {
		new_module = make_shared<FifoQueueModule>(io_service, 100);
	} else if(type == "bitrate_rate") {
		new_module = make_shared<BitrateRateModule>(io_service, 1000000);
	} else if(type == "fixed_interval_rate") {
		new_module = make_shared<FixedIntervalRateModule>(io_service, chrono::milliseconds(1));
	} else if(type == "trace_rate") {
		new_module = make_shared<TraceRateModule>(io_service, "config/traces/Verizon-LTE-short.down", "config/traces/Verizon-LTE-short.up");
	} else {
		cerr << "Unknown module type: " << type << endl;
		return;
	}

	addModule(id, new_module, false);
	updateModule(id, json_root, publish);
}

void ModuleManager::addModule(const string &id, shared_ptr<Module> module, bool publish) {
	if(modules.find(id) != modules.end()) {
		cerr << "Module " + id + " already exists!" << endl;
		return;
	}

	cout << "Add module " + id + "!" << endl;
	modules[id] = module;

	for(const auto& entry : module->getParameters()) {
		const string &parameter_id = entry.second.id;
		const auto parameter = entry.second.parameter;

		mqtt.subscribe("set/module/" + id + "/" + parameter_id, [&, parameter](const string &topic, const string &payload) {
			double value;

			try {
				value = stod(payload);
			} catch(const std::exception& e) {
				value = numeric_limits<double>::quiet_NaN();
			}

			if(isfinite(value)) {
				parameter->set(value);
			} else {
				parameter->callChangeHandlers();
			}
		});

		mqtt.publish("get/module/" + id + "/" + parameter_id, to_string(parameter->get()), true, false);
		parameter->addChangeHandler([&, id, parameter_id](double value) {
			mqtt.publish("get/module/" + id + "/" + parameter_id, to_string(value), true, false);
		});
	}

	for(const auto& entry : module->getStatistics()) {
		const string &statistic_id = entry.second.id;
		const auto statistic = entry.second.statistic;

		statistic->addHandler([&, id, statistic_id](double value) {
			mqtt.publish("get/module/" + id + "/" + statistic_id, to_string(value), true, true);
		});
	}

	if(publish) {
		mqtt.publish("get/module/" + id, module->serialize(), true, true);
	}
}

void ModuleManager::updateModule(const string &id, const Json::Value &json_root, bool publish) {
	if(modules.find(id) == modules.end()) {
		cerr << "Module " + id + " does not exist!" << endl;
		return;
	}

	modules.at(id)->deserialize(json_root);

	if(publish) {
		mqtt.publish("get/module/" + id, modules.at(id)->serialize(), true, true);
	}
}

void ModuleManager::removeModule(const string &id, bool publish, bool publish_paths, bool force) {
	if(modules.find(id) == modules.end()) {
		cerr << "Module " + id + " does not exist!" << endl;

		if(publish) {
			mqtt.publish("get/module/" + id, nullptr, true, true);
		}

		return;
	}

	if(force == false && modules.at(id)->getRemovable() == false) {
		cerr << "Module " + id + " is not removable!" << endl;
		return;
	}

	if(publish) {
		mqtt.publish("get/module/" + id, nullptr, true, true);

		for(const auto& entry : modules.at(id)->getParameters()) {
			const string &parameter_id = entry.second.id;

			mqtt.unsubscribe("set/module/" + id + "/" + parameter_id);

			mqtt.publish("get/module/" + id + "/" + parameter_id, nullptr, true, true);
		};

		for(const auto& entry : modules.at(id)->getStatistics()) {
			const string &statistic_id = entry.second.id;

			mqtt.publish("get/module/" + id + "/" + statistic_id, nullptr, true, true);
		};
	}

	for(auto it = paths.begin(); it != paths.end(); /*++it*/) {
		if(it->from_node_id == id || it->to_node_id == id) {
			it = removePath(it, false);
		} else {
			++it;
		}
	}

	if(publish && publish_paths) {
		mqtt.publish("get/paths", serializePaths(), true, true);
	}

	cout << "Remove module " + id + "!" << endl;
	modules.erase(id);
}

void ModuleManager::updateModules(const Json::Value &json_root, bool publish) {
	for(const auto& module : modules) {
		if(json_root.isMember(module.first)){
			continue;
		}

		removeModule(module.first, publish);
	}

	for(auto it = json_root.begin(); it != json_root.end(); ++it) {
		if(modules.find(it.name()) == modules.end()) {
			addModule(it.name(), *it, publish);
		} else {
			updateModule(it.name(), *it, publish);
		}
	}
}

void ModuleManager::addPath(Path path, bool publish) {
	try {
		path.from_port_info = modules.at(path.from_node_id)->getPort(path.from_port_id);
		path.to_port_info = modules.at(path.to_node_id)->getPort(path.to_port_id);
	} catch(const out_of_range &e) {
		cerr << "Unknown node or port ID in path!" << endl;
		return;
	}

	try {
		path.from_port_info.port->connect(path.to_port_info.port);
		path.to_port_info.port->connect(path.from_port_info.port);
	} catch(const incompatible_port_types &e) {
		cerr << "Cannot connect incompatible port types!" << endl;
		return;
	}

	cout << "Add path!" << endl;
	paths.push_back(path);

	if(publish) {
		mqtt.publish("get/paths", serializePaths(), true, true);
	}
}

shared_ptr<Module> ModuleManager::getModule(const string &id) {
	try {
		return modules.at(id);
	} catch(const out_of_range &e) {
		throw out_of_range("Unknown module ID: " + id);
	}
}

list<Path>::iterator ModuleManager::removePath(list<Path>::iterator it, bool publish) {
	it->from_port_info.port->disconnect();
	it->to_port_info.port->disconnect();

	cout << "Remove path!" << endl;
	it = paths.erase(it);

	if(publish) {
		mqtt.publish("get/paths", serializePaths(), true, true);
	}

	return it;
}

void ModuleManager::updatePaths(const Json::Value &json_root, bool publish) {
	for(auto it = paths.begin(); it != paths.end(); /*++it*/) {
		bool found = false;
		for(const auto& json_path : json_root) {
			if(!(json_path.isMember("from") && json_path.isMember("to"))) {
				continue;
			}

			Path path;
			path.deserialize(json_path);

			if(it->from_node_id == path.from_node_id &&
				it->from_port_id == path.from_port_id &&
				it->to_node_id == path.to_node_id     &&
				it->to_port_id == path.to_port_id        ) {
				found = true;
				break;
			}
		}

		if(!found) {
			it = removePath(it, false);
		} else {
			++it;
		}
	}

	for(const auto& json_path : json_root) {
		if(!(json_path.isMember("from") && json_path.isMember("to"))) {
			continue;
		}

		Path path;
		path.deserialize(json_path);

		bool found = false;
		for(const auto& item : paths) {
			if(item.from_node_id == path.from_node_id &&
				item.from_port_id == path.from_port_id &&
				item.to_node_id == path.to_node_id     &&
				item.to_port_id == path.to_port_id        ) {
				found = true;
				break;
			}
		}

		if(!found) {
			addPath(path, false);
		}
	}

	if(publish) {
		mqtt.publish("get/paths", serializePaths(), true, true);
	}
}

Json::Value ModuleManager::serializeModules() {
	Json::Value json_root;
	for(const auto& module : modules) {
		json_root[module.first] = module.second->serialize();
	}

	return json_root;
}

Json::Value ModuleManager::serializePaths() {
	Json::Value json_root = Json::arrayValue;
	for(const auto& path : paths) {
		json_root.append(path.serialize());
	}

	return json_root;
}

Json::Value ModuleManager::serialize() {
	Json::Value json_root;
	json_root["modules"] = serializeModules();
	json_root["paths"] = serializePaths();

	return json_root;
}

void ModuleManager::deserialize(const Json::Value &json_root, bool publish) {
	updateModules(json_root["modules"], publish);
	updatePaths(json_root["paths"], publish);
}

void ModuleManager::publishFiles(const string &path) {
	if(!filesystem::exists(path)) {
		cerr << "Path " << path << " does not exist!" << endl;
		return;
	}

	Json::Value json_array = Json::arrayValue;

	for(const auto &entry : filesystem::directory_iterator(path)) {
		if(entry.path().extension().string() == ".json") {
			json_array.append(entry.path().stem().string());
		}
	}

	mqtt.publish("get/graphs", json_array, true, true);
}

void ModuleManager::loadFromFile(const string &filename) {
	if(!filesystem::exists(filename)) {
		cerr << "File " << filename << " does not exist!" << endl;
		return;
	}

	Json::Value json_root;

	ifstream file(filename);
	file >> json_root;
	file.close();

	deserialize(json_root);
}

void ModuleManager::saveToFile(const string &filename) {
	filesystem::path path(filename);
	filesystem::create_directories(path.parent_path());

	Json::StreamWriterBuilder stream_writer_builder;
	stream_writer_builder["indentation"] = "\t";

	unique_ptr<Json::StreamWriter> stream_writer(stream_writer_builder.newStreamWriter());

	ofstream file(filename);
	stream_writer->write(serialize(), &file);
	file.close();

	publishFiles(path.parent_path().string());
}

void ModuleManager::clear() {
	list<string> module_ids;
	for(const auto& module : modules) {
		module_ids.emplace_back(module.first);
	}

	for(const auto& module_id : module_ids) {
		removeModule(module_id, true);
	}
}

ModuleManager::~ModuleManager() {
	clear();
}
