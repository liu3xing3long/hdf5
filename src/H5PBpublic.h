/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _H5PBpublic_H
#define _H5PBpublic_H

#include "H5public.h"

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5PBreset_stats(hid_t file_id);
H5_DLL herr_t H5PBget_stats(hid_t file_id, int accesses[2], int hits[2], int misses[2], int evictions[2], int bypasses[2]);

#ifdef __cplusplus
}
#endif
#endif
