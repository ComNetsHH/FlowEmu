#ifndef PORT_HPP
#define PORT_HPP

#include <string>
#include <functional>
#include <exception>

enum Side {left, right};

struct incompatible_port_types : public std::exception {};

class Port {
	public:
		virtual const char* getType() const = 0;

		virtual void connect(Port* connected_port) = 0;
		virtual void disconnect() = 0;
};

template<typename T> class ReceivingPort;

template<typename T> class SendingPort : virtual public Port {
	public:
		const char* getType() const {
			return "sending";
		}

		void connect(Port* connected_port) {
			auto casted_connected_port = dynamic_cast<ReceivingPort<T>*>(connected_port);
			if(casted_connected_port == nullptr) {
				throw incompatible_port_types();
			}

			this->connected_port = casted_connected_port;
		}

		void disconnect() {
			this->connected_port = nullptr;
		}

		void send(T packet) {
			if(this->connected_port == nullptr) {
				return;
			}

			connected_port->send(packet);
		}
	protected:
		ReceivingPort<T>* connected_port = nullptr;
};

template<typename T> class ReceivingPort : virtual public Port {
	public:
		const char* getType() const {
			return "receiving";
		}

		void connect(Port* connected_port) {
			auto casted_connected_port = dynamic_cast<SendingPort<T>*>(connected_port);
			if(casted_connected_port == nullptr) {
				throw incompatible_port_types();
			}

			this->connected_port = casted_connected_port;
		}

		void disconnect() {
			this->connected_port = nullptr;
		}

		void setReceiveHandler(std::function<void(T)> handler) {
			this->receive_handler = handler;
		}

		void send(T packet) {
			try {
				receive_handler(packet);
			} catch(const std::bad_function_call &e) {
			}
		}

	protected:
		SendingPort<T>* connected_port = nullptr;

		std::function<void(T)> receive_handler;
};

template<typename T> class RespondingPort;

template<typename T> class RequestingPort : virtual public Port {
	public:
		const char* getType() const {
			return "requesting";
		}

		void connect(Port* connected_port) {
			auto casted_connected_port = dynamic_cast<RespondingPort<T>*>(connected_port);
			if(casted_connected_port == nullptr) {
				throw incompatible_port_types();
			}

			this->connected_port = casted_connected_port;
		}

		void disconnect() {
			this->connected_port = nullptr;
		}

		void setNotifyHandler(std::function<void()> handler) {
			this->notify_handler = handler;
		}

		T request() {
			if(this->connected_port == nullptr) {
				return T();
			}

			return connected_port->request();
		}

		void notify() {
			try {
				notify_handler();
			} catch(const std::bad_function_call &e) {
			}
		}

	protected:
		RespondingPort<T>* connected_port = nullptr;

		std::function<void()> notify_handler;
};

template<typename T> class RespondingPort : virtual public Port {
	public:
		const char* getType() const {
			return "responding";
		}

		void connect(Port* connected_port) {
			auto casted_connected_port = dynamic_cast<RequestingPort<T>*>(connected_port);
			if(casted_connected_port == nullptr) {
				throw incompatible_port_types();
			}

			this->connected_port = casted_connected_port;
		}

		void disconnect() {
			this->connected_port = nullptr;
		}

		void setRequestHandler(std::function<T()> handler) {
			this->request_handler = handler;
		}

		T request() {
			try {
				return request_handler();
			} catch(const std::bad_function_call &e) {
			}

			return T();
		}

		void notify() {
			if(this->connected_port == nullptr) {
				return;
			}

			return connected_port->notify();
		}

	protected:
		RequestingPort<T>* connected_port = nullptr;

		std::function<T()> request_handler;
};

#endif
