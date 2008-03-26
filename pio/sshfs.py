#!/usr/bin/python 
import pexpect 
import time
from sys import argv

def usage():
    return """Usage: 
	For mounting:
		sshfs.py user@hostname mount-dir password
	For unmounting:
		sshfs.py mount-dir"""

def start_sshfs(cmd, pw):
    try: 
        sshfs = pexpect.spawn(cmd) 
        sshfs.expect("password: ") 
        time.sleep (0.5) 
        sshfs.sendline (pw) 
        time.sleep (5)
        sshfs.expect (pexpect.EOF) 
    except Exception, e: 
        print str(e) 

def stop_sshfs(cmd):
    try:
        pexpect.run(cmd) 
    except Exception, e: 
        print str(e)

def main(): 
    if len(argv) == 4:
        if "@" in argv[1]:
            start_sshfs("sshfs -o allow_other " + argv[1] + ": " + argv[2], argv[3])
        else: 
            print usage()
    elif len(argv) == 2:
        stop_sshfs("fusermount -u " + argv[1])
    else:
	print usage()

if __name__ == '__main__': 
    main ()  

