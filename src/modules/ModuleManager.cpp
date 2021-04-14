#include "ModuleManager.hpp"

#include <iostream>
#include <memory>
#include <regex>

#include "delay/FixedDelayModule.hpp"
#include "loss/GilbertElliotLossModule.hpp"
#include "loss/UncorrelatedLossModule.hpp"
#include "queue/FifoQueueModule.hpp"
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
				string type = json_root.get("type", "").asString();

				shared_ptr<Module> new_module;
				if(type == "fixed_delay") {
					new_module = make_shared<FixedDelayModule>(io_service, mqtt, 50);
				} else if(type == "gilbert_elliot_loss") {
					new_module = make_shared<GilbertElliotLossModule>(io_service, mqtt, 0.001, 0.001, 0, 1);
				} else if(type == "uncorrelated_loss") {
					new_module = make_shared<UncorrelatedLossModule>(mqtt, 0.1);
				} else if(type == "fifo_queue") {
					new_module = make_shared<FifoQueueModule>(mqtt, 100);
				} else if(type == "fixed_interval_rate") {
					new_module = make_shared<FixedIntervalRateModule>(io_service, mqtt, chrono::milliseconds(1));
				} else if(type == "trace_rate") {
					new_module = make_shared<TraceRateModule>(io_service, mqtt, "config/traces/Verizon-LTE-short.down", "config/traces/Verizon-LTE-short.up");
				} else {
					cerr << "Unknown module type: " << type << endl;
					return;
				}

				addModule(node_id, new_module, false);
				updateModule(node_id, json_root);
			} else {
				updateModule(node_id, json_root);
			}
		} else {
			removeModule(node_id);
		}
	});

	mqtt.subscribeJson("set/paths", [&](const string &topic, const Json::Value &json_root) {
		for(const auto& json_path : json_root) {
			if(!(json_path.isMember("from") && json_path.isMember("to"))) {
				continue;
			}

			Path path;
			path.from_node_id = json_path["from"].get("node", "").asString();
			path.from_port_id = json_path["from"].get("port", "").asString();
			path.to_node_id = json_path["to"].get("node", "").asString();
			path.to_port_id = json_path["to"].get("port", "").asString();

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

			if(found) {
				continue;
			}

			try {
				path.from_port_info = modules.at(path.from_node_id)->getPort(path.from_port_id);
				path.to_port_info = modules.at(path.to_node_id)->getPort(path.to_port_id);
			} catch(const out_of_range &e) {
				cerr << "Unknown node or port ID in path!" << endl;
				break;
			}

			try {
				path.from_port_info.port->connect(path.to_port_info.port);
				path.to_port_info.port->connect(path.from_port_info.port);
			} catch(const incompatible_port_types &e) {
				cerr << "Cannot connect incompatible port types!" << endl;
				break;
			}

			cout << "Add path!" << endl;
			paths.push_back(path);
		}

		for(auto it = paths.begin(); it != paths.end(); /*++it*/) {
			bool found = false;
			for(const auto& json_path : json_root) {
				if(!(json_path.isMember("from") && json_path.isMember("to"))) {
					continue;
				}

				Path path;
				path.from_node_id = json_path["from"].get("node", "").asString();
				path.from_port_id = json_path["from"].get("port", "").asString();
				path.to_node_id = json_path["to"].get("node", "").asString();
				path.to_port_id = json_path["to"].get("port", "").asString();

				if(it->from_node_id == path.from_node_id &&
				   it->from_port_id == path.from_port_id &&
				   it->to_node_id == path.to_node_id     &&
				   it->to_port_id == path.to_port_id        ) {
					found = true;
					break;
				}
			}

			if(found) {
				++it;
				continue;
			}

			it->from_port_info.port->disconnect();
			it->to_port_info.port->disconnect();

			cout << "Remove path!" << endl;
			it = paths.erase(it);
		}

		mqtt.publish("get/paths", json_root, true, true);
	});

	mqtt.publish("get/paths", Json::arrayValue, true, true);
}

void ModuleManager::addModule(string id, shared_ptr<Module> module, bool publish) {
	if(modules.find(id) != modules.end()) {
		cerr << "Module " + id + " already exists!" << endl;
		return;
	}

	cout << "Add module " + id + "!" << endl;
	modules[id] = module;

	if(publish) {
		mqtt.publish("get/module/" + id, module->serialize(), true, true);
	}
}

void ModuleManager::updateModule(string id, Json::Value json_root, bool publish) {
	if(modules.find(id) == modules.end()) {
		cerr << "Module " + id + " does not exist!" << endl;
		return;
	}

	modules[id]->deserialize(json_root);

	if(publish) {
		mqtt.publish("get/module/" + id, modules[id]->serialize(), true, true);
	}
}

void ModuleManager::removeModule(string id, bool publish) {
	if(publish) {
		mqtt.publish("get/module/" + id, nullptr, true, true);
	}

	if(modules.find(id) == modules.end()) {
		cerr << "Module " + id + " does not exist!" << endl;
		return;
	}

	for(auto it = paths.begin(); it != paths.end(); /*++it*/) {
		if(it->from_node_id == id || it->to_node_id == id) {
			it->from_port_info.port->disconnect();
			it->to_port_info.port->disconnect();

			cout << "Remove path!" << endl;
			it = paths.erase(it);
		} else {
			++it;
		}
	}

	cout << "Remove module " + id + "!" << endl;
	modules.erase(id);
}
