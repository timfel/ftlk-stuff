#ifndef libphase6_h
#define libphase6_h

#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <time.h>

using namespace std;

#define QUESTION 1
#define ANSWER 2
#define TAG 3
#define PHASE 4
#define DATE 5

//This class is used as database item
class vokabel {
	public:
		string question;
		string answer;
		int iotag;			//Whether or not question and answer can be switched
		int status;			//The status in the "Phase6" system of this item
		int date;			//The date this item is due for asking
};

//This class is used to handle a database
class phase6 {
		private:
			int count_;			//Number of items in the database, starting with 1
			int current_;		//Currently chosen question for training
			string filename_;	//Database file
			int io_;				//Tag whether the current question was switched
			int today_;			//Current date on app start (keep it, in case we learn overnight :P)
			vokabel* db_;		//Database (array of vokabel) !!!DEVS SHOULD NEVER ACCESS THIS DIRECTLY! USE THE PHASE6_GET FUNCTION!!!
			bool dyndb_;			//Is the database dynamically created and needs realloc() ?
			void iotagswitch(char* question, char* answer);
			vokabel* resize_db(int newsize);
		
		public:
			phase6(vokabel* vokabelarray, char* db_file);
			phase6(char* db_file);
			virtual ~phase6();
			int open();
			int save();
			bool ask(char* question);
			bool ask(char* picturefile, char* question);
			bool answer(char* answer, int power = 3);
			bool answer(int& x, int& y, int power = 3);
			int add(char* question, char* answer, int iotag = 0);
			int add(char* picturefile, char* question, int x0, int y0, int x1, int y1);
			int remove(int position);
			int sort(int typ = QUESTION);
			int find(char* searchstring, int pos = 1, int typ = QUESTION);
			
			int count();
			int current();
			int io();
			int today();
			string filename();
			void filename(string newfile);
			vokabel* db();
			const char* db_question(int i);
			const char* db_answer(int i);
			int db_iotag(int i);
			int db_status(int i);
			int db_date(int i);
			bool dyndb();
};

#endif 
