#include <string>
#include <vector>
#include "PollOption.h"

class Poll {
private:
	bool isOpened = true;
	std::vector<PollOption> _options;
public:
	std::string theme;
	Poll(std::string theme, std::vector<std::string> options);
	bool addVotes(std::string optionTitle, int count);
	bool addOption(std::string option);
	void close();
	std::string description();
};