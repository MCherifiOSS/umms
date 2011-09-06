#include <string.h>
#include <stdio.h>
#include "umms-gtk-backend.h"

gboolean 
umms_backend_resume (void)
{
    printf("line:%d ---> call umms_backend_resume\n", __LINE__);
    return TRUE;
}


gboolean 
umms_backend_pause (void)
{
    printf("line:%d ---> call umms_backend_pause\n", __LINE__);
    return TRUE;
}


gboolean 
umms_backend_reset (void)
{
    printf("line:%d ---> call umms_backend_reset\n", __LINE__);
    return TRUE;
}


gboolean 
umms_backend_seek_relative (gint to_seek)
{
    printf("line:%d ---> call umms_backend_seek_relative\n", __LINE__);
    return TRUE;
}


gboolean 
umms_backend_seek_absolute (gint to_seek)
{
    printf("line:%d ---> call umms_backend_seek_absolute\n", __LINE__);
    return TRUE;
}


gint
umms_backend_query_duration (void)
{
    printf("line:%d ---> call umms_backend_query_duration\n", __LINE__);
    return TRUE;
}


gint 
umms_backend_query_position (void)
{
    printf("line:%d ---> call umms_backend_query_position\n", __LINE__);
    return TRUE;
}

