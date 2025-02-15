#pragma once

#include "Ini.hpp"

#include <boost/algorithm/string.hpp>

struct LocalForm {
	RE::FormID localID;
	std::string modname;

	operator RE::FormID() const {
		if (localID >= 0xFF000000) return localID;
		return RE::TESDataHandler::GetSingleton()->LookupFormID(localID, modname);
	}

	RE::FormID Get() const { if (localID >= 0xFF000000) return localID;  return RE::TESDataHandler::GetSingleton()->LookupFormID(localID, modname); }
};

class ConfigManager : public SINGLETON<ConfigManager>{
public:
	static ConfigManager& getInstance() {
		static ConfigManager instance("Data/SKSE/Plugins/CraftingPullFromContainers.ini");
		return instance;
	}

	bool HasKey(const std::string& key, const std::optional<std::string> setToIfNotExist = std::nullopt) {
		bool exist = cfg["CraftingPullFromContainers"].isKeyExist(key);
		return exist ? exist : setToIfNotExist.has_value() ? Modify(key, setToIfNotExist.value()) : exist;
	}

	bool Modify(const std::string& Key, const std::string& Value, const std::string& comment = "") {
		return cfg.modify("CraftingPullFromContainers", Key, Value, comment);
	}

	std::string GetKey(const std::string& Key) {
		return cfg["CraftingPullFromContainers"][Key];
	}

	void GetFormList(const std::string& Key, std::vector<LocalForm>& res) {
		std::string content = GetKey(Key);
		std::vector<std::string> elements{};
		boost::split(elements, content, boost::is_any_of(","));
		for (auto&& e : elements) {
			std::vector<std::string> item{};
			boost::split(item, e, boost::is_any_of("~"));
			if (item.size() != 2) {
				logger::error("Error processing \"{}\" got less or more than two parts", e);
				continue;
			}

			RE::FormID id = 0;
			std::string plugin{};
			try {
				id = std::stoll(item[0], nullptr, 16);
				plugin = item[1];
			}
			catch (...) {
				logger::error("Error processing \"{}\" std::stoll error", item[0]);
				continue;
			}

			if (id >= 0xFF000000) {
				res.push_back({id, ""});
				logger::info("Loaded Dynamic {:X}", id);
			}
			else {
				auto&& formID = RE::TESDataHandler::GetSingleton()->LookupFormID(id, plugin);
				if (formID) res.push_back({id, plugin});
				else logger::error("Failed to lookup form {:X}:{}", id, plugin);

				logger::info("Loaded Form {:X}", formID);
			}
		}
	}

	void AddToFormList(const std::string& Key, std::vector<LocalForm>& tolist, RE::TESForm* form) {
		auto&& localID = form->IsDynamicForm() ? form->GetFormID() : form->GetLocalFormID();
		auto&& file = form->GetFile(0);

		tolist.push_back({ localID, file ? std::string(file->GetFilename()) : "" });

		std::string e = std::format("0x{:X}~{}", localID, file ? file->GetFilename() : "");

		auto content = GetKey(Key);
		if (content.empty()) content += e;
		else content += "," + e;

		Modify(Key, content);
	}

	bool RemoveFromFormList(const std::string& Key, std::vector<LocalForm>& fromlist, RE::FormID formID) {
		auto it = std::remove_if(fromlist.begin(), fromlist.end(),
			[&](LocalForm& x) { return x == formID; });

		if (formID >= 0xFF000000) {
			auto content = GetKey(Key);
			std::string e = std::format("0x{:X}~", it->localID);

			auto idx = content.find(e);
			auto size = e.size();
			if (idx != std::string::npos) {
				if (content.at(idx + e.size()) == ',') size++;
				content.erase(idx, size);

				Modify(Key, content);

				fromlist.erase(it);
			}
			else {
				logger::error("Trying to remove non-existent form {}", e);
				return false;
			}
		}
		else {
			auto content = GetKey(Key);
			std::string e = std::format("0x{:X}~{}", formID, it->modname);

			auto idx = content.find(e);
			auto size = e.size();
			if (idx != std::string::npos) {
				if (content.at(idx + e.size()) == ',') size++;
				content.erase(idx, size);

				Modify(Key, content);

				fromlist.erase(it);
			}
			else {
				logger::error("Trying to remove non-existent form {}", e);
				return false;
			}
		}

		return true;
	}

private:

	ConfigManager(const char* path) : cfg(path) {}

	inicpp::IniManager cfg;
};