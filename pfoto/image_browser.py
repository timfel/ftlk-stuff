from fltk import *
from image_functions import *

class Fl_Tiled_Images(Fl_Group):
    currentx = 0
    currenty = 0
    def image_append(self, bild, scale):
        scale += 1
        if self.currentx == 0 or self.currenty == 0:
            self.currentx = self.x()
            self.currenty = self.y()
        self.begin()
        box = Fl_Button(self.currentx, self.currenty, self.h()//scale, self.w()//scale)
        box.box(FL_NO_BOX)
        o = Fl_JPEG_Image(bild)
        neww, newh = img_scale(o, self.w()//scale)
        o = o.copy(neww, newh)
        box.image(o)
        box.show()
        self.end()
        self.redraw()
        if self.currentx < (self.x()+self.w()-self.w()//scale-5):
            self.currentx += self.w()//scale
        else:
            self.currentx = self.x()
            self.currenty = self.h()//scale
        return box
        
    def resize(self, w, h):
        pass


