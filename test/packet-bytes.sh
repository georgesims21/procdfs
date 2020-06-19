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

print_res() {
    #1: device name
    #2: bRX    #3: bTX
    #4: pRX    #5: pTX

    #echo "$1\t: RX Pkts/s:\t" "$4\t" "TX Pkts/s:\t" "$5\t" "RX Data/s:\t" "$2\t" "TX Data/s:\t" "$3\t"
    if(( $1 == $wlp ))
    then
        echo "$1: $4\t $5\t\t $2\t\t $3\t\t "
    else
        echo "$1: $4\t\t $5\t\t $2\t\t $3\t\t "
    fi
}

print_rows() {
    #1: device name
    #2: bRX    #3: bTX
    #4: pRX    #5: pTX

    echo "\tRX Pkts/s\t" "TX Pkts/s:\t" "RX Data/s:\t" "TX Data/s:\t"
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

    echo -e "\r\033[4;0H\033[2K $(print_rows)\n"\
        "\033[2K$(print_res $lo $l_bRX $l_bTX $l_pRX $l_pTX)\n"\
        "\033[2K$(print_res $wlp $w_bRX $w_bTX $w_pRX $w_pTX)\n"\
    "\033[2K$(print_res $tun $t_bRX $t_bTX $t_pRX $t_pTX)\t"

done
