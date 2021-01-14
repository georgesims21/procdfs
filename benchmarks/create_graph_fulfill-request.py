import matplotlib.pyplot as plt
import numpy as np
import math


def main():
    # names I wish to use in the output with their respective mount points
    fs = "procdfs"
    mount = "/home/george/github/mountdir/net/"

    files = [
       "rt6_stats",
        "raw6",
        "igmp",
        "dev",
        "netstat",
        "ipv6_route"
    ]
    sizes = [
        35,
        163,
        365,
        1131,
        2775,
        3900
    ]
    two_machines_request = [
        0.000029,
        0.000051,
        0.000053,
        0.000192,
        0.000493,
        0.000641
    ]
    five_machines_request = [
        0.000105,
        0.000123,
        0.000175,
        0.000473,
        0.001853,
        0.002297
    ]


    # make to (ms) instead of secs
    for i in range(6):
        two_machines_request[i] = two_machines_request[i] * 1000
        five_machines_request[i] = five_machines_request[i] * 1000


    font_size = 18
    tick_size = 10
    line_width = 1.2

    # line
    xpoints = np.array(sizes)
    two_machines_req_y = np.array(two_machines_request)
    five_machines_req_y = np.array(five_machines_request)

    plt.figure(figsize=(5,10))
    plt.tick_params(axis='y', which='major', labelsize=3)
    plt.xticks(fontsize=tick_size)
    plt.yticks(fontsize=tick_size)
    plt.rcParams.update({'font.size': font_size})
    plt.subplot(2,1,1)
    plt.title("procdfs: Wall Clock Time For Single Request", fontsize=font_size)
    plt.plot(xpoints, two_machines_req_y, marker='o', color='b', label="2 machines", linewidth=line_width)
    plt.plot(xpoints, five_machines_req_y, marker='x', color='r', label="5 machines", linewidth=line_width)
    plt.xlabel("File size (bytes)", fontsize=font_size)
    plt.ylabel("Latency (ms)", fontsize=font_size)
    plt.legend()
    plt.ylim(ymin=0)
    plt.xlim(xmin=0)
    plt.grid(True)
    plt.subplot(2,1,2)
    plt.title("procdfs: read() - Kernel + User Space Time", fontsize=font_size)
    plt.plot(xpoints, two_machines_req_y, marker='o', color='b', label="2 machines", linewidth=line_width)
    plt.plot(xpoints, five_machines_req_y, marker='x', color='r', label="5 machines", linewidth=line_width)
    plt.xlabel("File size (bytes)", fontsize=font_size)
    plt.ylabel("Latency (ms)", fontsize=font_size)
    plt.tight_layout(h_pad=1)
    plt.ylim(ymin=0)
    plt.xlim(xmin=0)
    plt.grid(True)
    plt.legend()
    # plt.savefig("output.pdf", bbox_inches='tight')
    plt.show()

    # bar
    # proc = np.array(proc_wc_time)
    # prom = np.array(prom_wc_time)
    # both = []
    # both.append(proc)
    # both.append(prom)
    # both = np.array(both)
    # xpoints = np.array(Xaxis_nrmachines)
    # proc = np.array(proc_sys_time)
    # prom = np.array(prom_sys_time)
    # plt.title("procdfs vs Prometheus: Kernel + User Space Time")
    # plt.bar(["procdfs", "Prometheus"], [proc, prom], width=0.2)
    # plt.show()

if __name__=="__main__":
    main()