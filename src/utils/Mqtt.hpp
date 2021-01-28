//
// Originally created by Daniel Stolpmann as part of his private home automation system.
//

#ifndef MQTT_HPP
#define MQTT_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <list>
#include <functional>

#include <mosquitto.h>
#include <json/json.h>

class Mqtt {
	public:
		Mqtt(const std::string &host, uint16_t port, const std::string &client_id);
		~Mqtt();

		void publish(const std::string &topic, const std::string &message, bool retain = false, bool once = false);
		void publish(const std::string &topic, const Json::Value &json, bool retain = false, bool once = false);
		void publish(const std::string &topic, const void* buffer, size_t buflen, bool retain = false, bool once = false);

		void subscribe(const std::string &topic, std::function<void(const std::string &topic, const std::string &message)> callback);
		void subscribeJson(const std::string &topic, std::function<void(const std::string &topic, const Json::Value &json)> callback);
		void subscribeBinary(const std::string &topic, std::function<void(const std::string &topic, void* data, size_t size)> callback);

		void loop();

		void on_connect(int rc);
		void on_disconnect(int rc);
		void on_message(const struct mosquitto_message* message);

	private:
		struct mosquitto *mosq;
		bool connected;

		std::map<std::string, std::vector<uint8_t>> once_messages;
		std::map<std::string, std::vector<uint8_t>> retained_messages;

		enum class SubscriptionType : uint8_t {
			String,
			Json,
			Binary
		};

		struct subscription {
			std::string topic;
			SubscriptionType type;
			std::function<void(const std::string &topic, const std::string &message)> callback_string;
			std::function<void(const std::string &topic, const Json::Value &json)> callback_json;
			std::function<void(const std::string &topic, void* data, size_t size)> callback_binary;
		};

		std::list<subscription> subscriptions;

		Json::StreamWriterBuilder json_writer;
		Json::CharReaderBuilder json_reader_builder;
};

#endif
