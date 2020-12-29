import subprocess, resource, timeit, time, threading

prom_sys_time = 0
prom_wc_time = 0
proc_sys_time = 0
proc_wc_time = 0

class myThread (threading.Thread):
    def __init__(self, threadID, N):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.N = N
    def run(self):
        decide(self.threadID, self.N)

def sys_loop(command, nriterations):
    usage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    usage_end = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_time = (usage_end.ru_utime - usage_start.ru_utime) / nriterations
    kernel_time = (usage_end.ru_stime - usage_start.ru_stime) / nriterations
    return user_time + kernel_time

def wc_loop(command, nriterations):
    wc_start = timeit.default_timer()
    for i in range(nriterations):
        subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
    wc_end = timeit.default_timer()
    wc_time = (wc_end - wc_start) / nriterations
    return wc_time

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
        proc_sys_time = sys_loop(cmds.get("proc"), nriterations)
        print(f"[{id}] completed, exiting..")
    elif(id == 3):
        print(f"[{id}] starting proc wc loop...")
        proc_wc_time = wc_loop(cmds.get("proc"), nriterations)
        print(f"[{id}] completed, exiting..")
    else:
        print("Something went wrong, exiting..")
        exit(1)

def main():
    nrthreads = 4
    nriterations = 1000
    nrmachines = 1

    # Create new threads
    threads = []
    for i in range(nrthreads):
        threads.append(myThread(i, nriterations))

    # Start new Threads
    for thread in threads:
        thread.start()

    # Join them
    for thread in threads:
        thread.join()

    with open("/home/george/github/procdfs/benchmarks/test.txt", "a") as fp:
        fp.write(f"[{nrmachines}]\nprom | sys-time: {prom_sys_time}\twall-time: {prom_wc_time}\n"
                 f"proc | sys-time: {proc_sys_time}\twall-time: {proc_wc_time}\n")

if __name__=="__main__":
    main()