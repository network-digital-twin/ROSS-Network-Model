#include "network.h"


tw_lpid lpTypeMapper(tw_lpid gid)
{
     // printf("%llu\n",gid);
     if (gid < total_switches)
     {
         // printf("%llu >= %i\n",gid,total_terminals);
         return SWITCH;
     }
     else
     {
         // printf("%llu < %i\n",gid,total_terminals);
         return TERMINAL;
     }

}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid network_map(tw_lpid gid)
{
     return (tw_peid) gid / g_tw_nlp;
}
