import os
from xml.marshal import generic

class Config():
    alben = []
    configuration = {}
    configdatei = file
    
    def load_config(self, win):
        self.configdatei = file(os.environ.get("HOME")+"/.pyflphoto", 'a+')
        try:
            self.configuration = generic.load(self.configdatei)
            #for i in self.configuration["alben"]:
            #    if self.configuration["alben"].count(i) > 1:
            #        self.configuration["alben"].remove(i)
        except:
            self.configuration["alben"] = []
            self.configuration["options"] = {}
            generic.dump(self.configuration, self.configdatei)
        self.configdatei.flush()
        self.configdatei.close()
        self.alben = self.configuration["alben"]
    
    def add_to_config(self, dirname):
        b = False
        self.configdatei = file(os.environ.get("HOME")+"/.pyflphoto", 'w')
        if self.configuration["alben"].count(dirname) == 0:
            self.configuration["alben"].append(dirname)
            b = True
        self.alben = self.configuration["alben"]
        generic.dump(self.configuration, self.configdatei)
        self.configdatei.flush()
        self.configdatei.close()
        return b
        
    def del_from_config(self, dirnum):
        self.configdatei = file(os.environ.get("HOME")+"/.pyflphoto", 'w')
        self.configuration["alben"].pop(dirnum-1)
        self.alben = self.configuration["alben"]
        generic.dump(self.configuration, self.configdatei)
        self.configdatei.flush()
        self.configdatei.close()
    
    def set_option(self, option, value):
        self.configdatei = file(os.environ.get("HOME")+"/.pyflphoto", 'w')
        self.configuration["options"][option] = value
        generic.dump(self.configuration, self.configdatei)
        self.configdatei.flush()
        self.configdatei.close()
