/**
 * author: Brando
 * date: 7/22/22
 */

#include "appdriver.hpp"

int main(int argc, char * argv[]) {
	int result = 0;
	AppDriver appDriver(argc, argv, &result);
	return appDriver.run();
}

