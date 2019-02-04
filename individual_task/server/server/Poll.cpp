#include "Poll.h"

Poll::Poll(std::string theme, std::vector<std::string> options) : theme(theme) {
	for (int index = 0; index < options.size(); ++index) {
		_options.push_back(PollOption(options[index]));
	}
}

bool Poll::addVotes(std::string optionTitle, int count) {
	if (!isOpened) {
		return false;
	}
	for (int index = 0; index < _options.size(); ++index) {
		PollOption& option = _options[index];
		if (option.title.compare(optionTitle) == 0) {
			int newCount = option.votesCount + count;
			if (newCount < 0) {
				return false;
			}
			option.votesCount = newCount;
			return true;
		}
	}
	return false;
}

bool Poll::addOption(std::string option) {
	if (!isOpened) {
		return false;
	}
	_options.push_back(PollOption(option));
	return true;
}

void Poll::close() {
	isOpened = false;
}

std::string Poll::description() {
	int totalVotes = 0;
	for (int index = 0; index < _options.size(); ++index) {
		totalVotes += _options[index].votesCount;
	}
	std::string description;
	description += theme + "\r\n";
	description += (isOpened) ? "1" : "0";
	description += "\r\n";
	char buffer[5];
	for (int index = 0; index < _options.size(); ++index) {
		PollOption& option = _options[index];
		description += option.title + " ";
		description += std::to_string(option.votesCount) + " ";
		if (totalVotes != 0) {
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%.2f", (double)option.votesCount / (double)totalVotes);
			description += buffer;
		}
		else {
			description += "0.00";

		}
		description += "\r\n";
	}
	return description;
}