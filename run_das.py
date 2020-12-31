import subprocess, resource, timeit, time, threading
import socket
import fcntl
import struct


# prom_sys_time = 0
# prom_wc_time = 0
# proc_sys_time = 0
# proc_wc_time = 0

class myThread (threading.Thread):
    def __init__(self, threadID, N):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.N = N
    def run(self):
        decide(self.threadID, self.N)

def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15].encode('utf-8'))
    )[20:24])

def sys_loop(command, nriterations):
    usage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    usage_end = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_time = usage_end.ru_utime - usage_start.ru_utime
    kernel_time = usage_end.ru_stime - usage_start.ru_stime
    return (user_time + kernel_time) / nriterations

def sys_loop2(command, nriterations):
    usage_start2 = resource.getrusage(resource.RUSAGE_CHILDREN)
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    usage_end2 = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_time2 = usage_end2.ru_utime - usage_start2.ru_utime
    kernel_time2 = usage_end2.ru_stime - usage_start2.ru_stime
    return (user_time2 + kernel_time2) / nriterations

def wc_loop(command, nriterations):
    wc_start3 = timeit.default_timer()
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    wc_end3 = timeit.default_timer()
    wc_time3 = (wc_end3 - wc_start3) / nriterations
    return wc_time3

def wc_loop2(command, nriterations):
    wc_start4 = timeit.default_timer()
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    wc_end4 = timeit.default_timer()
    wc_time4 = (wc_end4 - wc_start4) / nriterations
    return wc_time4

def decide(id, nriterations):
    global prom_sys_time
    global prom_wc_time
    global proc_sys_time
    global proc_wc_time
    cmds = {
        "prom" : "curl -fs --data-urlencode \'query=sum(node_netstat_Icmp6_OutMsgs"
                 "{job=\"node_exp\"})\' localhost:9090/api/v1/query | jq -r \'.data."
                 "result[].value[1]\'",
        "proc" : "cat /home/george/vagrant/shared/procdfs/box/mount/net/netstat | awk \'NR==2{print ($98)}\'"
    }
    if(id == 0):
        print(f"[{id}] starting prom sys loop...")
        prom_sys_time = sys_loop(cmds.get("prom"), nriterations)
        print(f"[{id}] completed, exiting..")
    elif(id == 1):
        print(f"[{id}] starting prom wc loop...")
        prom_wc_time = wc_loop(cmds.get("prom"), nriterations)
        print(f"[{id}] completed, exiting..")
    elif(id == 2):
        print(f"[{id}] starting proc sys loop...")
        proc_sys_time = sys_loop2(cmds.get("proc"), nriterations)
        print(f"[{id}] completed, exiting..")
    elif(id == 3):
        print(f"[{id}] starting proc wc loop...")
        proc_wc_time = wc_loop2(cmds.get("proc"), nriterations)
        print(f"[{id}] completed, exiting..")
    else:
        print("Something went wrong, exiting..")
        exit(1)

def main():
    ip = get_ip_address("ib0")
    cmds = {
        "prom" : "curl -fs --data-urlencode \'query=sum(node_netstat_Icmp6_OutMsgs"
                 "{job=\"node_exp\"})\' localhost:9090/api/v1/query | jq -r \'.data."
                 "result[].value[1]\'",
        "proc" : f"cat /home/gss680/{ip} | " + "awk \'NR==2{print ($98)}\'"
    }
    nrthreads = 4
    nriterations = 15000
    nrmachines = 2

    # # Create new threads
    # threads = []
    # for i in range(nrthreads):
    #     threads.append(myThread(i, nriterations))
    #
    # # Start new Threads
    # for thread in threads:
    #     thread.start()
    #
    # # Join them
    # for thread in threads:
    #     thread.join()

    # print("starting..")
    # prom_sys_time = sys_loop(cmds.get("prom"), nriterations)
    # print("25%..")
    # prom_wc_time = wc_loop(cmds.get("prom"), nriterations)
    # print("50%..")
    #prom_sys_time = 0
    #prom_wc_time = 0
    #proc_sys_time = sys_loop(cmds.get("proc"), nriterations)
    #print("75%..")
    #proc_wc_time = wc_loop(cmds.get("proc"), nriterations)
    #print("100%.. writing to file")

    with open(f"/home/gss680/{ip}OUTPUT.txt", "a") as fp:
        fp.write(f"[{nrmachines}]\nprom | sys-time: {prom_sys_time}\twall-time: {prom_wc_time}\n"
                     f"proc | sys-time: {proc_sys_time}\twall-time: {proc_wc_time}\n")

if __name__=="__main__":
    main()
