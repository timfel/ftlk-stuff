from fltk import *
import sys
from sshfs import start_sshfs
from ftpfs import start_ftpfs

# global object names

class make_window():
    def __init__(self):
        self.win = Fl_Double_Window(417, 166, 405, 130)
        self.win.set_modal()
        self.win.callback(self.cancel_cb)

        self.return_btn = Fl_Return_Button(325, 100, 72, 20, """Ok""")
        self.return_btn.label('''Ok''')
        self.return_btn.callback(self.return_cb)

        self.cancel_btn = Fl_Button(255, 100, 70, 20, """Cancel""")
        self.cancel_btn.label('''Cancel''')
        self.cancel_btn.callback(self.cancel_cb)

        self.server = Fl_Input(100, 30, 275, 25, """User@Server""")
        self.server.label('''Server''')
        self.server.take_focus()

        self.passwd = Fl_Secret_Input(100, 55, 275, 25, """Password""")
        self.passwd.label('''Password''')
        self.win.end()
        self.win.show()
    
    def cancel_cb(self, ptr):
        sys.exit(0)

    def return_cb(self, ptr):
        self.win.hide()

def main(dir):
    app = make_window()
    Fl.run()
    while app.win.shown():
        Fl_wait()
    serv = app.server.value()
    if serv.startswith("ssh"):
        serv = serv.rpartition("/")[2]
        start_sshfs("sshfs -o allow_other "+serv+": "+dir, app.passwd.value())
    elif serv.startswith("ftp"):
        serv = serv.rpartition("/")[2]
        user = ""
        if "@" in serv:
            user = serv.partition("@")[0]
            serv = serv.partition("@")[2]
        start_ftpfs('curlftpfs -o allow_other,user="'+user+':'+app.passwd.value()+'" '+serv+" "+dir)
    sys.exit(0)
    
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Usage: pio.py [Mount-Directory]"
    else:
        main(sys.argv[1])
