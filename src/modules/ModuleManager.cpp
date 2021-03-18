#include "ModuleManager.hpp"

#include <iostream>
#include <memory>
#include <regex>

using namespace std;

ModuleManager::ModuleManager(Mqtt &mqtt) : mqtt(mqtt) {
	mqtt.subscribeJson("set/module/+", [&](const string &topic, const Json::Value &json_root) {
		regex r("set\\/module\\/(\\w+)");
		smatch m;
		if(!(regex_search(topic, m, r) && m.size() == 2)) {
			return;
		}
		string node_id = m.str(1);

		/* if(!json_root.empty()) {
			if(modules.find(node_id) == modules.end()) {
				
			} else {
				
			}
		} else {
			removeModule(node_id);
		} */
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

void ModuleManager::addModule(string id, shared_ptr<Module> module) {
	mqtt.publish("get/module/" + id, module->serialize(), true, true);

	if(modules.find(id) != modules.end()) {
		cerr << "Module " + id + " already exists!" << endl;
		return;
	}

	cout << "Add module " + id + "!" << endl;
	modules[id] = module;
}

void ModuleManager::removeModule(string id) {
	mqtt.publish("get/module/" + id, nullptr, true, true);

	if(modules.find(id) == modules.end()) {
		cerr << "Module " + id + " does not exist!" << endl;
		return;
	}

	cout << "Remove module " + id + "!" << endl;
	modules.erase(id);
}
