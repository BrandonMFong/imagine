/**
 * author: Brando
 * date: 7/29/22
 */

#include <stdio.h>
#include <png.hpp>
#include <jpeg.hpp>
#include <image.hpp>
#include <appdriver.hpp>
#include <cpplib.hpp>
#include <cpplib_tests.hpp>

extern "C" {
#include <string.h>
}

int test_PNGIsType(void);
int test_PNGPath(void);
int test_PNG(int * p, int * f) {
	int pass = 0, fail = 0;
	INTRO_TEST_FUNCTION;

	if (!test_PNGIsType()) pass++;
	else fail++;

	if (!test_PNGPath()) pass++;
	else fail++;

	if (p) *p = pass;
	if (f) *f = fail;

	return 0;
}

int test_JPEGIsType(void);
int test_JPEGPath(void);
int test_JPEG(int * p, int * f) {
	int pass = 0, fail = 0;
	INTRO_TEST_FUNCTION;

	if (!(test_JPEGIsType())) pass++;
	else fail++;

	if (!(test_JPEGPath())) pass++;
	else fail++;

	if (p) *p = pass;
	if (f) *f = fail;

	return 0;
}

int test_Image(int * p, int * f) {
	int pass = 0, fail = 0;
	INTRO_TEST_FUNCTION;

	if (p) *p = pass;
	if (f) *f = fail;

	return 0;
}

int test_AppDriver(int * p, int * f) {
	int pass = 0, fail = 0;
	INTRO_TEST_FUNCTION;

	if (p) *p = pass;
	if (f) *f = fail;

	return 0;
}

int main() {
	int pass = 0, fail = 0, tp = 0, tf = 0;

	printf("\n---------------------------\n");
	printf("\nStarting PNG tests...\n\n");
	test_PNG(&pass, &fail);
	tp += pass; tf += fail;

	printf("\nPass: %d\n", pass);
	printf("Fail: %d\n", fail);

	printf("\n---------------------------\n");
	printf("\nStarting JPEG tests...\n\n");
	test_JPEG(&pass, &fail);
	tp += pass; tf += fail;

	printf("\nPass: %d\n", pass);
	printf("Fail: %d\n", fail);

	printf("\n---------------------------\n");
	printf("\nStarting Image tests...\n\n");
	test_Image(&pass, &fail);
	tp += pass; tf += fail;

	printf("\nPass: %d\n", pass);
	printf("Fail: %d\n", fail);

	printf("\n---------------------------\n");
	printf("\nStarting AppDriver tests...\n\n");
	test_AppDriver(&pass, &fail);
	tp += pass; tf += fail;

	printf("\nPass: %d\n", pass);
	printf("Fail: %d\n", fail);

	printf("\n---------------------------\n\n");

	printf("Total pass: %d\n", tp);
	printf("Total fail: %d\n", tf);

	return 0;
}

int test_PNGIsType(void) {
	int result = 0;
	const char * path = "test.png";

	if (!PNG::isType(path)) result = 1;

	if (result == 0) {
		path = "test.txt";

		if (PNG::isType(path)) result = 1;
	}

	PRINT_TEST_RESULTS(!result);

	return result;
}

int test_PNGPath(void) {
	int result = 0;
	const char * path = "hello/world/test.png";

	PNG * png = new PNG(path, &result);

	if (result) {
		printf("Error with initializing png file: %d\n", result);
	} else if (strcmp(png->path(), path)) {
		printf("%s != %s\n", png->path(), path);
	} else {
		delete png;
	}

	PRINT_TEST_RESULTS(!result);

	return result;
}

int test_JPEGIsType(void) {
	int result = 0;
	const char * path = "test.jpeg";

	if (!JPEG::isType(path)) result = 1;

	if (!result) {
		path = "test.txt";
		if (JPEG::isType(path)) result = 1;
	}

	PRINT_TEST_RESULTS(!result);
	return result;
}

int test_JPEGPath(void) {
	int result = 0;
	const char * path = "hello/world/jpeg.jpeg";

	JPEG * jpeg = new JPEG(path, &result);

	if (result) {
		printf("Error creating jpeg object %d\n", result);
	} else if (strcmp(path, jpeg->path())) {
		printf("%s != %s\n", path, jpeg->path());
	} else {
		delete jpeg;
	}

	PRINT_TEST_RESULTS(!result);
	return result;
}

