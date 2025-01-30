/* Implementação do escalonamento Lottery no Minix 3.4.0rc6 */
#include "sched.h"
#include "schedproc.h"
#include <assert.h>
#include <minix/com.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_TICKETS 10
#define MAX_TICKETS 100

static int schedule_process(struct schedproc *rmp, unsigned flags);

static int lottery_draw(void) {
    return rand() % MAX_TICKETS + 1;
}

static int pick_winner(void) {
    int total_tickets = 0;
    int winner_ticket;
    struct schedproc *rmp;
    
    for (int i = 0; i < NR_PROCS; i++) {
        if (schedproc[i].flags & IN_USE) {
            total_tickets += schedproc[i].priority; 
        }
    }
    
    if (total_tickets == 0) {
        return -1;
    }
    
    winner_ticket = lottery_draw() % total_tickets;
    total_tickets = 0;
    
    for (int i = 0; i < NR_PROCS; i++) {
        rmp = &schedproc[i];
        if (rmp->flags & IN_USE) {
            total_tickets += rmp->priority;
            if (total_tickets >= winner_ticket) {
                return i;
            }
        }
    }
    return -1;
}

int do_noquantum(message *m_ptr) {
    struct schedproc *rmp;
    int proc_nr_n;
    
    if (sched_isokendpt(m_ptr->m_source, &proc_nr_n) != OK) {
        return EBADEPT;
    }
    
    rmp = &schedproc[proc_nr_n];
    rmp->priority = (rmp->priority > 1) ? rmp->priority - 1 : 1;
    
    return schedule_process(rmp, SCHEDULE_CHANGE_PRIO);
}

int do_start_scheduling(message *m_ptr) {
    struct schedproc *rmp;
    int proc_nr_n, rv;
    
    if ((rv = sched_isemtyendpt(m_ptr->m_lsys_sched_scheduling_start.endpoint, &proc_nr_n)) != OK) {
        return rv;
    }
    
    rmp = &schedproc[proc_nr_n];
    rmp->endpoint = m_ptr->m_lsys_sched_scheduling_start.endpoint;
    rmp->priority = DEFAULT_TICKETS;
    rmp->flags = IN_USE;
    
    return schedule_process(rmp, SCHEDULE_CHANGE_ALL);
}

static int schedule_process(struct schedproc *rmp, unsigned flags) {
    int winner_proc = pick_winner();
    if (winner_proc < 0) {
        return EINVAL;
    }
    
    rmp = &schedproc[winner_proc];
    return sys_schedule(rmp->endpoint, SCHEDULING_CHANGE_PRIO, &rmp->priority, 0, 0);
}
