from fltk import *
import sys
from config import Config
from image_browser import Fl_Tiled_Images
from file_op import *
from image_functions import *

class MakeWindow():
    
    def cancel_cb(self, ptr):
        sys.exit(0)
        
    def alben_cb(self, ptr):
        self.show_album(self.alben.text(self.alben.value()))
        
    def bilder_cb(self, ptr):
        o = ptr.image()
        self.tabs.value(self.editgroup)
        neww, newh = img_scale(o, self.editbtn.w())
        self.editbtn.image(o.copy(neww, newh))
        self.editgroup.redraw()
    
    def import_cb(self):
        o = Fl_File_Chooser(".", "*", 4, "Choose your dir...")
        o.show()
        while o.visible():
            Fl_wait()
        directory = o.value()
        if self.cfg.add_to_config(directory):
            self.alben.add(directory)
        self.show_album(directory)
        
    def show_album(self, dir):
        for i in recursive_add(dir):
            o = self.tiles.image_append(i, 3)
            o.callback(self.bilder_cb)
        
    
    def delete_cb(self):
        self.cfg.del_from_config(self.alben.value())
        self.alben.clear()
        for i in self.cfg.alben:
            self.alben.add(i)
    
    def slide_cb(self):
        pass
    
    def menu_cb(self, ptr, args):
        if args == "exit":
            sys.exit(0)
        elif args == "import":
            self.import_cb()
        elif args == "delete":
            self.delete_cb()
        elif args == "slide":
            self.slide_cb()
    
    def __init__(self, configobject):
        self.cfg = configobject
        self.cfg.load_config(self)
        
        self.window = Fl_Double_Window(1, 16, 890, 780)
        self.window.callback(self.cancel_cb)
        self.alben = Fl_Select_Browser(0, 25, 195, 755)
        for i in self.cfg.alben:
            self.alben.add(i)
        self.alben.callback(self.alben_cb)
        self.alben.end()
        self.tabs = Fl_Tabs(195, 25, 695, 755, """View""")
        self.tiles = Fl_Tiled_Images(195, 45, 695, 735, """Uebersicht""")
        self.tiles.box(FL_EMBOSSED_FRAME)
        self.tiles.label('''Uebersicht''')
        self.tiles.end()
        self.editgroup = Fl_Group(195, 45, 695, 735, """Bearbeiten""")
        self.editgroup.label('''Bearbeiten''')
        self.editbtn = Fl_Button(200, 50, 685, 725)
        self.editbtn.box(FL_NO_BOX)
        self.editgroup.end()
        self.tabs.label('''View''')
        self.tabs.end()
        self.menubar = Fl_Menu_Bar(0, 0, 890, 24)
        self.menubar.add("Importieren", 0x40061, self.menu_cb, "import", 0)
        self.menubar.add("Entfernen", 0x40064, self.menu_cb, "delete", 0)
        self.menubar.add("Slideshow", 0x40073, self.menu_cb, "slide", 128)
        self.menubar.add("Programm Verlassen", 0x40071, self.menu_cb, "exit", 0)
        self.window.end()
        self.window.show()

fl_register_images()
cfg = Config()
app = MakeWindow(cfg)
Fl.run()
