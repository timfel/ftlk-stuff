// generated by Fast Light User Interface Designer (fluid) version 2.1000

#include "flPear.h"
#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#include <errno.h>

Fl_Input *hddimg=(Fl_Input *)0;
Fl_Input* create_hddimg=(Fl_Input *)0;
Fl_Input *cdimg=(Fl_Input *)0;
Fl_Check_Button *fscreen=(Fl_Check_Button *)0;
Fl_Check_Button *g4use=(Fl_Check_Button *)0;
Fl_Check_Button *ramuse=(Fl_Check_Button *)0;
Fl_Check_Button *netuse1=(Fl_Check_Button *)0;
Fl_Check_Button *netuse2=(Fl_Check_Button *)0;
Fl_Input *boot_args=(Fl_Input *)0;
Fl_Input_Choice *res=(Fl_Input_Choice *)0;
Fl_Check_Button *cd_dev_check=(Fl_Check_Button *)0;
Fl_Value_Output *valuemoutput=(Fl_Value_Output *)0;
Fl_Input_Choice *valueout=(Fl_Input_Choice *)0;

#include <FL/Fl.H>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
//#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Display.H>

using namespace std;

int parse_config_file(string);
string binary;
string systemcall;
string location;
#ifndef WIN32
	fstream conffile;
#endif
Fl_Text_Display *G_disp;     
Fl_Text_Buffer *G_buff;      
int G_outfd;
pid_t G_pids;

#ifndef WIN32
void start_child(int t) {
    int out[2]; pipe(out);
    switch ( ( G_pids = fork() ) ) {
        case -1: // Error
            close(out[0]); close(out[1]);
            perror("fork()");
            exit(1);
        case 0: // Child
            close(out[0]); dup2(out[1], 2); close(out[1]);
            	execlp("/bin/sh", "sh", "-c", systemcall.c_str(),0);
            	perror("execlp(ps)");
            	exit(1);        
        default: // Parent
            G_outfd = out[0]; close(out[1]);
            return;
    }
}

void data_ready(int fd, void *data) {
    //int t = (int)data;
    int t = 0;
    char s[4096];
    int bytes = read(fd, s, 4096-1);
    // fprintf(stderr, "Data ready for %d) pid=%ld fd=%d bytes=%d\n", t, (long)G_pids, fd, bytes);  
    if ( bytes == -1 ) {                // ERROR
        perror("read()");
    } else if ( bytes == 0 ) {          // EOF
        G_buff->append("\n\n*** EOF ***\n");
        int status;
        if ( waitpid(G_pids, &status, WNOHANG) < 0 ) {
            sprintf(s, "waitpid(): %s\n", strerror(errno));
        } else {
            if ( WIFEXITED(status) ) {
                sprintf(s, "Exit=%d\n", WEXITSTATUS(status));
                close(fd); Fl::remove_fd(fd); G_pids = -1;
            } else if ( WIFSIGNALED(status) ) {
                sprintf(s, "Killed with %d\n", WTERMSIG(status));
                close(fd); Fl::remove_fd(fd); G_pids = -1;
            } else if ( WIFSTOPPED(status) ) {
                sprintf(s, "Stopped with %d\n", WSTOPSIG(status));
            }
        }
        G_buff->append(s);
    } else {                            // DATA
        s[bytes] = 0;
        G_buff->append(s);
    }
}

void close_cb(Fl_Widget*, void*) {
    printf("Killing child processes..\n");
    (G_pids != -1) ? kill(G_pids, 9) : printf("Nothing to kill.\n");
    printf("Done.\n");
    exit(0);
}
#endif

void cancel_callback(Fl_Button*, void* wp) {
	Fl_Window* w = (Fl_Window*)wp;
	w->hide();
}

void create_callback(Fl_Return_Button*, void* wp) {
	Fl_Window* w = (Fl_Window*)wp;
	w->hide();
#ifndef WIN32
	string creatorcall = "wine /usr/share/pearpc/buildhdd.exe";
#endif
#ifdef WIN32
	string creatorcall = location;
	creatorcall += "\\buildhdd.exe";
#endif
	creatorcall += " ";
#ifndef WIN32
	int pos;
	creatorcall += "Z:\\";
#endif
	creatorcall += create_hddimg->value();
#ifndef WIN32
	for (int i = 35; i < creatorcall.length(); i++)
	{
		if ((pos = creatorcall.find("/", i)) != string::npos)
		{
			creatorcall.replace(pos, 1, "\\\\");
		}
	}
#endif
	creatorcall += " ";
	creatorcall += valueout->value();
	cout << creatorcall << endl;
	system(creatorcall.c_str());
}

void hddmake_callback(Fl_Button*, void*) {
	Fl_Window* w;
   {Fl_Window* o = new Fl_Window(280, 180);
    w = o;
    o->begin();
     {Fl_Input* o = create_hddimg = new Fl_Input(25, 25, 175, 25, "HDD-Image");
      o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
    }
     {Fl_Button* o = new Fl_Button(205, 25, 50, 25, "Find...");
      o->callback((Fl_Callback*)hdd_callback, (void*)(create_hddimg));
    }
      valueout = new Fl_Input_Choice(160, 75, 50, 25);
      valueout->add("1");
      valueout->add("3");
      valueout->add("6");
      valueout->value("3");
     {Fl_Box* o = new Fl_Box(25, 75, 110, 25, "Size in GB");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
    }
     {Fl_Return_Button* o = new Fl_Return_Button(35, 125, 100, 25, "Create it!");
      o->shortcut(0xff0d);
      o->callback((Fl_Callback*)create_callback, (void*)(w));
    }
     {Fl_Button* o = new Fl_Button(145, 125, 100, 25, "Cancel");
      o->callback((Fl_Callback*)cancel_callback, (void*)(w));
    }
    o->end();
   }
   w->show();
}

int main (int argc, char **argv) {
#ifdef WIN32
  location = argv[0];
  //location.erase((location.length()-11), 11);
  cout << "flPear - the PearPC Frontend. Built for Windows" << endl;
#endif
#ifndef WIN32
  cout << "flPear - the PearPC Frontend. Built for UNIX" << endl;
  Fl::scheme("gtk+");
#endif
	
  Fl_Window* w;
   {Fl_Window* o = new Fl_Window(425, 355);
    w = o;
    o->begin();
     {Fl_Input* o = hddimg = new Fl_Input(25, 25, 255, 25, "HDD-Image");
      o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
    }
     {Fl_Input* o = cdimg = new Fl_Input(25, 70, 255, 25, "CD-Image");
      o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
    }
     {Fl_Check_Button* o = fscreen = new Fl_Check_Button(25, 160, 90, 25, "Fullscreen");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      o->labelsize(11);
    }
     {Fl_Check_Button* o = g4use = new Fl_Check_Button(25, 185, 215, 25, "Use experimental G4 processor");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      o->labelsize(11);
    }
     {Fl_Check_Button* o = ramuse = new Fl_Check_Button(25, 210, 245, 25, "Use 256 MB RAM instead of 128 MB");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
      o->labelsize(11);
    }
     {Fl_Check_Button* o = netuse1 = new Fl_Check_Button(25, 235, 190, 25, "Use 3c90x network device");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      o->labelsize(11);
    }
     {Fl_Check_Button* o = netuse2 = new Fl_Check_Button(25, 260, 195, 25, "Use rtl8139 network device");
      o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      o->labelsize(11);
    }
     {Fl_Input* o = boot_args = new Fl_Input(100, 285, 180, 25, "BootArgs:");
      o->align(FL_ALIGN_LEFT);
      o->labelsize(11);
    }
     {Fl_Button* o = new Fl_Button(290, 25, 50, 25, "Find...");
      o->callback((Fl_Callback*)hdd_callback, (void*)(hddimg));
    }
     {Fl_Button* o = new Fl_Button(350, 25, 50, 25, "Make...");
      o->callback((Fl_Callback*)hddmake_callback, (void*)(hddimg));
    }
     {Fl_Button* o = new Fl_Button(290, 70, 50, 25, "Find...");
      o->callback((Fl_Callback*)cd_callback, (void*)(cdimg));
    }
     {Fl_Input_Choice* o = res = new Fl_Input_Choice(25, 115, 230, 25, "Resolution");
      o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
      res->add("800x600x15");
      res->add("1024x768x15");
      res->add("1280x800x15");
      res->add("1280x960x15");
    }
     {Fl_Check_Button* o = cd_dev_check = new Fl_Check_Button(5, 70, 20, 25);
     	o->callback((Fl_Callback*)cd_dev_callback);
     	o->value(1);
    }
     {Fl_Dial* o = new Fl_Dial(305, 205, 75, 75);
      o->box(FL_OSHADOW_BOX);
      o->minimum(10);
      o->maximum(500);
      o->type(0);
      o->step(5);
      o->value(40);
      o->callback((Fl_Callback*)dial_change);
      
    }
     {Fl_Box* o = new Fl_Box(280, 190, 125, 20, "Refresh Rate");
      o->align(FL_ALIGN_TOP|FL_ALIGN_INSIDE);
    }
     {Fl_Value_Output* o = valuemoutput = new Fl_Value_Output(328, 280, 30, 20);
      o->box(FL_THIN_DOWN_BOX);
      o->minimum(10);
      o->maximum(500);
      o->step(1);
      o->value(40);
    }
     {Fl_Return_Button* o = new Fl_Return_Button(295, 315, 110, 25, "Run PearPC");
      o->shortcut(0xff0d);
      o->callback((Fl_Callback*)run_callback);
    }
     {Fl_Button* o = new Fl_Button(155, 315, 110, 25, "Locate ppc");
      o->callback((Fl_Callback*)locate_callback);
    }
     {Fl_Button* o = new Fl_Button(15, 315, 110, 25, "Save config");
      o->callback((Fl_Callback*)save_callback);
    }
    o->end();
  }
  #ifndef WIN32
  	parse_config_file(getenv("HOME"));
  #endif
  #ifdef WIN32
  	parse_config_file("");
  #endif
  w->show(argc, argv);
  return  Fl::run();
}

void run_callback(Fl_Return_Button*, void*) {
#ifndef WIN32
	ofstream ofile("/tmp/ppc.cfg", ios::trunc);
#endif
#ifdef WIN32
	string temp = location;
	temp += "\\ppc.cfg";
	ofstream ofile(temp.c_str(), ios::trunc);
#endif
	ofile << "ppc_start_resolution = \"" << res->value() << "\"" << endl;
	fscreen->value()==1 ? 
	  ofile << "ppc_start_full_screen = 1" << endl : 
	  ofile << "ppc_start_full_screen = 0" << endl;
#ifndef WIN32
	ofile << "redraw_interval_msec = " << valuemoutput->value() << endl;
#endif
#ifdef WIN32
	ofile << "redraw_interval_msec = 40" << endl;
#endif
	ofile << "key_compose_dialog = \"F11\"" << endl;
	ofile << "key_change_cd_0 = \"none\"" << endl;
	ofile << "key_toggle_mouse_grab = \"F12\"" << endl;
	ofile << "key_toggle_full_screen = \"Alt+Return\"" << endl;
	ofile << "prom_bootmethod = \"select\"" << endl;
	if (boot_args->value() == "") {
	  ofile << "prom_env_machargs = \"-v\"" << endl;
        } else {
	  ofile << "prom_env_machargs = \"" << boot_args->value() << "\"" << endl;
	}
#ifndef WIN32
	ofile << "prom_driver_graphic = \"/usr/share/pearpc/video.x\"" << endl;
#endif
#ifdef WIN32
	ofile << "prom_driver_graphic = \"" << location << "\\video.x\"" << endl;
#endif
	ofile << "#page_table_pa = 104857600" << endl;
	g4use->value()==1 ? 
	  ofile << "cpu_pvr = 0x000c0000" << endl : 
	  ofile << "cpu_pvr = 0x00088302" << endl;
	ramuse->value()==1 ? 
	  ofile << "memory_size=0x10000000" << endl : 
	  ofile << "#memory_size=0x10000000" << endl;
	ofile << "pci_ide0_master_installed = 1" << endl;
	ofile << "pci_ide0_master_image = \"" << hddimg->value() << "\"" << endl;
	ofile << "pci_ide0_master_type = \"hd\"" << endl;
	cd_dev_check->value()==1 ?
	  ofile << "pci_ide0_slave_installed = 1" << endl : 
	  ofile << "pci_ide0_slave_installed = 0" << endl;
	ofile << "pci_ide0_slave_image = \"" << cdimg->value() << "\"" << endl;
	ofile << "pci_ide0_slave_type = \"cdrom\"" << endl;
	netuse1->value()==1 ?
	  ofile << "pci_3c90x_installed = 1" << endl :
	  ofile << "pci_3c90x_installed = 0" << endl;
	ofile << "pci_3c90x_mac = \"de:ad:ca:fe:12:34\"" << endl;
	netuse2->value()==1 ?
	  ofile << "pci_rtl8139_installed = 1" << endl :
	  ofile << "pci_rtl8139_installed = 0" << endl;
	ofile << "pci_rtl8139_mac = \"de:ad:ca:fe:12:35\"" << endl;
	ofile << "pci_usb_installed = 1" << endl;
#ifndef WIN32
		ofile << "pci_serial_installed = 0" << endl;
		ofile << "nvram_file = \"/tmp/nvram\"" << endl;
#endif
#ifdef WIN32
		ofile << "nvram_file = \"" << location << "\\nvram\"" << endl;
#endif
	ofile.close();
#ifndef WIN32
		systemcall = binary;
		systemcall += " /tmp/ppc.cfg 1>&2";
#endif
#ifdef WIN32
		systemcall = binary;
		systemcall += " ";
		systemcall += location;
		systemcall += "\\ppc.cfg";
#endif

#ifndef WIN32	
	Fl_Window* out = new Fl_Window(520, 320, "Output");
	out->begin();
	{
		start_child(0);
		G_buff = new Fl_Text_Buffer();
		G_disp = new Fl_Text_Display(10, 10, 500, 300);
		G_disp->buffer(G_buff);
        	G_disp->textfont(FL_COURIER);
        	G_disp->textsize(12);
        	Fl::add_fd(G_outfd, data_ready, (void*)0);
	}
	out->resizable(out);
	out->end();
	out->show();
#endif
#ifdef WIN32
	system(systemcall.c_str());
#endif
}
void hdd_callback(Fl_Button*, void* ph) {
	Fl_Input* hd = (Fl_Input*)ph;
	char* newfile;
	newfile = fl_file_chooser("Choose a hdd image...", 
							"Image Files (*.img)",
							"",0);
  	if (newfile != NULL) hd->value(newfile);
}
void cd_callback(Fl_Button*, void*) {
	char* newfile;
	newfile = fl_file_chooser("Choose an iso image...", 
							"ISO Files (*.iso)",
							"",0);
  	if (newfile != NULL) cdimg->value(newfile);
}
void dial_change(Fl_Dial* w, void*) {
	valuemoutput->value(w->value());
}
void cd_dev_callback(Fl_Check_Button* chk, void*) {
	chk->value()==0 ? cdimg->deactivate() : cdimg->activate();
}
int parse_config_file(string homedir) {
#ifndef WIN32
	homedir += "/.flPear.cfg";
	conffile.open(homedir.c_str(), ios::in);
#endif
#ifdef WIN32
	homedir = location;
	homedir += "\\flPear.cfg";
	ifstream conffile(homedir.c_str());
#endif
	if (conffile == NULL)
	{
#ifndef WIN32
		cerr << "\nCreating config file";
		conffile.open(homedir.c_str(), ios::out);
			binary = "/usr/share/pearpc/ppc";
#endif
#ifdef WIN32
		cerr << "\nCreating config file";
		ofstream conffile(homedir.c_str());
			binary = location;
			binary += "\\ppc";
#endif
		conffile << binary << "\n\n\n\n\n\n\n\n";
		conffile.close();
	}
	else	
	{
		char c;
		string buf;
		int opt;
		
		getline(conffile, binary);
		cout << "Binary:" << binary << endl;
	
		getline(conffile,buf);
		hddimg->value(buf.c_str());
		cout << "HDD-Image:" << buf << endl;
		
		opt = conffile.get();
		cd_dev_check->value(opt);
		cd_dev_callback(cd_dev_check, 0);
		c = conffile.get();
		
		getline(conffile,buf);
		cdimg->value(buf.c_str());
		cout << "CD-Image:" << buf << endl;
		
		getline(conffile, buf);
		res->value(buf.c_str());
		cout << "Resolution:" << buf << endl;
		
		opt = conffile.get();
		fscreen->value(opt);
		opt = conffile.get();
		g4use->value(opt);
		opt = conffile.get();
		ramuse->value(opt);
		opt = conffile.get();
		netuse1->value(opt);
		opt = conffile.get();
		netuse2->value(opt);
		
		conffile >> opt;
		valuemoutput->value(opt);
		opt = conffile.get();
		getline(conffile,buf);
		boot_args->value(buf.c_str());
		cout << "Boot Arguments:" << buf << endl;
	}
	conffile.close();
}
void save_callback(Fl_Button*, void*) {
	#ifndef WIN32
		string homedir = getenv("HOME");
		homedir += "/.flPear.cfg";
	#endif
	#ifdef WIN32
		string homedir = location;
		homedir += "\\flPear.cfg";
	#endif
	cout << "Saving to " << homedir << endl;
	#ifndef WIN32
		conffile.open(homedir.c_str(), ios::out|ios::trunc);
	#endif
	#ifdef WIN32
		ofstream conffile(homedir.c_str(), ios::trunc);
	#endif
	conffile << binary.c_str() << endl;
	conffile << hddimg->value() << endl;
	conffile.put(cd_dev_check->value());
	conffile.put('\n');
	conffile << cdimg->value() << endl;
	conffile << res->value() << endl;
	conffile.put(fscreen->value());
	conffile.put(g4use->value());
	conffile.put(ramuse->value());
	conffile.put(netuse1->value());
	conffile.put(netuse2->value());
	conffile.put('\n');
	conffile << valuemoutput->value() << endl;
	conffile << boot_args->value() << endl;
	conffile.close();
}
void locate_callback(Fl_Button*, void*) {
	char* newfile;
	newfile = fl_file_chooser("Locate the ppc binary...", 
							"Any File (*)",
							"",0);
	#ifndef WIN32
		string configfile = getenv("HOME");
		configfile += "/.flPear.cfg";
	#endif
	#ifdef WIN32
		string configfile = location;
		configfile += "\\flPear.cfg";
	#endif
  	if (newfile != NULL) binary=newfile;
}
