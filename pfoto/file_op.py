import os

def recursive_add(dir):
    pics = []
    for picture in os.listdir(dir):
        if os.path.isdir(dir+"/"+picture):
            if not picture.startswith('.'):
                pics += recursive_add(dir+"/"+picture+"/")
        elif picture.endswith(".jpg"):
            pics.append(dir+"/"+picture)
        elif picture.endswith(".JPG"):
            pics.append(dir+"/"+picture)
        elif picture.endswith(".jpeg"):
            pics.append(dir+"/"+picture)
        elif picture.endswith(".JPEG"):
            pics.append(dir+"/"+picture)
    return pics
