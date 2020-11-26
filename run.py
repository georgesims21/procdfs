import subprocess

ip = subprocess.run('ip -o -4 addr list ib0 | awk \'{print $4}\' | cut -d/ -f1', shell=True, stdout=subprocess.PIPE).stdout.decode('utf-8')
run = "./procdfs -d -f -s mountdir{0} 2 1234 ib0 iplist.txt {1}".format(ip, ip)

with open('output.txt', 'a') as f:
    p1 = subprocess.run(run, shell=True)
