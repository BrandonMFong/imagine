/**
 * author: Brando
 * date: 7/22/22
 */

#include <stdio.h>
#include <cpplib.hpp>
#include <string.h>

Array<char *> * ARGS = 0;

void Help() {
	char * buf = basename(ARGS->objectAtIndex(0));
	printf("usage: %s <path> to <image type>\n", buf);
}

/**
 * Comparator for the args
 */
ArrayComparisonResult ArgumentCompare(char * a, char * b) {
	if (strcmp(a, b) == 0) return kArrayComparisonResultEquals;
	else if (strcmp(a, b) == -1) return kArrayComparisonResultLessThan;
	else if (strcmp(a, b) == 1) return kArrayComparisonResultGreaterThan;
	else return kArrayComparisonResultUnknown;
}

int main(int argc, char * argv[]) {
	ARGS = new Array<char *>(argv, argc);
	ARGS->setComparator(ArgumentCompare);
	ARGS->print();

	Help();

	delete ARGS;
	return 0;
}

