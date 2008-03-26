
def img_scale(img, num):
        if img.h() > img.w():
            fact = img.h() / num
        else:
            fact = img.w() / num
        if fact > 1:
            return img.w()//fact, img.h()//fact
        else:
            return img.w(), img.h()
