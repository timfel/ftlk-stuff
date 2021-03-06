/*
 *			This program is free software; you can redistribute it and/or modify
 *			it under the terms of the GNU General Public License as published by
 *			the Free Software Foundation; either version 2 of the License, or
 *			(at your option) any later version.
 *
 *			This program is distributed in the hope that it will be useful,
 *			but WITHOUT ANY WARRANTY; without even the implied warranty of
 *			MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *			GNU General Public License for more details.
 *
 *			You should have received a copy of the GNU General Public License
 *			along with this program; if not, write to the Free Software
 *			Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * 		generated by Fast Light User Interface Designer (fluid) version 2.1000 
 */

#include "flPhase6.h"
#include "libphase6/libphase6.h"
#include <FL/Fl.h>
using namespace std;

Fl_Select_Browser *fragebr=(Fl_Select_Browser *)0;
Fl_Select_Browser *antwortbr=(Fl_Select_Browser *)0;
Fl_Check_Button *umkehrstellen=(Fl_Check_Button *)0;
Fl_Input *antwort=(Fl_Input *)0;
Fl_Input *antwort_angeben=(Fl_Input *)0;
Fl_Input *frage_angeben=(Fl_Input *)0;
Fl_Box *frage=(Fl_Box *)0;
Fl_Box *meldungen=(Fl_Box *)0;
Fl_Button *pic_b=(Fl_Button *)0;
Fl_Progress *fortschritt=(Fl_Progress *)0;
Fl_Value_Output *fehler=(Fl_Value_Output *)0;
Fl_Value_Output *richtig=(Fl_Value_Output *)0;
Fl_Value_Slider *slider=(Fl_Value_Slider *)0;
Fl_Window *win=(Fl_Window *)0;
Fl_Window *win_add=(Fl_Window *)0;
Fl_Window *win_edit=(Fl_Window *)0;
Fl_Window *win_liste=(Fl_Window *)0;
Fl_Window *win_start=(Fl_Window *)0;

Fl_Menu_Item menu[] = {
 {"Datei", 0,	0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {"Neu", 0x4006e,	(Fl_Callback*)cb_new, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Öffnen", 0x4006f,	(Fl_Callback*)cb_open, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Speichern", 0x40073,	(Fl_Callback*)cb_save, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Speichern unter...", 0x50073,	(Fl_Callback*)cb_save_as, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
 {"Schließen", 0x40071,	(Fl_Callback*)cb_exit, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {"Bearbeiten", 0,	0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {"Hinzuf\303\274gen", 0x40061,	(Fl_Callback*)cb_add, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Bild hinzufügen", 0x40070,	(Fl_Callback*)cb_add_pic, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Editieren", 0x40065,	(Fl_Callback*)cb_liste, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Suchen", 0x40066,	(Fl_Callback*)cb_suche, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {"Test starten", 0x40072,	(Fl_Callback*)cb_start, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

int wdh;
bool q_eingabe;
bool q_abbrechen = false;
vokabel vokabeldb[255];
bool changed = false;
int falschw = 0;
int richtigw = 0;
int coords[4];
phase6 *phase=(phase6 *)0;

inline int double2integer(double d) {
	return d<0?d-.5:d+.5;
}

int main (int argc, char **argv) {
	Fl::visual(FL_RGB); 
	fl_register_images();
	Fl::get_system_colors();
#ifndef WIN32
	Fl::scheme("gtk+");
#endif
#ifdef MACX
	Fl::scheme("plastic");
#endif
	
	Fl_Window* w;
	 {Fl_Window* o = win = new Fl_Window(200, 200, 585, 235);
		w = o;
		o->begin();
		antwort = new Fl_Input(70, 116, 300, 24, "Antwort");
		 {Fl_Box* o = frage = new Fl_Box(20, 81, 500, 24, "Frage:");
			o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
		}
		 {Fl_Progress* o = fortschritt = new Fl_Progress(0, 215, 585, 20, "Fortschritt");
			o->color((Fl_Color)51);
			o->selection_color((Fl_Color)187);
			o->align(FL_ALIGN_LEFT);
		}
		 {Fl_Menu_Bar* o = new Fl_Menu_Bar(0, 0, 600, 20);
			o->box(FL_THIN_UP_BOX);
			o->menu(menu);
		}
		 {Fl_Return_Button* o = new Fl_Return_Button(370, 115, 200, 24, "Antwort abgeben...");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_abgeben);
		}
		 {Fl_Button* o = new Fl_Button(370, 140, 200, 24, "Test abbrechen...");
			o->callback((Fl_Callback*)cb_test_abbrechen);
		}
		fehler = new Fl_Value_Output(255, 171, 35, 24, "Fehler:");
		richtig = new Fl_Value_Output(295, 171, 35, 24, "Richtig");
		richtig->align(FL_ALIGN_RIGHT);
		 {Fl_Box* o = meldungen = new Fl_Box(5, 25, 585, 50);
			o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
		}
		o->end();
	}
	
	win->show(argc, argv);
	
	return	Fl::run();
}



void cb_abbrechen(Fl_Button*, void* w) {
	Fl_Window* wi = (Fl_Window*)w;
	wi->hide();
}

void cb_abgeben(Fl_Return_Button*, void*) {
	q_eingabe=true;
}

void cb_add(Fl_Menu_Item*, void*) {
	Fl_Window* w;
	 {Fl_Window* o = win_add = new Fl_Window(340, 190, "Item hinzuf\303\274gen...");
		w = o;
		o->begin();
		frage_angeben = new Fl_Input(105, 15, 205, 25, "Frage:");
		antwort_angeben = new Fl_Input(105, 65, 205, 25, "Antwort:");
		umkehrstellen = new Fl_Check_Button(25, 100, 285, 24, "Kann auch umgekehrt gestellt werden");
		 {Fl_Return_Button* o = new Fl_Return_Button(55, 146, 100, 24, "Hinzuf\303\274gen");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_add_now);
		}
		 {Fl_Button* o = new Fl_Button(180, 146, 100, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
		o->hotspot(o);
	}
	w->show();
}

void cb_add_pic(Fl_Menu_Item*, void*) {
	Fl_Window* w;
	 {Fl_Window* o = win_add = new Fl_Window(340, 120, "Bild hinzuf\303\274gen...");
		w = o;
		o->begin();
		frage_angeben = new Fl_Input(105, 15, 205, 25, "Frage:");
		pic_b = new Fl_Button(105, 45, 205, 25, "Bild suchen...");
		pic_b->callback((Fl_Callback*)cb_open_pic, coords);
		 {Fl_Return_Button* o = new Fl_Return_Button(55, 76, 100, 24, "Hinzuf\303\274gen");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_add_pic_now);
		}
		 {Fl_Button* o = new Fl_Button(180, 76, 100, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
		o->hotspot(o);
	}
	w->show();
}

void cb_add_pic_now(Fl_Menu_Item*, void*) {
	win_add->hide();
	phase->add((char*)pic_b->label(), (char*)frage_angeben->value(), 
						coords[0], coords[1], coords[2], coords[3]);
	if ((win_liste != (Fl_Window *)0) && (win_liste->shown()))
	{
		win_liste->hide();
		cb_liste((Fl_Menu_Item*)0,(void*)0);
	}
	for (int i = 0; i < 4; i++)
	{
		coords[i] = 0;
	}
	changed = true;
}

void cb_add_now(Fl_Return_Button*, void*) {
	win_add->hide();
	if (umkehrstellen->value() == true)
		phase->add((char*)frage_angeben->value(),(char*)antwort_angeben->value(),1);
	else
		phase->add((char*)frage_angeben->value(),(char*)antwort_angeben->value(),0);
		
	if ((win_liste != (Fl_Window *)0) && (win_liste->shown()))
	{
		win_liste->hide();
		cb_liste((Fl_Menu_Item*)0,(void*)0);
	}
	changed = true;
}

void cb_browser(Fl_Select_Browser* b, void*) {
	antwortbr->value(b->value());
	fragebr->value(b->value());
}

void cb_edit(Fl_Menu_Item*, void*) {
	Fl_Window* w;
	 {Fl_Window* o = win_edit = new Fl_Window(340, 190, "Item bearbeiten...");
		w = o;
		o->begin();
		frage_angeben = new Fl_Input(105, 15, 205, 25, "Frage angeben:");
		 frage_angeben->value((char*)phase->db_question(fragebr->value()));
		antwort_angeben = new Fl_Input(105, 65, 205, 25, "Antwort angeben:");
		 antwort_angeben->value((char*)phase->db_answer(fragebr->value()));
		umkehrstellen = new Fl_Check_Button(25, 100, 285, 24, "Kann auch umgekehrt gestellt werden");
		int iotag = phase->db_iotag(fragebr->value());
		 iotag == 1 ? umkehrstellen->value(true) : umkehrstellen->value(false);
		 
		 {Fl_Return_Button* o = new Fl_Return_Button(55, 146, 100, 24, "Okay");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_edit_now);
		}
		 {Fl_Button* o = new Fl_Button(180, 146, 100, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
		o->hotspot(o);
	}
	w->show();
}

void cb_searchedit(Fl_Menu_Item*, void*) {
	int pos = 0;
	for (int i = 0; i < fragebr->value(); i++)
		{
			pos = phase->find((char*)fragebr->text(fragebr->value()), pos);
			pos++;
		}
	cout << pos << endl;
	
	Fl_Window* w;
	 {Fl_Window* o = win_edit = new Fl_Window(340, 190, "Item bearbeiten...");
		w = o;
		o->begin();
		frage_angeben = new Fl_Input(105, 15, 205, 25, "Frage angeben:");
		 frage_angeben->value((char*)phase->db_question(pos));
		antwort_angeben = new Fl_Input(105, 65, 205, 25, "Antwort angeben:");
		 antwort_angeben->value((char*)phase->db_answer(pos));
		umkehrstellen = new Fl_Check_Button(25, 100, 285, 24, "Kann auch umgekehrt gestellt werden");
		int iotag = phase->db_iotag(pos);
		 iotag == 1 ? umkehrstellen->value(true) : umkehrstellen->value(false);
		 
		 {Fl_Return_Button* o = new Fl_Return_Button(55, 146, 100, 24, "Okay");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_edit_now);
		}
		 {Fl_Button* o = new Fl_Button(180, 146, 100, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
		o->hotspot(o);
	}
	w->show();
}

void cb_edit_now(Fl_Return_Button*, void*) {
	win_liste->hide();
	win_edit->hide();
	phase->remove(fragebr->value());
	if (umkehrstellen->value() == true)
			phase->add((char*)frage_angeben->value(),(char*)antwort_angeben->value(),1);
	 	else
			phase->add((char*)frage_angeben->value(),(char*)antwort_angeben->value(),0);
		cb_liste((Fl_Menu_Item*)0,(void*)0);
		changed = true;
}

void cb_exit(Fl_Menu_Item*, void*) {
	q_abbrechen = true;
	q_eingabe = true;
	
	if (changed)
	{
		Fl_Window* w;
		{Fl_Window* o = new Fl_Window(120,60, "Änderungen speichern?");
		w = o;
		o->begin();
			{Fl_Return_Button* o = new Fl_Return_Button(0,35,60,25, "Ja");
		 	o->callback((Fl_Callback*)cb_save, (void*)(w));
		}
			{Fl_Button* o = new Fl_Button(60,35,60,25, "Nein");
				o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
	}
		{Fl_Box* o = new Fl_Box(5,5,110,30);
			o->label("Änderungen speichern?");
			o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
	}
	o->end();
	o->hotspot(o);}
	w->show();
	while (w->shown()) Fl::wait();
	}

	win->hide();
}

void cb_liste(Fl_Menu_Item*, void*) {
	phase->sort();
	Fl_Window* w;
	 {Fl_Window* o = win_liste = new Fl_Window(50, 50, 650, 370, "Liste bearbeiten...");
		w = o;
		o->begin();
		fragebr = new Fl_Select_Browser(5, 5, 315, 330);
		 fragebr->begin();
		 	for (int i = 1; i <= phase->count() ; i++)
		 	{
		 		fragebr->add((char*)phase->db_question(i));
		 	}
		 fragebr->callback((Fl_Callback*)cb_browser);
	 fragebr->end();
	
		antwortbr = new Fl_Select_Browser(330, 5, 315, 330);
		 antwortbr->begin();
		 	for (int i = 1; i <= phase->count() ; i++)
		 	{
		 		antwortbr->add((char*)phase->db_answer(i));
		 	}
		 antwortbr->callback((Fl_Callback*)cb_browser);
		 antwortbr->end();
		
		 {Fl_Button* o = new Fl_Button(15, 344, 80, 24, "Hinzuf\303\274gen");
			o->callback((Fl_Callback*)cb_add);
		}
		 {Fl_Button* o = new Fl_Button(195, 344, 80, 24, "Entfernen");
			o->callback((Fl_Callback*)cb_remove);
		}
		 {Fl_Button* o = new Fl_Button(375, 344, 80, 24, "Bearbeiten");
			o->callback((Fl_Callback*)cb_edit);
		}
		 {Fl_Button* o = new Fl_Button(555, 344, 80, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
	}
	w->show();
}

void cb_new(Fl_Menu_Item*, void*) {
	Fl_Native_File_Chooser chooser;
	chooser.title("Wähle die Datenbank");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("DB Files\t*.6db");
	chooser.directory(getenv("HOME"));
	switch ( chooser.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", chooser.errmsg()); break;
	case	1: fprintf(stderr, "*** CANCEL\n"); break;
	default:
		string file = chooser.filename();
		string::size_type loc = file.find(".6db", file.length()-5);
		if( loc != string::npos ) {
		 phase = new phase6((char*)file.c_str());
	 	} 
	 	else {
	 	 file += ".6db";
	 	 phase = new phase6((char*)file.c_str());
	 	}
	}
}

void cb_open(Fl_Menu_Item*, void*) {
	Fl_Native_File_Chooser chooser;
	chooser.title("Wähle die Datenbank");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("DB Files\t*.6db");
	chooser.directory(getenv("HOME"));
	switch ( chooser.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", chooser.errmsg()); break;
	case	1: fprintf(stderr, "*** CANCEL\n"); break;
	default:
			phase = new phase6((char*)chooser.filename());
			phase->open();
	}
}

void cb_open_pic(Fl_Button*, void* c) {
	string newfile;
	cout << coords[0] << coords[1] << coords[2] << coords[3] << endl;
	
	Fl_Native_File_Chooser chooser;
	chooser.title("Wähle Bild");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Pictures\t*.{png,gif,jpg,xpm}");
	chooser.directory(getenv("HOME"));
	switch ( chooser.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", chooser.errmsg()); break;
	case	1: fprintf(stderr, "*** CANCEL\n"); break;
	default: newfile = chooser.filename();
	}
	
	if (newfile.length() > 0) {
		pic_b->label(newfile.c_str());
		
		Fl_Shared_Image* img;
		img = (Fl_Shared_Image::get(newfile.c_str()));
		Fl_Image *temp;
		temp = img->copy(400, 400 * img->h() / img->w()); 
		img->release();
		img = (Fl_Shared_Image *)temp;
		
		Fl_Window* w = new Fl_Window(400,400 * img->h() / img->w(),"Wähle die obere linke und untere rechte Ecke");
		w->begin();
			Fl_Button* b = new Fl_Button(0,0,400,400 * img->h() / img->w());
			b->image(img);
			b->callback((Fl_Callback*)cb_pic_rect, coords);
			b->redraw();
		w->end();
		w->show();
		
		while (win_add->shown()) {
			if (coords[0] != 0)
			{
				fl_line(coords[0],coords[2],coords[0]+30,coords[2]);
				fl_line(coords[0],coords[2],coords[0],coords[2]+30);
			}
			if (coords[1] != 0)
			{
				fl_line(coords[0],coords[2],coords[1],coords[2]);
				fl_line(coords[0],coords[2],coords[0],coords[3]);
				fl_line(coords[1],coords[2],coords[1],coords[3]);
				fl_line(coords[0],coords[3],coords[1],coords[3]);
			}
			Fl::wait();
		}
		w->hide();
	}
}

void cb_pic_rect(Fl_Button* b, void* c) {	
	if ((coords[0] == 0) && (coords[1] == 0) && (coords[2] == 0) && (coords[3] == 0))
	{
		coords[0] = (int)Fl::event_x();
		coords[2] = (int)Fl::event_y();
		cout << "x0 = " << coords[0] << ", y0 = " << coords[2] << endl;
		fl_line(coords[0],coords[2],coords[0]+5,coords[2]);
				fl_line(coords[0],coords[2],coords[0],coords[2]+5);
	}
	else
	{
		coords[1] = (int)Fl::event_x();
		coords[3] = (int)Fl::event_y();
		cout << "x1 = " << coords[1] << ", y1 = " << coords[3] << endl;
	}
}

void cb_remove(Fl_Menu_Item*, void*) {
	win_liste->hide();
	phase->remove((fragebr->value()));
	cb_liste((Fl_Menu_Item*)0,(void*)0);
	changed = true;
}

void cb_save(Fl_Menu_Item*, void* w) {
	//Fl_Window* wi = (Fl_Window*)w;
	phase->save();
	changed = false;
}

void cb_save_as(Fl_Menu_Item*, void*) {
	Fl_Native_File_Chooser chooser;
	chooser.title("Wähle die Datenbank");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("DB Files\t*.6db");
	chooser.directory(getenv("HOME"));
	switch ( chooser.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", chooser.errmsg()); break;
	case	1: fprintf(stderr, "*** CANCEL\n"); break;
	default:
		string file = chooser.filename();
		string::size_type loc = file.find(".6db", file.length()-5);
		if( loc != string::npos ) {
		 phase->filename((char*)file.c_str());
	 	} 
	 	else {
	 	 file += ".6db";
	 	 phase->filename((char*)file.c_str());
	 	}
	}
	changed = false;
}

void cb_start(Fl_Menu_Item*, void*) {
	Fl_Window* w;
	 {Fl_Window* o = win_start = new Fl_Window(440, 95, "Training starten...");
		w = o;
		o->begin();
		 {Fl_Value_Slider* o = slider = new Fl_Value_Slider(60, 25, 324, 25, "Wieviele Vokabeln sollen abgefragt werden?");
			o->minimum(1);
			o->step(1);
			slider->align(FL_ALIGN_CENTER);
			o->value(1);
			o->slider_size(10);
			o->align(FL_ALIGN_TOP);
			o->maximum(phase->count());
			slider->type(3);
		}
		 {Fl_Return_Button* o = new Fl_Return_Button(0, 70, 220, 25, "Starten");
			o->shortcut(0xff0d);
			o->callback((Fl_Callback*)cb_start_now, (void*)(slider));
		}
		 {Fl_Button* o = new Fl_Button(220, 70, 220, 25, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(w));
		}
		o->end();
		o->resizable(o);
		o->hotspot(o);
	}
	w->show();
}

void cb_start_now(Fl_Return_Button*, Fl_Value_Slider*) {
	int wdh = double2integer(slider->value());
	char q[255];
	
	//char pic[255];
	//int x, y;
	
	string h;
	string h2;
	
	win_start->hide();
	
	for (int i = 1; i <= wdh; i++)
	{
		antwort->value("");
		if ((phase->ask(q) == false) || (q_abbrechen))
		{
		//if ((phase->ask(pic,q) == false) || (q_abbrechen)) {
			frage->label("Keine weiteren Fragen zu beantworten. Auch gut! :)");
			frage->redraw();
			fortschritt->value(100);
			q_eingabe=true;
			break;
		}
		else
		{
			h2 = "Frage: ";
			h2 += q;
			frage->label(h2.c_str());
			frage->redraw();
			
			//Fl_Shared_Image* img;
			//img = (Fl_Shared_Image::get(pic));
			//Fl_Image *temp;
			//temp = img->copy(400, 400 * img->h() / img->w()); 
			//img->release();
			//img = (Fl_Shared_Image *)temp;
			
			//Fl_Window* w = new Fl_Window(400,400 * img->h() / img->w(),"Wähle die obere linke und untere rechte Ecke");
			//w->begin();
				//Fl_Button* b = new Fl_Button(0,0,400,400 * img->h() / img->w());
				//b->image(img);
				//b->callback((Fl_Callback*)cb_pic_rect);
				//b->redraw();
			//w->end();
			//w->show();
			
			//while (coords[0]==0) {
				//Fl::wait();
			//}
			//w->hide();
			
			while (q_eingabe==false) Fl::wait();
			q_eingabe = false;
			if ((phase->answer((char*)antwort->value())) == false)
			{
				h = "Leider falsch, richtig wäre gewesen: ";
				phase->io() == 1 ?
					h += phase->db_question(phase->current()) :
					h += phase->db_answer(phase->current());
				meldungen->label(h.c_str());
				falschw++;
			}
			
			//x = coords[0]; y = coords[2];
			//for (int j = 0; j < 4; j++)
			//{
				//coords[j]=0;
			//}
			//if (phase->answer(x,y) == false)
			//{
				//h = "Leider falsch, richtig wäre gewesen: X=";
				//char erneuth[4];
				//sprintf(erneuth, "%d", x);
				//h += erneuth;
				//cout << "X: " << x << " Y: " << y << endl;
				//h += ", Y=";
				//sprintf(erneuth, "%d", y);
				//h += erneuth;
				//meldungen->label(h.c_str());
				//falschw++;
			//}
			
			else
			{
				meldungen->label("Richtig!!");
				richtigw++;
			}
			fehler->value(falschw);
			richtig->value(richtigw);
			meldungen->redraw();
			fortschritt->value(i * 100 / wdh);
		}
		if (i > 1)
		{
		changed = true;
		}
	}	
	
	meldungen->label("Training beendet");
	meldungen->redraw();
}

void cb_suche(Fl_Menu_Item* o, void*) {
Fl_Window* w;
	{Fl_Window* o = new Fl_Window(175, 45);
		w = o;
		{ Fl_Input* o = new Fl_Input(0, 20, 175, 25, "Search for: ");
			o->align(FL_ALIGN_TOP);
			o->callback((Fl_Callback*)cb_suche_now, w);
			o->when(FL_WHEN_ENTER_KEY);
		}
		o->end();
	}
	w->show();
}

void cb_suche_now(Fl_Input* o, void* win) {
	Fl_Window* w = (Fl_Window*)win;
	w->hide();
	 {win_liste = new Fl_Window(50, 50, 650, 370, "Liste bearbeiten...");
		win_liste->begin();
		fragebr = new Fl_Select_Browser(5, 5, 315, 330);
		 fragebr->begin();
		 fragebr->callback((Fl_Callback*)cb_browser);
	 fragebr->end();
	
		antwortbr = new Fl_Select_Browser(330, 5, 315, 330);
		 antwortbr->begin();
		 antwortbr->callback((Fl_Callback*)cb_browser);
		 antwortbr->end();
		 
		int pos = 1;
	while ((pos != 0) && (pos <= phase->count()))
		{
			pos = phase->find((char*)o->value(), pos);
			cout << pos << endl;
			if (pos != -1)
			{
				fragebr->add((char*)phase->db_question(pos));
				antwortbr->add((char*)phase->db_answer(pos));
			}
			pos++;
		}
		
		 {Fl_Button* o = new Fl_Button(15, 344, 80, 24, "Hinzuf\303\274gen");
			o->callback((Fl_Callback*)cb_add);
		}
		 {Fl_Button* o = new Fl_Button(195, 344, 80, 24, "Entfernen");
			o->callback((Fl_Callback*)cb_remove);
		}
		 {Fl_Button* o = new Fl_Button(375, 344, 80, 24, "Bearbeiten");
			o->callback((Fl_Callback*)cb_searchedit);
		}
		 {Fl_Button* o = new Fl_Button(555, 344, 80, 24, "Abbrechen");
			o->shortcut(0xff1b);
			o->callback((Fl_Callback*)cb_abbrechen, (void*)(win_liste));
		}
		win_liste->end();
	}
	win_liste->show();
}

void cb_test_abbrechen(Fl_Button*, void*) {
	q_abbrechen = true;
	q_eingabe = true;
}
