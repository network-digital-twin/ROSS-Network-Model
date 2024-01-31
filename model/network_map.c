#include "network.h"



int get_terminal_GID(int lpid)
{
     return lpid;
}

int get_switch_GID(int lpid)
{
     return total_terminals + lpid;
}

int get_assigned_switch_LID(int lpid)
{
     return lpid % total_switches;
}

int get_assigned_switch_GID(int lpid)
{
     return get_switch_GID(get_assigned_switch_LID(lpid));
}


tw_lpid lpTypeMapper(tw_lpid gid)
{
     // printf("%llu\n",gid);
     if (gid < total_terminals)
     {
          // printf("%llu < %i\n",gid,total_terminals);
          return TERMINAL;
     }
     else
     {
          // printf("%llu >= %i\n",gid,total_terminals);
          return SWITCH;
     }

}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid network_map(tw_lpid gid)
{
     return (tw_peid) gid / g_tw_nlp;
}
