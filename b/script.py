import subprocess, sys, os
import time


def path():
    name = "WebCrawler"
    for user, dir, files in os.walk('.'):
        if name in files:
            name = user + '/' + name
            break
    return name


def run_prog(num, conf="conf.txt"):
    name = path()
    all_time = []
    for i in range(num):
        proc = subprocess.Popen([name, conf],shell=True, stdin = subprocess.PIPE,stdout=subprocess.PIPE)
        for line in proc.stdout:
        	res = int(line)
        	all_time.append(res)
    return (all_time)




def main(arr):
	if len(arr)==1:
		print("Please enter num of times")
	elif len(arr)==2:
		time = run_prog(int(arr[1]))
	else:
		time = run_prog(int(arr[1]), arr[2])
	for i in range(len(time)):
		print(i+1, 'time process: ', time[i])
	print("MIN TIME: ", min(time))		


if __name__ == '__main__':
    main(sys.argv)
