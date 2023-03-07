/**
 * author: Brando
 * date: 7/22/22
 */

#ifndef APPDRIVER_HPP
#define APPDRIVER_HPP

#include <bflibcpp/array.hpp>

class Image;

class AppDriver {
public:
	AppDriver(int argc, char * argv[], int * err);
	virtual ~AppDriver();
	AppDriver * shared();
	int run();
	void help();

	const BF::Array<const char *> * args();

private:
	int handleAsCommand(Image * img);
	int handleDetailsCommand(Image * img);
	BF::Array<const char *> * _args;
};

#endif // APPDRIVER_HPP

