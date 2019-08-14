
#include "files/FilePath.h"
#include "files/FileEnumerator.h"
#include "files/File.h"
#include "files/FileUtil.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	// FilePath
	FilePath fp("/usr/local/annety.log.gz");
	cout << "full:" << fp << endl;
	cout << "dirname:" << fp.dirname() << endl;
	cout << "basename:" << fp.basename() << endl;
	cout << "extension:" << fp.extension() << endl;
	cout << "final extension:" << fp.final_extension() << endl;

	cout << "components:" << endl;
	std::vector<std::string> components;
	fp.get_components(&components);
	for (auto cc : components) {
		cout << "|" << cc << "|";
	}
	cout << endl;

	// FileEnumerator
	cout << "all (*.cc) file:" << endl;
	FileEnumerator enums(FilePath("."), false, FileEnumerator::FILES, "*.cc");
	for (FilePath name = enums.next(); !name.empty(); name = enums.next()) {
		cout << name << endl;
	}

	// File
	FilePath path("annety-text-file.log");
	File f(path, File::FLAG_OPEN_ALWAYS | File::FLAG_APPEND | File::FLAG_READ);
	cout << "write(annety-file.log):"<< f.write(0, "test text", sizeof("test text")) << endl;

	char buf[1024];
	std::cout << "read len:"<< f.read(0, buf, sizeof(buf)) << std::endl;
	std::cout << "read content:" << buf << std::endl;

	cout << "delete file:" << delete_file(path, false) << endl;
	
	// FileUtil
	FilePath path1("annety-text-file1.log");
	std::string cc;
	cout << "write status:" << write_file(path1, "1234567890", sizeof("1234567890")) << endl;
	cout << "read status:" << read_file_to_string(path1, &cc) << endl;
	cout << "read content:" << cc << endl;

	cout << "delete file:" << delete_file(path1, false) << endl;
}
