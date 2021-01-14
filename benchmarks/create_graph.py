import matplotlib.pyplot as plt
import numpy as np
import math


def main():
    # names I wish to use in the output with their respective mount points
    fs = "procdfs"
    mount = "/home/george/github/mountdir/net/"

    Xaxis = [1, 2, 5, 10]

    prom_sys = [
        0.010837498666666666,
        0.0108111256,
        0.0107875048,
        0.0109649436
    ]
    prom_wc = [
        0.015432822493153314,
        0.012903274830399701,
        0.015521603754411141,
        0.015804417727546146
    ]
    proc_sys = [
        0.007287529333333334,
        0.008643407266666667,
        0.008752221666666666,
        0.008879784533333332
    ]
    proc_wc = [
        0.014123483863441895,
        0.06920413403619702,
        0.12010078519905606,
        0.11964131918900336
    ]


# make to (ms) instead of secs
    for i in range(4):
        prom_sys[i] = prom_sys[i] * 1000
        prom_wc[i] = prom_wc[i] * 1000
        proc_sys[i] = proc_sys[i] * 1000
        proc_wc[i] = proc_wc[i] * 1000

    font_size = 18
    tick_size = 10
    line_width = 1.2

    # line
    xpoints = np.array(Xaxis)
    prom_sys_y = np.array(prom_sys)
    prom_wc_y = np.array(prom_wc)
    proc_sys_y = np.array(proc_sys)
    proc_wc_y = np.array(proc_wc)

    plt.figure(figsize=(5,10))
    plt.tick_params(axis='y', which='major', labelsize=3)
    plt.yticks(fontsize=tick_size)
    plt.rcParams.update({'font.size': font_size})
    plt.subplot(2,1,1)
    plt.title("procdfs vs Prometheus: Kernel + User Space Time", fontsize=font_size)
    plt.plot(xpoints, proc_sys_y, marker='o', color='b', label="procdfs", linewidth=line_width)
    plt.plot(xpoints, prom_sys_y, marker='x', color='r', label="Prometheus", linewidth=line_width)
    plt.xlabel("Number of Machines", fontsize=font_size)
    plt.ylabel("Latency (ms)", fontsize=font_size)
    plt.legend()
    plt.ylim(ymin=0, ymax=16)
    plt.xlim(xmin=0)
    plt.grid(True)
    plt.xticks([0,1,2,3,4,5,6,7,8,9,10], fontsize=tick_size + 7)
    plt.subplot(2,1,2)
    plt.title("procdfs vs Prometheus: Wall Clock Time", fontsize=font_size)
    plt.plot(xpoints, proc_wc_y, marker='o', color='b', label="procdfs", linewidth=line_width)
    plt.plot(xpoints, prom_wc_y, marker='x', color='r', label="Prometheus", linewidth=line_width)
    plt.xlabel("Number of Machines", fontsize=font_size)
    plt.ylabel("Latency (ms)", fontsize=font_size)
    plt.tight_layout(h_pad=1)
    plt.ylim(ymin=0, ymax=160)
    plt.xlim(xmin=0)
    plt.grid(True)
    plt.legend()
    # plt.savefig("output.pdf", bbox_inches='tight')
    plt.xticks([0,1,2,3,4,5,6,7,8,9,10], fontsize=tick_size + 7)
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