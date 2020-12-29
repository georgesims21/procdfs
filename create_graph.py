import subprocess, resource

# Run a certain command N times and return the average CPU time used
def loop(command, file, nriterations, fs):
    usage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
    for i in range(nriterations):
        subprocess.call([command, f"{fs}{file}"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    usage_end = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_time = (usage_end.ru_utime - usage_start.ru_utime) / nriterations
    kernel_time = (usage_end.ru_stime - usage_start.ru_stime) / nriterations
    return user_time + kernel_time

def main():
    N = 15000 # number of iterations I want to use

    # names I wish to use in the output with their respective mount points
    filesystems = {
        "procdfs":"/home/george/github/mountdir/net/",
        "procfs":"/proc/net/"
    }
    # single loop to look after all tests and output results to file, so:
    # 1st `$stat <procdfs mountpoint>/rt6_stats`
    # 2nd `$stat <procfs mountpoint>/rt6_stats`
    # ...
    with open("/home/george/github/procdfs/benchmarks/procdfs-vs-procfs-statcat.txt", 'a') as f:
        for cmd in commands:
            for file in files:
                for fs, mtpt in filesystems.items():
                    f.write(f"${cmd} {file} on {fs} took {loop(cmd, file, N, mtpt)} seconds")
                    # print(f"${cmd} {file} on {fs} took {loop(cmd, file, N, mtpt)} seconds")

if __name__=="__main__":
    main()