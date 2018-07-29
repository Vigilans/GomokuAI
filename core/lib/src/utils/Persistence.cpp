#include "Persistence.h"
#include <fstream>
#include <memory>

using namespace std;
using namespace Gomoku;

// 注意data文件夹一定要先存在！
constexpr auto data_path = L"./data/persistence.json";

struct Handle {
	// 需监视内存消耗
	json data;

	static Handle& Inst() {
		static Handle handle;
		return handle;
	}

	Handle() {
		ifstream ifs(data_path);
		if (ifs.is_open()) {
			ifs >> data;
		}
	}

	~Handle() {
		ofstream ofs(data_path);
		if (!ofs.is_open()) {
			throw data_path; // 想用filesystem...但是botzone上大概跑不起来
		}
		ofs << data;
	}
};

json Gomoku::Persistence::Load(std::string_view name) {
	if (Handle::Inst().data.count(name.data())) {
		return Handle::Inst().data[name.data()];
	} else {
		return json();
	}
}

void Gomoku::Persistence::Save(std::string_view name, json value) {
	Handle::Inst().data[name.data()] = std::move(value);
}
