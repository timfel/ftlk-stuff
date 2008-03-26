#!/usr/bin/python 
import pexpect 
import time
from sys import argv

def usage():
    return """Usage: 
	For mounting:
		ftpfs.py user@hostname mount-dir password
	For unmounting:
		ftpfs.py mount-dir"""

def start_ftpfs(cmd):
    print cmd
    try: 
	pexpect.run(cmd)
    except Exception, e: 
        print str(e) 

def stop_ftpfs(cmd):
    try:
        pexpect.run(cmd) 
    except Exception, e: 
        print str(e)

def main(): 
    if len(argv) == 4:
        if "@" in argv[1]:
	    user = argv[1].partition("@")[0]
            server = argv[1].partition("@")[2]
            start_ftpfs('curlftpfs -o allow_other,user="'+user+':'+argv[3]+'" '+server+" "+argv[2])
        else: 
            print usage()
    elif len(argv) == 2:
        stop_ftpfs("fusermount -u " + argv[1])
    else:
	print usage()

if __name__ == '__main__': 
    main ()  

