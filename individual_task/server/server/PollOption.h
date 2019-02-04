#include <string>

class PollOption {
public:
	std::string title;
	int votesCount = 0;
	PollOption(std::string title) : title(title) {}
};