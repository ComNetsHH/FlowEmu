#ifndef STATISTIC_HPP
#define STATISTIC_HPP

#include <functional>
#include <list>

class Statistic {
	public:
		Statistic() {
			
		}

		void addHandler(std::function<void(double)> handler) {
			handlers.push_back(handler);
		}

		void set(double value) {
			for(const auto& handler : handlers) {
				try {
					handler(value);
				} catch(const std::bad_function_call &e) {
				}
			}
		}

	private:
		std::list<std::function<void(double)>> handlers;
};

#endif
