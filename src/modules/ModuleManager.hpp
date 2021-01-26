#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <list>

#include "Module.hpp"

class ModuleManager {
	public:
		void push_back(Module *module);
	private:
		std::list<Module*> modules;
};

#endif
