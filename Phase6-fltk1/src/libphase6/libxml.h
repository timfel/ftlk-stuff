#IFNDEF LIBXML_H
	#DEFINE LIBXML_H

	#include <sstream>
	#include <fstream>
	#include <string>

	using namespace std;

	typedef enum {
		XML_READ_OPTION = 1;
		XML_WRITE_OPTION = 2;
		XML_READ_VALUE = 3;
		XML_WRITE_VALUE = 4;
	} xmlMode;

	const char* xml_read(xmlMode& value, fstream file);
	int xml_write(char* text, xmlMode value, fstream file);
	int xml_seek(char* text, fstream file);
	int xml_seek(xmlMode value, fstream file);
	int xml_delete(int line, fstream file);
	int xml_delete_block(int line, fstream file);
	

#ENDIF
