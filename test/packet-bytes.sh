#!/bin/bash
# George Sims

pnetd="/proc/net/dev"
wlp="wlp58s0"
lo="lo"
tun="tun0"

ret_arr() {
   #1: device name

   echo $(cat $pnetd | grep $1 | awk '{print $2}')  # bRX
   echo $(cat $pnetd | grep $1 | awk '{print $10}') # bTX
   echo $(cat $pnetd | grep $1 | awk '{print $3}')  # pRX
   echo $(cat $pnetd | grep $1 | awk '{print $11}') # pTX
}

print_rows() {
    #1: device name
    #2: bRX    #3: bTX
    #4: pRX    #5: pTX

    echo "\t\tRX Pkts/s\t" "TX Pkts/s\t" "RX Data/s\t" "TX Data/s\t"
}

echo -e "\033[?25l\033[2J\033[0;0H IFSTAT\n ------\n #kernel\n" # hide cursor & clear screen
while true; do

    wlp_pre=($( ret_arr  $wlp))
    lo_pre=($(  ret_arr  $lo))
    tun_pre=($( ret_arr  $tun))
    sleep 1
    wlp_post=($(ret_arr $wlp))
    lo_post=($( ret_arr $lo))
    tun_post=($(ret_arr $tun))

    w_bRX=$(( ${wlp_post[0]} - ${wlp_pre[0]} ))
    w_bTX=$(( ${wlp_post[1]} - ${wlp_pre[1]} ))
    w_pRX=$(( ${wlp_post[2]} - ${wlp_pre[2]} ))
    w_pTX=$(( ${wlp_post[3]} - ${wlp_pre[3]} ))

    l_bRX=$(( ${lo_post[0]} - ${lo_pre[0]} ))
    l_bTX=$(( ${lo_post[1]} - ${lo_pre[1]} ))
    l_pRX=$(( ${lo_post[2]} - ${lo_pre[2]} ))
    l_pTX=$(( ${lo_post[3]} - ${lo_pre[3]} ))

    t_bRX=$(( ${tun_post[0]} - ${tun_pre[0]} ))
    t_bTX=$(( ${tun_post[1]} - ${tun_pre[1]} ))
    t_pRX=$(( ${tun_post[2]} - ${tun_pre[2]} ))
    t_pTX=$(( ${tun_post[3]} - ${tun_pre[3]} ))

    echo -e "\r\033[4;0H\033[2K $(print_rows)\n"                        \
            "\033[2K$lo\t\t $l_bRX\t\t $l_bTX\t\t $l_pRX\t\t $l_pTX\n"  \
            "\033[2K$wlp\t $w_bRX\t\t $w_bTX\t\t $w_pRX\t\t $w_pTX\n"   \
            "\033[2K$tun\t\t $t_bRX\t\t $t_bTX\t\t $t_pRX\t\t $t_pTX\t"

done
