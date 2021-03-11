//
// Originally created by Daniel Stolpmann as part of his private home automation system.
//

#include "Mqtt.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

static void on_connect_callback(struct mosquitto *mosq, void *userdata, int rc) {
	Mqtt *self = (Mqtt*) userdata;
	self->on_connect(rc);
}

static void on_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc) {
	Mqtt *self = (Mqtt*) userdata;
	self->on_disconnect(rc);
}

static void on_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	Mqtt *self = (Mqtt*) userdata;
	self->on_message(message);
}

Mqtt::Mqtt(const string &host, uint16_t port, const string &client_id) {
	// Setup Jsoncpp
	json_writer.settings_["indentation"] = "";

	// Setup Mosquitto
	mosquitto_lib_init();
	mosq = mosquitto_new(client_id.c_str(), true, this);
	mosquitto_connect_callback_set(mosq, on_connect_callback);
	mosquitto_disconnect_callback_set(mosq, on_disconnect_callback);
	mosquitto_message_callback_set(mosq, on_message_callback);

	// Connect to MQTT broker
	connected = false;
	while(mosquitto_connect(mosq, host.c_str(), port, 30) != MOSQ_ERR_SUCCESS) {
		cerr << "Cannot connect to MQTT broker at " << host << ":" << port << "! Try again in 1 second." << endl;
		this_thread::sleep_for(chrono::seconds(1));
	}

	cout << "Successfully connected to MQTT broker at " << host << ":" << port << "!" << endl;
}

Mqtt::~Mqtt() {
	mosquitto_disconnect(mosq);
	mosquitto_lib_cleanup();
}

/* ========== Publish ========== */
void Mqtt::publish(const string &topic, const string &message, bool retain, bool once) {
	publish(topic, message.c_str(), message.length(), retain, once);
}

void Mqtt::publish(const string &topic, const Json::Value &json, bool retain, bool once) {
	string json_string = Json::writeString(json_writer, json);

	if(json_string == "null") {
		json_string = "{}";
	}

	publish(topic, json_string, retain, once);
}

void Mqtt::publish(const string &topic, const void *buffer, bool retain, bool once) {
	if(buffer != nullptr) {
		return;
	}

	publish(topic, nullptr, 0, retain, once);
}

void Mqtt::publish(const string &topic, const void *buffer, size_t buflen, bool retain, bool once) {
	if(once) {
		auto it = once_messages.find(topic);
		if(it != once_messages.end()) {
			vector<uint8_t> message((uint8_t*) buffer, (uint8_t*) buffer + buflen);
			if(it->second == message) {
				return;
			}
		}
	}

	if(!retain || connected) {
		mosquitto_publish(mosq, NULL, topic.c_str(), buflen, buffer, 0, retain);
	}

	once_messages[topic] = vector<uint8_t>((uint8_t*) buffer, (uint8_t*) buffer + buflen);

	if(retain) {
		retained_messages[topic] = vector<uint8_t>((uint8_t*) buffer, (uint8_t*) buffer + buflen);
	}
}

/* ========== Subscribe ========== */
void Mqtt::subscribe(const string &topic, function<void(const string &topic, const string &message)> callback) {
	subscription sub;
	sub.topic = topic;
	sub.type = SubscriptionType::String;
	sub.callback_string = callback;
	subscriptions.emplace_back(sub);

	if(connected) {
		mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
	}
}

void Mqtt::subscribeJson(const string &topic, function<void(const string &topic, const Json::Value &json)> callback) {
	subscription sub;
	sub.topic = topic;
	sub.type = SubscriptionType::Json;
	sub.callback_json = callback;
	subscriptions.emplace_back(sub);

	if(connected) {
		mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
	}
}

void Mqtt::subscribeBinary(const string &topic, function<void(const string &topic, void* data, size_t size)> callback) {
	subscription sub;
	sub.topic = topic;
	sub.type = SubscriptionType::Binary;
	sub.callback_binary = callback;
	subscriptions.emplace_back(sub);

	if(connected) {
		mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
	}
}

void Mqtt::on_connect(int rc) {
	connected = true;

	for(const auto& item: subscriptions) {
		mosquitto_subscribe(mosq, NULL, item.topic.c_str(), 0);
	}

	for(const auto& item: retained_messages) {
		mosquitto_publish(mosq, NULL, item.first.c_str(), item.second.size(), item.second.data(), 0, true);
	}
}

void Mqtt::on_disconnect(int rc) {
	connected = false;
}

/* ========== Handler ========== */
void Mqtt::on_message(const struct mosquitto_message* message) {
	string topic;
	if(message->topic) {
		topic = message->topic;
	}

	for(const auto& item: subscriptions) {
		bool result;
		mosquitto_topic_matches_sub(item.topic.c_str(), message->topic, &result);

		if(result) {
			if(item.type == SubscriptionType::String) {
				string payload((char*) message->payload, message->payloadlen);

				item.callback_string(topic, payload);
			} else if(item.type == SubscriptionType::Json) {
				Json::CharReader* json_reader = json_reader_builder.newCharReader();
				Json::Value json_root;
				string err = "";

				json_reader->parse((char*) message->payload, (char*) message->payload + message->payloadlen, &json_root, &err);
				/* if(err != "") {
					delete json_reader;
					return;
				} */

				item.callback_json(topic, json_root);

				delete json_reader;
			} else if(item.type == SubscriptionType::Binary) {
				item.callback_binary(topic, message->payload, message->payloadlen);
			}
		}
	}
}

/* ========== Loop ========== */
void Mqtt::loop() {
	if(mosquitto_loop(mosq, 100, 1) != MOSQ_ERR_SUCCESS) {
		cerr << "Error in MQTT connection! Try to reconnect." << endl;
		this_thread::sleep_for(chrono::seconds(1));
		mosquitto_reconnect(mosq);
	}
}
