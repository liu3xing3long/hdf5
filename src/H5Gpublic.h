/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Gproto.h
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public declarations for the H5G package (symbol
 *                      tables).
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Gpublic_H
#define _H5Gpublic_H

/* Public headers needed by this file */
#include <sys/types.h>
#include <H5public.h>
#include <H5Ipublic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef herr_t (*H5G_iterate_t)(hid_t group, const char *group_name,
				void *op_data);

hid_t H5Gcreate (hid_t file_id, const char *name, size_t size_hint);
hid_t H5Gopen (hid_t file_id, const char *name);
herr_t H5Gclose (hid_t grp_id);
herr_t H5Gset (hid_t file, const char *name);
herr_t H5Gpush (hid_t file, const char *name);
herr_t H5Gpop (hid_t file);
herr_t H5Giterate (hid_t file, const char *name, int *idx, H5G_iterate_t op,
		   void *op_data);

#ifdef __cplusplus
}
#endif
#endif
