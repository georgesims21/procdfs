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
    two_machines_getattrb = [
        0.000085,
        0.000224,
        0.000406,
        0.001174,
        0.002919,
        0.003673
    ]
    five_machines_getattrb = [
        0.000157,
        0.000339,
        0.000533,
        0.001507,
        0.004226,
        0.005059
    ]
    ten_machines_getattrb = [
        0.000198,
        0.000287,
        0.000383,
        0.001911,
        0.005684,
        0.006323
    ]
    two_machines_read = [
        0.000085,
        0.000147,
        0.000415,
        0.001459,
        0.002971,
        0.003640
    ]
    five_machines_read = [
        0.000103,
        0.000151,
        0.000446,
        0.001780,
        0.004226,
        0.005019
    ]
    ten_machines_read = [
        0.000181,
        0.000253,
        0.000429,
        0.002193,
        0.006000,
        0.007543
    ]


    # make to (ms) instead of secs
    for i in range(6):
        two_machines_getattrb[i] = two_machines_getattrb[i] * 1000
        five_machines_getattrb[i] = five_machines_getattrb[i] * 1000
        ten_machines_getattrb[i] = ten_machines_getattrb[i] * 1000
        two_machines_read[i] = two_machines_read[i] * 1000
        five_machines_read[i] = five_machines_read[i] * 1000
        ten_machines_read[i] = ten_machines_read[i] * 1000

    font_size = 18
    tick_size = 10
    line_width = 1.2

    # line
    xpoints = np.array(sizes)
    twoline_getattrb = np.array(two_machines_getattrb)
    fiveline_getattrb = np.array(five_machines_getattrb)
    tenline_getattrb = np.array(ten_machines_getattrb)
    twoline_read = np.array(two_machines_read)
    fiveline_read = np.array(five_machines_read)
    tenline_read = np.array(ten_machines_read)

    plt.figure(figsize=(5,10))
    plt.tick_params(axis='y', which='major', labelsize=3)
    plt.xticks(fontsize=tick_size)
    plt.yticks(fontsize=tick_size)
    plt.rcParams.update({'font.size': font_size})
    plt.subplot(2,1,1)
    plt.title("procdfs: getattrb() - Kernel + User Space Time", fontsize=font_size)
    plt.plot(xpoints, twoline_getattrb, marker='o', color='b', label="2 machines", linewidth=line_width)
    plt.plot(xpoints, fiveline_getattrb, marker='x', color='r', label="5 machines", linewidth=line_width)
    plt.plot(xpoints, tenline_getattrb, marker='*', color='g', label="10 machines", linewidth=line_width)
    plt.xlabel("File size (bytes)", fontsize=font_size)
    plt.ylabel("Latency (ms)", fontsize=font_size)
    plt.legend()
    plt.ylim(ymin=0)
    plt.xlim(xmin=0)
    plt.grid(True)
    plt.subplot(2,1,2)
    plt.title("procdfs: read() - Kernel + User Space Time", fontsize=font_size)
    plt.plot(xpoints, twoline_read, marker='o', color='b', label="2 machines")
    plt.plot(xpoints, fiveline_read, marker='x', color='r', label="5 machines")
    plt.plot(xpoints, tenline_read, marker='*', color='g', label="10 machines")
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