#include "libxml.h"

const char* xml_read(xmlMode& value, fstream file)
{
	char c;
	string read_string;
	
	if (file.get(c) == '<')
	{
		value = XML_READ_OPTION;
		while (file.get(c) != '>')
		{
			read_string.append(c);
		}
		return read_string.c_str();
	}
	else
	{
		value = XML_READ_VALUE;
		while (file.get(c) != '\n')
		{
			read_string.append(c);
		}
		return read_string.c_str();
	}
}
	
int xml_write(char* text, xmlMode value, fstream file)
{
	if (value == XML_WRITE_OPTION)
	{
		file << "<";
		file << text << ">" << '\n';
		return file.pos;
	}
	else if (value == XML_WRITE_VALUE)
	{
		file << text << '\n';
		return file.pos;
	}
	else
	{
		return -1;
	}
}

int xml_seek_text(char* text, fstream file, int pos = 0)
{
	for i in file.getline(
	
	
int xml_delete(int line, fstream file);
int xml_delete_block(int line, fstream file);
