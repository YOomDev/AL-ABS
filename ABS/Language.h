#pragma once

#include <fstream> // file IO
#include <string> // strings
#include <vector> // vector lists

static const bool isEqual(const std::string& one, const std::string& two) { return !(one.size() != two.size() || one.compare(two) != 0); }
static const std::vector<std::string> splitOnce(const std::string& text, const std::string& separator) {
	std::vector<std::string> output;
	int found = text.find(separator);
	if (found >= 0) {
		output.push_back(text.substr(0, found));
		output.push_back(text.substr(found + separator.size(), text.size()));
	} else {
		output.push_back(text);
		output.push_back("");
	}
	return output;
}

struct Language {
private:
	std::string selected;
	std::vector<std::string> identifiers;
	std::vector<std::string> translations;

	void saveToFile() {
		std::ofstream file(("languages/" + selected + ".txt").c_str(), std::ofstream::out | std::ofstream::trunc); // std::ofstream::trunc flag to clear file on opening
		if(file.is_open()) {
			for (int i = 0; i < identifiers.size(); i++) { file << identifiers[i] + " " + translations[i] << std::endl; }
			file.close();
		}
	}
	
public:
	Language() {
		std::ifstream file("settings.txt");
		if (file.is_open()) {
			std::string tmp;
			std::vector<std::string> tmpVec;
			while (!file.eof()) {
				file >> tmp;
				tmpVec = splitOnce(tmp, " ");
				if (isEqual(tmpVec[0], "%LANGUAGE%")) {
					loadLanguage(tmpVec[1]);
					break;
				}
			}
			file.close();
			if (selected.size() > 1) { return; }
		}
		loadLanguage("english");
		findLanguages();
	}

	const std::string getCurrentLanguage() { return selected; }

	void loadLanguage(const std::string& name) {
		if (!isEqual(name, selected)) {
			selected = name;
			std::ifstream file("settings.txt");
			if (file.is_open()) {
				identifiers.clear();
				translations.clear();

				std::string tmp;
				std::vector<std::string> tmpVec;
				while (!file.eof()) {
					file >> tmp;
					tmpVec = splitOnce(tmp, " ");
					identifiers.push_back(tmpVec[0]);
					translations.push_back(tmpVec[1]);
				}
				file.close();
			}
		}
	}

	const std::string getTranslation(const std::string& identifier) {
		for (int i = 0; i < identifiers.size(); i++) {
			if (isEqual(identifier, identifiers[i])) { return translations[i]; }
		}
		identifiers.push_back(identifier);
		translations.push_back(identifier);
		saveToFile();
		return identifier;
	}

	std::vector<std::string> languages;

	const void findLanguages() { // find all the languages in the language folder, just name files without extentions
		languages.clear();

		// TODO

		
		languages.push_back("english"); // tmp
	};
};