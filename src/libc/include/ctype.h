#pragma once

void loadLookupTable();

extern "C" int isalnum(int ch);
extern "C" int isalpha(int ch);
extern "C" int islower(int ch);
extern "C" int isupper(int ch);
extern "C" int isdigit(int ch);
extern "C" int isxdigit(int ch);
extern "C" int iscntrl(int ch);
extern "C" int isgraph(int ch);
extern "C" int isspace(int ch);
extern "C" int isblank(int ch);
extern "C" int isprint(int ch);
extern "C" int ispunct(int ch);