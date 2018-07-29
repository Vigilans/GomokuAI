#ifndef GOMOKU_PERSISTENCE_H_
#define GOMOKU_PERSISTENCE_H_
#include <nlohmann/json.hpp>

namespace Gomoku {

	using json = nlohmann::json;

	struct Persistence {
		static json Load(std::string_view name);
		static void Save(std::string_view name, json value);
	};

}

#endif // !GOMOKU_PERSISTENCE_H_
