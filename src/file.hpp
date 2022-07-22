
#ifndef FILE_H
#define FILE_H
class File {
public:

	File(const char * path, int * err);
	virtual ~File();

	void read();
	void write();
private:

	char * _path;
};

#endif

