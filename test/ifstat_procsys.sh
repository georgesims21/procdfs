#!/bin/bash
# George Sims

pnetd="$HOME/github/procsys/mountdir/net/dev"
#pnetd="/proc/net/dev"
wlp="wlp58s0"
lo="lo"
tun="tun0"

ret_arr() {
   #1: device name

   cat $pnetd | grep "$1" | awk '{print $2}'  # bRX
   cat $pnetd | grep "$1" | awk '{print $10}' # bTX
   cat $pnetd | grep "$1" | awk '{print $3}'  # pRX
   cat $pnetd | grep "$1" | awk '{print $11}' # pTX
}

print_rows() {
    #1: device name
    #2: bRX    #3: bTX
    #4: pRX    #5: pTX

    printf "\t\tRX Pkts/s\tTX Pkts/s\tRX Data/s\tTX Data/s\t"
}

echo -e "\033[?25l\033[2J\033[0;0H IFSTAT\n ------\n #kernel\n" # hide cursor & clear screen
while true; do

    wlp_pre=($(ret_arr $wlp))
    lo_pre=($(ret_arr $lo))
    tun_pre=($(ret_arr $tun))
    sleep 1
    wlp_post=($(ret_arr $wlp))
    lo_post=($(ret_arr $lo))
    tun_post=($(ret_arr $tun))

    w_bRX=$((${wlp_pre[0]}))
    w_bTX=$((${wlp_pre[1]}))
    w_pRX=$((${wlp_pre[2]}))
    w_pTX=$((${wlp_pre[3]}))
    pw_bRX=$((${wlp_post[0]}))
    pw_bTX=$((${wlp_post[1]}))
    pw_pRX=$((${wlp_post[2]}))
    pw_pTX=$((${wlp_post[3]}))

    l_bRX=$((${lo_pre[0]}))
    l_bTX=$((${lo_pre[1]}))
    l_pRX=$((${lo_pre[2]}))
    l_pTX=$((${lo_pre[3]}))
    pl_bRX=$((${lo_post[0]}))
    pl_bTX=$((${lo_post[1]}))
    pl_pRX=$((${lo_post[2]}))
    pl_pTX=$((${lo_post[3]}))

    t_bRX=$((${tun_pre[0]}))
    t_bTX=$((${tun_pre[1]}))
    t_pRX=$((${tun_pre[2]}))
    t_pTX=$((${tun_pre[3]}))
    pt_bRX=$((${tun_post[0]}))
    pt_bTX=$((${tun_post[1]}))
    pt_pRX=$((${tun_post[2]}))
    pt_pTX=$((${tun_post[3]}))


    fw_bRX=$(($pl_bRX-$l_bRX))
    fw_bTX=$(($pl_bTX-$l_bTX))
    fw_pRX=$(($pl_pRX-$l_pRX))
    fw_pTX=$(($pl_pTX-$l_pTX))

    fl_bRX=$(($pw_bRX-$w_bRX))
    fl_bTX=$(($pw_bTX-$w_bTX))
    fl_pRX=$(($pw_pRX-$w_pRX))
    fl_pTX=$(($pw_pTX-$w_pTX))

    ft_bRX=$(($pt_bRX-$t_bRX))
    ft_bTX=$(($pt_bTX-$t_bTX))
    ft_pRX=$(($pt_pRX-$t_pRX))
    ft_pTX=$(($pt_pTX-$t_pTX))

    echo -e "\r\033[4;0H\033[2K $(print_rows)\n"                            \
            "\033[2K$lo\t\t  $fl_bRX\t\t  $fl_bTX\t\t  $fl_pRX\t\t  $fl_pTX\n"  \
            "\033[2K$wlp\t  $fw_bRX\t\t  $fw_bTX\t\t  $fw_pRX\t\t  $fw_pTX\n"   \
            "\033[2K$tun\t\t  $ft_bRX\t\t  $ft_bTX\t\t  $ft_pRX\t\t  $ft_pTX\t"

done
