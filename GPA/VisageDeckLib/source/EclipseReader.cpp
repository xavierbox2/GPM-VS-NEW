#include "EclipseReader.h"

bool EclipseReader::is_little_endian()
{
    short int word = 0x0001;
    char *byte = (char *)&word;
    return(byte[0] ? true : false);
}

void EclipseReader::read_unsigned(unsigned char* word, ifstream &reader, int size)
{
    for (int i = 0; i < size; i++) reader >> word[i];
}