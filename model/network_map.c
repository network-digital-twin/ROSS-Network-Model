#include <assert.h>
#include "network.h"
tw_lpid *switch_LID_to_terminal_GID = NULL;
tw_lpid *terminal_GID_to_switch_LID = NULL; // reverse mapping. index 0 represents terminal with GID='total_switches'.

/**
 * Initialise global variable 'switch_LID_to_terminal_GID'
 * @param count number of switches that have terminals, should be the same size as 'switch_LIDs'
 * @param switch_LIDs the LIDs of the switches that have terminals
 */
void init_mapping(int count, const tw_lpid *switch_LIDs) {
    // Initialise the global variable
    switch_LID_to_terminal_GID = (tw_lpid *)malloc(total_switches * sizeof(tw_lpid));
    terminal_GID_to_switch_LID = (tw_lpid *)malloc(total_terminals * sizeof(tw_lpid));
    // First initialise all entries to -1
    for (int i = 0; i < total_switches; i++){
        switch_LID_to_terminal_GID[i] = -1; // TODO: change -1 to a better value, since tw_lpid is unsigned
    }
    // Then set the entries that have terminals attached to them
    for (int i = 0; i < count; i++){
        switch_LID_to_terminal_GID[switch_LIDs[i]] = i + total_switches;
        terminal_GID_to_switch_LID[i] = switch_LIDs[i];
    }
}

// terminal LID -> terminal GID
tw_lpid get_terminal_GID(tw_lpid t_lid)
{
    // terminal_LID == switch_LID
    return get_attached_terminal_GID(t_lid);
}

// terminal GID -> terminal LID
tw_lpid get_terminal_LID(tw_lpid t_gid)
{
    // terminal_LID == switch_LID == switch_GID
    return terminal_GID_to_switch_LID[t_gid - total_switches];
}

/**
 * switch LID -> switch GID
 * @param s_lid switch LID
 * @return
 */
tw_lpid get_switch_GID(tw_lpid s_lid)
{
     return s_lid;
}

// terminal LID -> assigned switch GID
tw_lpid get_assigned_switch_GID(tw_lpid t_lid)
{
    assert(t_lid < total_switches);
     return t_lid;
}

/**
 * switch LID -> attached terminal LID
 * @param s_lid switch LID
 * @return terminal LID; -1 if not exist.
 */
tw_lpid get_attached_terminal_LID(tw_lpid s_lid)
{
    if (switch_LID_to_terminal_GID[s_lid] != -1) {
        // An attached terminal exists.
        return s_lid;
    } else {
        return -1;
    }
}

/**
 * switch LID -> attached terminal GID
 * @param s_lid switch LID
 * @return terminal GID; -1 if not exist.
 */
tw_lpid get_attached_terminal_GID(tw_lpid s_lid)
{
     return switch_LID_to_terminal_GID[s_lid];
}


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
