#include "mail.h"



int get_mailbox_GID(int lpid)
{
     return lpid;
}

int get_post_office_GID(int lpid)
{
     return total_mailboxes + lpid;
}

int get_assigned_post_office_LID(int lpid)
{
     return lpid % total_post_offices;
}

int get_assigned_post_office_GID(int lpid)
{
     return get_post_office_GID(get_assigned_post_office_LID(lpid));
}


tw_lpid lpTypeMapper(tw_lpid gid)
{
     // printf("%llu\n",gid);
     if (gid < total_mailboxes)
     {
          // printf("%llu < %i\n",gid,total_mailboxes);
          return MAILBOX;
     }
     else
     {
          // printf("%llu >= %i\n",gid,total_mailboxes);
          return POSTOFFICE;
     }

}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid mail_map(tw_lpid gid)
{
     return (tw_peid) gid / g_tw_nlp;
}
