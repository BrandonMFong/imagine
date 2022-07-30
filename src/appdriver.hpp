/**
 * author: Brando
 * date: 7/22/22
 */

#include <array.hpp>

class Image;

class AppDriver {
public:
	AppDriver(int argc, char * argv[], int * err);
	virtual ~AppDriver();
	AppDriver * shared();
	int run();
	void help();

	const Array<char *> * args() {
		return this->_args;
	}

private:
	int handleAsCommand(Image * img);
	int handleDetailsCommand(Image * img);
	Array<char *> * _args;
};
