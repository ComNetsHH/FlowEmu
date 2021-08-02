#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <atomic>
#include <functional>
#include <list>
#include <string>

class Parameter {
	public:
		Parameter(double default_value, double min, double max, double step) : value(default_value), min(min), max(max), step(step) {
			
		}

		void addChangeHandler(std::function<void(double)> handler) {
			change_handlers.push_back(handler);
		}

		void callChangeHandlers() {
			for(const auto& handler : change_handlers) {
				try {
					handler(value);
				} catch(const std::bad_function_call &e) {
				}
			}
		}

		void set(double value) {
			this->value.store(value);

			callChangeHandlers();
		}

		double get() {
			return value.load();
		}

		double getMin() {
			return min;
		}

		double getMax() {
			return max;
		}

		double getStep() {
			return step;
		}

	private:
		std::atomic<double> value;

		double min;
		double max;
		double step;

		std::list<std::function<void(double)>> change_handlers;
};

#endif
