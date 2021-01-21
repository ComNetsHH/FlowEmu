//
// Originally created by Sebastian Lindner as "Layer.hpp" for the Rekotrans emulator on 11/23/17.
//

#ifndef MODULE_HPP
#define MODULE_HPP

#include <string>

#include <assert.h>

/**
 * Module with no modules to the right or the left.
 * @tparam T
 */
class Module {
	public:
		virtual ~Module() {}

		void setName(const std::string& name) {
			this->name = name;
		}

		std::string getName() const {
			return this->name;
		}

	protected:
		std::string name = "UNNAMED_MODULE";
};

template<typename T> class ModuleHasLeft; // Forward declaration for ModuleHasRight.

/**
 * Module with another module to the right.
 * @tparam T The data type exchanged with the right module.
 */
template<typename T> class ModuleHasRight : virtual public Module {
	public:
		void setRightModule(ModuleHasLeft<T>* rightModule) {
			this->rightModule = rightModule;
		}

		virtual void receiveFromRightModule(T packet) {
			assert(0 && "receiveFromRightModule not implemented.");
		};

		virtual void passToRightModule(T packet) {
			assert(rightModule != nullptr && "Unset right module.");
			rightModule->receiveFromLeftModule(packet);
		}

		virtual T answerToRightModule() {
			assert(0 && "answerToRightModule not implemented.");
			return T();
		};

		virtual T requestFromRightModule() {
			assert(rightModule != nullptr && "Unset right module.");
			return rightModule->answerToLeftModule();
		}

	protected:
		ModuleHasLeft<T>* rightModule = nullptr;
};

/**
 * Module with another module to the left.
 * @tparam T The data type exchanged with the left module.
 */
template<typename T> class ModuleHasLeft : virtual public Module {
	public:
		void setLeftModule(ModuleHasRight<T>* leftModule) {
			this->leftModule = leftModule;
		}

		virtual void receiveFromLeftModule(T packet) {
			assert(0 && "receiveFromLeftModule not implemented.");
		};

		virtual void passToLeftModule(T packet) {
			assert(leftModule != nullptr && "Unset left module.");
			leftModule->receiveFromRightModule(packet);
		}

		virtual T answerToLeftModule() {
			assert(0 && "answerToLeftModule not implemented.");
			return T();
		};

		virtual T requestFromLeftModule() {
			assert(leftModule != nullptr && "Unset left module.");
			return leftModule->answerToRightModule();
		}

	protected:
		ModuleHasRight<T>* leftModule = nullptr;
};

#endif
