/*
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "libphase6.h"

using namespace std;

// Used internally for quick'n'dirty double to integer rounding
int d2i(double d) {
  return d<0?d-.5:d+.5;
}

void subscores(char* x, bool a) {
	if (a == true)
		for (int i = 0; i < strlen((const char*)x); i++)
		{
			if (x[i] == ' ')
			{
				x[i] = '_';
			}
		}
	else
		for (int i = 0; i < strlen((const char*)x); i++)
		{
			if (x[i] == '_')
			{
				x[i] = ' ';
			}
		}
}

// Used internally to decide on whether and if switch question and answer
void phase6::iotagswitch(char* question, char* answer) {
	if (db_[current_].iotag == 1)
	{
		char* h = question;
		srand((unsigned) time(NULL));
		io_ = d2i(rand() % 2);
		if (io_ == 1)
		{
			strcpy(question, answer);
			strcpy(answer, h);
			#ifdef DEBUG
				cout << "***DEBUG***	iotag	";
				cout << "Using iotag - switching question and answer" << endl;
			#endif
		}
	}
}

//Used internally to match searches to the db_
bool similarity(string startstring, string searchstring) {
#ifdef DEBUG
	cout << "***DEBUG*** 	FIND	";
	cout << "SEARCHING " << searchstring;
	cout << " IN " << startstring << endl;
#endif
	if (startstring.length() >= searchstring.length())
	{
		string::size_type loc = startstring.find(searchstring, 0);
		if( loc != string::npos ) {
     return true;
   	} 
   	else {
     return false; 
   }
	}
	else
		return false;
}

//Used internally to resize the db_
vokabel* phase6::resize_db(int newsize) {
	vokabel* newdb;
	newdb = new vokabel[newsize+1];
	
	#ifdef DEBUG
		cout << "***DEBUG*** 	RESIZEDB	";
		cout << "FROM " << count_;
		cout << " TO " << newsize;
		cout << " ITEMS" << endl;
	#endif
	
	for (int i = 1; ((i <= count_) && (i <= newsize)); i++)
	{
		newdb[i] = db_[i];
	}
	
	return newdb;
}

phase6::~phase6() {
	delete db_;
}

// Initializes a phase6 class variable. To be called prio_r to anything else
// Takes an array of type vokabel and a filepath
// Returns a phase6 class variable
phase6::phase6(vokabel* vokabelarray, char* db_file) {
	time_t Zeitstempel;
    tm *nun;
    Zeitstempel = time(0);
    nun = localtime(&Zeitstempel);

	count_=0;
	current_=0;
	io_=0;
	today_=nun->tm_yday;
	filename_=db_file;
	db_=vokabelarray;
	dyndb_=false;
	
#ifdef DEBUG
	cout << "***DEBUG***	INIT	";
	cout << count_ << ",";
	cout << current_ << ",";
	cout << io_ << ",";
	cout << today_ << ",";
	cout << filename_ << ",";
	cout << dyndb_;
	cout << endl;
#endif
}

// Initializes a phase6 class variable. To be called prio_r to anything else
// Takes only a filepath
// Returns a phase6 class variable
phase6::phase6(char* db_file) {
	time_t Zeitstempel;
		tm *nun;
		Zeitstempel = time(0);
		nun = localtime(&Zeitstempel);

cout << "init" << endl;
	count_=0;
	current_=0;
	io_=0;
	today_=nun->tm_yday;
	filename_=db_file;
	db_=new vokabel[0];
	dyndb_=true;

#ifdef DEBUG
	cout << "***DEBUG***	INIT	";
	cout << count_ << ",";
	cout << current_ << ",";
	cout << io_ << ",";
	cout << today_ << ",";
	cout << filename_ << ",";
	cout << dyndb_;
	cout << endl;
#endif
}

// Opens the file defined in the filename_ of the phase6 class variable
// Takes an initialized phase6 class variable
// Returns the number of entries if successful, -1 if not so
int phase6::open() {
  ifstream db(filename_.c_str());
  if (!db)
  {
  	cerr << "No such file or directory" << endl;
  	return -1;
  }
  
  char c;
  string helper;
  
  while ((c = db.get()) != EOF)
  {
  	if (dyndb_ == true)
  	{
  		db_ = resize_db(count_+1);
  		if (db_ == NULL)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	REMOVE	";
				cout << "Cannot allocate memory";
			#endif
			return -1;
		}
  	}
  	count_++;
  	
  	while ((c = db.get()) != '\\') {
  		db_[count_].question+=c;
  	}
  	
  	while ((c = db.get()) != '\\') {
  		db_[count_].answer+=c;
  	}
  	  	
  	(c = db.get()) == 'y' ? db_[count_].iotag=1 : db_[count_].iotag=0;
  	db.get(c);
  	db.get(c);
  	db_[count_].status=atoi(&c);
  	db.get(c);
  	helper="";
  	while ((c = db.get()) != '\n') helper+=c;
  	db_[count_].date=atoi(helper.c_str());
  }
  db.close();
  
#ifdef DEBUG
  for (int i = 1; i <= count_; i++)
  {
  	cout << "***DEBUG***	OPENING	";
  	cout << db_[i].question << "\\";
  	cout << db_[i].answer << "\\";
  	cout << db_[i].iotag  << "\\";
  	cout << db_[i].status  << "\\";
  	cout << db_[i].date << endl;
  }
#endif
  
  return count_;
}

// Saves a phase6 database to the filepath defined in the phase6 class variable
// Takes an initialized phase6 class variable
// Returns the number of saved entries
int phase6::save() {
	ofstream db(filename_.c_str(), ios::trunc);
	
	int i;
  	for(i=1; i <= count_; i++) {
  		db << " " << db_[i].question << '\\' << db_[i].answer << '\\';
  		db_[i].iotag==1 ? db << 'y' << '\\' : db << 'n' << '\\';
  		db << db_[i].status << '\\';
  		db << db_[i].date << '\n';

	#ifdef DEBUG
		cout << "***DEBUG***	SAVING	";
  		cout << db_[i].question << "\\";
  		cout << db_[i].answer << "\\";
  		cout << db_[i].iotag  << "\\";
  		cout << db_[i].status  << "\\";
  		cout << db_[i].date << endl;
	#endif	
  	}
  	
  	db.close();
  	return i;
}

// Searches the database for due vokabel and returns it, and if it can switch it might be switched
// Takes an initialized phase6 class variable and a string for each a question and an answer
// Returns TRUE if it assigned (new) values to the strings
// Returns FALSE if there is nothing to learn in the database
bool phase6::ask(char* question) {
	char answer[255];
	srand((unsigned) time(NULL));
	current_ = d2i(1 + (rand() % (count_)));

	for (int i = current_; i <= count_; i++)
	{
		if (db_[i].date <= today_)
		{
			current_ = i;
			strcpy(question, db_[current_].question.c_str());
			strcpy(answer, db_[current_].answer.c_str());
			
			#ifdef DEBUG
				cout << "***DEBUG***	ASK	";
				cout << "Found due vokabel, using that" << endl;
			#endif
			
			iotagswitch(question, answer);
			return true;
		}			
	}
	
	int j = current_;
	if (db_[current_].date > today_+1)
	{
		while ((db_[current_].date > today_+1) && (current_ != j))
		{
			if (current_ < count_)
			{
				current_++;
			}
			else
			{
				current_=1;
			}
		}
		
		if (current_ == j)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	ASK	";
				cout << "Found no due vokabel" << endl;
			#endif
			return false;
		}
	}
	
	strcpy(question, db_[current_].question.c_str());
	strcpy(answer, db_[current_].answer.c_str());
	
	iotagswitch(question, answer);
	return true;
}

// Searches the database for due picture and returns it
// Takes a string for each the picturename and the corresponding question
// Returns TRUE if it assigned a picturefile and a question
// Returns FALSE if there is nothing to learn in the database
bool phase6::ask(char* picturefile, char* question) {
	srand((unsigned) time(NULL));
	current_ = d2i(1 + (rand() % (count_)));
	
	for (int i = current_; i <= count_; i++)
	{
		if (db_[i].date <= today_)
		{
			current_ = i;
			int pos = db_[current_].question.find("#",0);
			memset(picturefile, '\0', 255);
			db_[current_].question.copy(picturefile,pos);
			db_[current_].question.copy(question,db_[current_].question.length()-pos-1,pos+1);
			
			#ifdef DEBUG
				cout << "***DEBUG***	ASK	";
				cout << "Found due picture, using that" << endl;
				cout << "***DEBUG***	ASK	";
				cout << picturefile << " and " << question << endl;
			#endif
			
			return true;
		}
	}
	
	int j = current_;
	if (db_[current_].date > today_+1)
	{
		while ((db_[current_].date > today_+1) && (current_ != j))
		{
			if (current_ < count_)
			{
				current_++;
			}
			else
			{
				current_=1;
			}
		}
		
		if (current_ == j)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	ASK	";
				cout << "Found no due picture" << endl;
			#endif
			return false;
		}
	}
	
	int pos = db_[current_].question.find("#",0);
	memset(picturefile, '\0', 255);
	db_[current_].question.copy(picturefile,pos);
	db_[current_].question.copy(question,db_[current_].question.length()-pos-1,pos+1);
	
	#ifdef DEBUG
		cout << "***DEBUG***	ASK	";
		cout << picturefile << " and " << question << endl;
	#endif
	
	return true;
}

// Evaluates whether an answer is right or not and sets the phase class variable's correct/wrong tags
// Takes an initialized phase6 class variable and a string for the answer
// Can take an int value to modify the delay until a vokabel is next used, the int is the power for the calculatio_n
// Returns TRUE or FALSE according to whether the answer is right or wrong
bool phase6::answer(char* answer, int power) {
	char* a;
	bool correct;
	
	subscores(answer, true);
	
	int pos = 1;
	int count = 0;
	while ((pos != 0) && (pos <= count_))
	{
		io_ == 0 ? pos = find((char*)db_[current_].question.c_str(), pos)
					  : pos = find((char*)db_[current_].answer.c_str(), pos, ANSWER);
		if (pos != -1)
			count++;
		pos++;
	}
	
	string* as = new string[count];
	string* qs = new string[count];
	
	#ifdef DEBUG
		cout << "***DEBUG***	ANSWER	";
		cout << "NUMBER OF MATCHES: " << count << endl;
	#endif
	
	pos = 1;
	for (int i = 1; i <= count ; i++)
	{
		io_ == 0 ? pos = find((char*)db_[current_].question.c_str(), pos)
					  : pos = find((char*)db_[current_].answer.c_str(), pos, ANSWER);
		as[i-1] = db_[pos].answer;
		qs[i-1] = db_[pos].question;
		#ifdef DEBUG
			cout << "***DEBUG***	ANSWER	MATCHED ";
			cout << as[i-1] << " TO " << qs[i-1] << endl;
		#endif
		pos++;
	}

   for (int i = 0; i < count; i++)
   {
	if (io_ == 1) {
		a = new char[qs[i].length()];
		strcpy(a, qs[i].c_str());
		subscores(a, true);
	}
	else
	{
		a = new char[as[i].length()];
		strcpy(a, as[i].c_str());
		subscores(a, true);
	}
	
  	if (strcmp(a, answer) == 0)
  	{
  		db_[current_].status++;
  		db_[current_].date=today_+1+d2i(pow((double)db_[current_].status, 3) / 3);
  		io_ = 0;
  		return true;
  	}
  	else
  	{
  		db_[current_].status=1;
  		db_[current_].date=today_+1+d2i(pow((double)db_[current_].status, 3) / 3);
  		correct = false;
  	}
   }
   
   io_ = 0;

	return correct;
}

// Evaluates whether an answer is right or not and sets the phase class variable's correct/wrong tags
// Takes the x and y coordinates of the answer
// Can take an int value to modify the delay until a vokabel is next used, the int is the power for the calculation
// Returns TRUE or FALSE according to whether the answer is right or wrong
// Returns the mid x and y value that would have been right if false
bool phase6::answer(int& x, int& y, int power) {
	
	int x0,x1,y0,y1;
	istringstream pos;
	pos.str(db_[current_].answer);
	pos >> x0 >> x1 >> y0 >> y1;
	
	#ifdef DEBUG
			cout << "***DEBUG***	ANSWER	MATCHED ";
			cout << "X0:" << x0 << ",X1:" << x1;
			cout << ",Y0:" << y0 << ",Y1:" << y1 << endl;
	#endif
	
	if ((x0 <= x) && (x1 >= x) && (y0 <= y) && (y1 >= y))
	{
		db_[current_].status++;
		db_[current_].date=today_+1+d2i(pow((double)db_[current_].status, 3) / 3);
		return true;
	}
	else
	{
		db_[current_].status=1;
		db_[current_].date=today_+1;
		x = ((x0+x1)-((x0+x1)%2))/2;
		y = ((y0+y1)-((y0+y1)%2))/2;
	}
	
	return false;
}

int phase6::add(char* picturefile, char* question, int x0, int x1, int y0, int y1) {
	
	if (dyndb_ == true)
	{
		db_ = resize_db(count_+1);
				if (db_ == NULL)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	REMOVE	";
				cout << "Cannot allocate memory";
			#endif
			return -1;
		}
	}
	count_++;
	
	ostringstream ans;
	
	db_[count_].question=picturefile;
	db_[count_].question+="#";
	db_[count_].question+=question;
	ans << x0 << " " << x1 << " " << y0 << " " << y1;
	db_[count_].answer=ans.str();
	db_[count_].iotag=0;
	db_[count_].status=1;
	db_[count_].date=today_+1;
	
#ifdef DEBUG
  	cout << "***DEBUG***	ADD	";
  	cout << db_[count_].question << "\\";
  	cout << db_[count_].answer << "\\";
  	cout << db_[count_].iotag  << "\\";
  	cout << db_[count_].status  << "\\";
  	cout << db_[count_].date << endl;
#endif
	
	return count_;
}

// Adds a new entry to the database of the phase6 class variable
// Takes an initialized phase6 class variable and a string for each a question and an answer
// Can take an int value whether or not question and answer can be switched in training
// Returns the new number of entries if successful
int phase6::add(char* question, char* answer, int iotag) {
	
	if (dyndb_ == true)
	{
		db_ = resize_db(count_+1);
	
		if (db_ == NULL)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	REMOVE	";
				cout << "Cannot allocate memory";
			#endif
			return -1;
		}
	}
	count_++;
	
	db_[count_].question=question;
	db_[count_].answer=answer;
	db_[count_].iotag=iotag;
	db_[count_].status=1;
	db_[count_].date=today_+1;
	
#ifdef DEBUG
	cout << "***DEBUG***	ADD	";
  	cout << db_[count_].question << "\\";
  	cout << db_[count_].answer << "\\";
  	cout << db_[count_].iotag  << "\\";
  	cout << db_[count_].status  << "\\";
  	cout << db_[count_].date << endl;
#endif	
	
	return count_;
}

// Removes an entry from the database of the phase6 class variable
// Takes an initialized phase6 class variable and the positio_n [0..x] of the vokabel
// Returns the new number of entries if successful
int phase6::remove(int position) {
	
	#ifdef DEBUG
		cout << "***DEBUG***	REMOVE	";
		cout << "Removing question ";
		cout << db_[position].question << endl;
	#endif
	
	for (int i = position; i < count_; i++)
	{
		db_[i] = db_[i+1];
	}
	
	if (dyndb_ == true)
	{
		db_ = resize_db(count_-1);	
		if (db_ == NULL)
		{
			#ifdef DEBUG
				cout << "***DEBUG***	REMOVE	";
				cout << "Cannot allocate memory";
			#endif
			return -1;
		}
	}
	count_--;
	
	return count_;
}

// Sorts the entries of the db_ either by answer or question(default)
// Takes an initialized phase6 class variable
// Can take the sort typ with ANSWER or question (defaults to the latter)
// Returns the number of sorts that have taken place
int phase6::sort(int typ) {
	vokabel temp;
	int sort = 0;
	
	switch (typ)
	{
		case QUESTION : 
			if (count_ > 1)
			{
				for (int i = count_; i > 0; i--)
				{
					for (int j = 0; j < i; j++)
					{
						if (db_[j].question > db_[j+1].question)
						{					
							temp = db_[j];
							db_[j] = db_[j+1];
							db_[j+1] = temp;
							sort++;
						}
					}	
				}
			}
			break;
		case ANSWER :
			if (count_ > 1)
			{
				for (int i = count_; i > 0; i--)
				{
					for (int j = 0; j < i; j++)
					{
						if (db_[j].answer > db_[j+1].answer)
						{					
							temp = db_[j];
							db_[j] = db_[j+1];
							db_[j+1] = temp;
							sort++;
						}
					}	
				}
			}
			break;
		default: phase6::sort();
	}
	
#ifdef DEBUG
	cout << "***DEBUG***	SORT	";
	cout << "Finished with " << sort << " sorts" << endl;
	if (sort > 0)
	{
		for (int i = 1; i <= count_ ; i++)
		{
			cout << "***DEBUG***	SORT	";
			cout << db_[i].question << "\\";
  			cout << db_[i].answer << "\\";
  			cout << db_[i].iotag  << "\\";
  			cout << db_[i].status  << "\\";
  			cout << db_[i].date << endl;	
		}	
	}
#endif
	
	return sort;
}

// Finds the first entry matching the search term
// Takes a searchstring as char* variable
// Can take the start position and the sort type with ANSWER or QUESTION (defaults to the latter)
// Returns the position of the found vokabel or -1 if none was found
int phase6::find(char* searchstring, int pos, int typ) {
	for (int i = pos; i <= count_ ; i++)
	{
		if (typ == QUESTION)
		{
			if (similarity((char*)db_[i].question.c_str(),searchstring))
			{
				#ifdef DEBUG
					cout << "***DEBUG***	FIND	";
					cout << "Position: " << i << endl;
					cout << "***DEBUG***	FIND	";
					cout << db_[i].question << "\\";
  					cout << db_[i].answer << "\\";
  					cout << db_[i].iotag  << "\\";
  					cout << db_[i].status  << "\\";
  					cout << db_[i].date << endl;
  				#endif
				return i;
			}
		}
		else
		{
			if (similarity((char*)db_[i].answer.c_str(),searchstring))
			{
				#ifdef DEBUG
					cout << "***DEBUG***	FIND	";
					cout << "Position: " << i << endl;
					cout << "***DEBUG***	FIND	";
					cout << db_[i].question << "\\";
  					cout << db_[i].answer << "\\";
  					cout << db_[i].iotag  << "\\";
  					cout << db_[i].status  << "\\";
 					cout << db_[i].date << endl;
  				#endif
				return i;
			}
		}
	}
	return -1;
}

// Numerous functions to return properties of the phase6 object
int phase6::count() { return count_; }
int phase6::current() { return current_; }
int phase6::io() { return io_; }
int phase6::today() { return today_; }
string phase6::filename() { return filename_; }
void phase6::filename(string newfile) { filename_ = newfile; }
vokabel* phase6::db() { return db_; }
const char* phase6::db_question(int i) { return db_[i].question.c_str(); }
const char* phase6::db_answer(int i) { return db_[i].answer.c_str(); }
int phase6::db_iotag(int i) { return db_[i].iotag; }
int phase6::db_status(int i) { return db_[i].status; }
int phase6::db_date(int i) { return db_[i].date; }
bool phase6::dyndb() { return dyndb_; }
