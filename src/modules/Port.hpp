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

			notify();
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
