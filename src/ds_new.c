#include "ds_new.h"

#include <stdio.h>

int req_tracker_ll_add(Request_tracker_node **rtn, Request *req) {

    if(req == NULL)
        return -1;

    Request_tracker_node *rtnptr = *rtn;
    if(rtnptr->req == NULL) {
        // empty ll
        rtnptr->req = req;
        rtnptr->next = NULL;
    } else {
        while(rtnptr->next != NULL)
            rtnptr = rtnptr->next;
        rtnptr->next->req = req;
        rtnptr->next->next = NULL;
    }

    return 0;
}