/****************************************************************************
* NCSA HDF                                                                 *
* Software Development Group                                               *
* National Center for Supercomputing Applications                          *
* University of Illinois at Urbana-Champaign                               *
* 605 E. Springfield, Champaign IL 61820                                   *
*                                                                          *
* For conditions of distribution and use, see the accompanying             *
* hdf/COPYING file.                                                        *
*
* MODIFICATIONS
* 	Robb Matzke, 30 Aug 1997
*	Added `ERRORS' fields to function prologues.
*                                                                          *
****************************************************************************/

#ifdef RCSID
static char RcsId[] = "@(#)$Revision$";
#endif

/* $Id$ */

/*LINTLIBRARY */
/*
   FILE
       hdf5file.c
   HDF5 file I/O routines

   EXPORTED ROUTINES
       H5Fcreate    -- Create an HDF5 file
       H5Fclose     -- Close an open HDF5 file

   LIBRARY-SCOPED ROUTINES

   LOCAL ROUTINES
       H5F_init_interface    -- initialize the H5F interface
 */

/* Packages needed by this file... */
#include <H5private.h>      	/*library functions			*/
#include <H5Aprivate.h>		/*atoms					*/
#include <H5ACprivate.h>	/*cache					*/
#include <H5Cprivate.h>		/*templates				*/
#include <H5Eprivate.h>		/*error handling			*/
#include <H5Gprivate.h>		/*symbol tables				*/
#include <H5Mprivate.h>		/*meta data				*/
#include <H5MMprivate.h>	/*core memory management		*/

#include <sys/types.h>
#include <sys/stat.h>

/*
 * Define the following if you want H5F_block_read() and H5F_block_write() to
 * keep track of the file position and attempt to minimize calls to the file
 * seek method.
 */
#define H5F_OPT_SEEK


#define PABLO_MASK	H5F_mask

/*--------------------- Locally scoped variables -----------------------------*/

/* Whether we've installed the library termination function yet for this interface */
static intn interface_initialize_g = FALSE;

/*--------------------- Local function prototypes ----------------------------*/
static herr_t H5F_init_interface(void);
static H5F_t *H5F_new (H5F_file_t *shared);
static H5F_t *H5F_dest (H5F_t *f);
static herr_t H5F_flush (H5F_t *f, hbool_t invalidate);
static herr_t H5F_close (H5F_t *f);

/*--------------------------------------------------------------------------
NAME
   H5F_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5F_init_interface()
   
RETURNS
   SUCCEED/FAIL
DESCRIPTION
    Initializes any interface-specific data or routines.

ERRORS

Modifications:
    Robb Matzke, 4 Aug 1997
    Changed pablo mask from H5_mask to H5F_mask for the FUNC_LEAVE call.
    It was already H5F_mask for the PABLO_TRACE_ON call.

--------------------------------------------------------------------------*/
static herr_t H5F_init_interface(void)
{
    herr_t ret_value = SUCCEED;
    FUNC_ENTER (H5F_init_interface, NULL, FAIL);

    /* Initialize the atom group for the file IDs */
    if((ret_value=H5Ainit_group(H5_FILE,H5A_FILEID_HASHSIZE,0,NULL))!=FAIL)
        ret_value=H5_add_exit(&H5F_term_interface);

    FUNC_LEAVE(ret_value);
}	/* H5F_init_interface */

/*--------------------------------------------------------------------------
 NAME
    H5F_term_interface
 PURPOSE
    Terminate various H5F objects
 USAGE
    void H5F_term_interface()
 RETURNS
    SUCCEED/FAIL
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void H5F_term_interface (void)
{
    H5Adestroy_group(H5_FILE);
} /* end H5F_term_interface() */

#ifdef LATER
/*--------------------------------------------------------------------------
 NAME
       H5F_encode_length_unusual -- encode an unusual length size
 USAGE
       void H5F_encode_length_unusual(f, p, l)
       const H5F_t *f;             IN: pointer to the file record
       uint8 **p;               IN: pointer to buffer pointer to encode length in
       uint8 *l;                IN: pointer to length to encode

 ERRORS

 RETURNS
    none
 DESCRIPTION
    Encode non-standard (i.e. not 2, 4 or 8-byte) lengths in file meta-data.
--------------------------------------------------------------------------*/
void H5F_encode_length_unusual(const H5F_t *f, uint8 **p, uint8 *l)
{
    intn i = H5F_SIZEOF_SIZE (f);

#ifdef WORDS_BIGENDIAN
    /*
     * For non-little-endian platforms, encode each byte in memory backwards.
     */
    for(; i>=0; i--,(*p)++)
        *(*p)=*(l+i);
#else
    /* platform has little-endian integers */
    for(; i>=0; i--,(*p)++)
        *(*p)=*l;
#endif

#ifdef LATER
done:
    if(ret_value == FALSE)   
      { /* Error condition cleanup */

      } /* end if */
#endif /* LATER */

    /* Normal function cleanup */

}	/* H5F_encode_length_unusual */

/*--------------------------------------------------------------------------
 NAME
       H5F_encode_offset_unusual -- encode an unusual offset size
 USAGE
       void H5F_encode_offset_unusual(f, p, o)
       const H5F_t *f;             IN: pointer to the file record
       uint8 **p;               IN: pointer to buffer pointer to encode offset in
       uint8 *o;                IN: pointer to offset to encode

ERRORS

 RETURNS
    none
 DESCRIPTION
    Encode non-standard (i.e. not 2, 4 or 8-byte) offsets in file meta-data.
--------------------------------------------------------------------------*/
void H5F_encode_offset_unusual(const H5F_t *f, uint8 **p, uint8 *o)
{
    intn i = H5F_SIZEOF_OFFSET(f);

#ifdef WORDS_BIGENDIAN
    /*
     * For non-little-endian platforms, encode each byte in memory backwards.
     */
    for(; i>=0; i--,(*p)++)
        *(*p)=*(o+i);
#else
    /* platform has little-endian integers */
    for(; i>=0; i--,(*p)++)
        *(*p)=*o;
#endif

#ifdef LATER
done:
    if(ret_value == FALSE)   
      { /* Error condition cleanup */

      } /* end if */
#endif /* LATER */

    /* Normal function cleanup */

}	/* H5F_encode_offset_unusual */
#endif /* LATER */

/*--------------------------------------------------------------------------
 NAME
       H5F_compare_files -- compare file objects for the atom API
 USAGE
       intn HPcompare_filename(obj, key)
       const VOIDP obj;             IN: pointer to the file record
       const VOIDP key;             IN: pointer to the search key

 ERRORS

 RETURNS
       TRUE if the key matches the obj, FALSE otherwise
 DESCRIPTION
       Look inside the file record for the atom API and compare the the
       keys.
--------------------------------------------------------------------------*/
static intn
H5F_compare_files (const VOIDP _obj, const VOIDP _key)
{
   const H5F_t		*obj = (const H5F_t *)_obj;
   const H5F_search_t	*key = (const H5F_search_t *)_key;
   int			ret_value = FALSE;
   
   FUNC_ENTER (H5F_compare_files, NULL, FALSE);

   ret_value = (obj->shared->key.dev == key->dev &&
		obj->shared->key.ino == key->ino);

   FUNC_LEAVE (ret_value);
}

/*--------------------------------------------------------------------------
 NAME
    H5Fget_create_template

 PURPOSE
    Get an atom for a copy of the file-creation template for this file

 USAGE
    hid_t H5Fget_create_template(fid)
        hid_t fid;    IN: File ID

 ERRORS
    ATOM      BADATOM       Can't get file struct. 
    FUNC      CANTCREATE    Can't create template. 
    FUNC      CANTINIT      Can't init template. 

 RETURNS
    Returns template ID on success, FAIL on failure

 DESCRIPTION
        This function returns an atom with a copy of the template parameters
    used to create a file.
--------------------------------------------------------------------------*/
hid_t H5Fget_create_template(hid_t fid)
{
    H5F_t *file=NULL;         /* file struct for file to close */
    hid_t ret_value = FAIL;

    FUNC_ENTER(H5Fget_create_template, H5F_init_interface, FAIL);

    /* Clear errors and check args and all the boring stuff. */
    H5ECLEAR;

    /* Get the file structure */
    if((file=H5Aatom_object(fid))==NULL)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL); /*can't get file struct*/

    /* Create the template object to return */
    if((ret_value=H5Mcreate(fid,H5_TEMPLATE,NULL))==FAIL)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTCREATE, FAIL); /*can't create template*/

    if(H5C_init(ret_value,&(file->shared->file_create_parms))==FAIL)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL); /*can't init template*/

done:
  if(ret_value == FAIL)   
    { /* Error condition cleanup */

    } /* end if */

    /* Normal function cleanup */

    FUNC_LEAVE(ret_value);
} /* end H5Fget_create_template() */

/*--------------------------------------------------------------------------
 NAME
    H5Fis_hdf5

 PURPOSE
    Check the file signature to detect an HDF5 file.

 USAGE
    hbool_t H5Fis_hdf5(filename)
        const char *filename;   IN: Name of the file to check
 ERRORS
    ARGS      BADRANGE      No filename specified. 
    FILE      BADFILE       Low-level file open failure. 
    IO        READERROR     Read error. 
    IO        READERROR     Seek error. 
    IO        SEEKERROR     Unable to determine length of file due to seek
                            failure. 

 RETURNS
    TRUE/FALSE/FAIL

 DESCRIPTION
    This function determines if a file is an HDF5 format file.
--------------------------------------------------------------------------*/
hbool_t H5Fis_hdf5(const char *filename)
{
    hdf_file_t f_handle=H5F_INVALID_FILE;      /* file handle */
    uint8 temp_buf[H5F_SIGNATURE_LEN];    /* temporary buffer for checking file signature */
    haddr_t curr_off=0;          /* The current offset to check in the file */
    size_t file_len=0;          /* The length of the file we are checking */
    hbool_t ret_value = BFALSE;

    FUNC_ENTER(H5Fis_hdf5, H5F_init_interface, BFAIL);

    /* Clear errors and check args and all the boring stuff. */
    H5ECLEAR;
    if(filename==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, BFAIL); /*no filename specified*/

    /* Open the file */
    f_handle=H5F_OPEN(filename,0);
    if(H5F_OPENERR(f_handle)) {
       /* Low-level file open failure */
       HGOTO_ERROR(H5E_FILE, H5E_BADFILE, BFAIL);
    }

    /* Get the length of the file */
    if(H5F_SEEKEND(f_handle)==FAIL) {
       /* Unable to determine length of file due to seek failure */
       HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, BFAIL);
    }
    file_len=H5F_TELL(f_handle);

    /* Check the offsets where the file signature is possible */
    while(curr_off<file_len)
      {
        if(H5F_SEEK(f_handle,curr_off)==FAIL)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, BFAIL); /*seek error*/
        if(H5F_READ(f_handle,temp_buf, H5F_SIGNATURE_LEN)==FAIL)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, BFAIL); /*read error*/
        if(HDmemcmp(temp_buf,H5F_SIGNATURE,H5F_SIGNATURE_LEN)==0)
          {
            ret_value=BTRUE;
            break;
          } /* end if */
        if(curr_off==0)
            curr_off=512;
        else
            curr_off*=2;
      } /* end while */

 done:
    if(f_handle!=H5F_INVALID_FILE)
       H5F_CLOSE(f_handle);   /* close the file we opened */

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_new
 *
 * Purpose:	Creates a new file object and initializes it.  The
 *		H5Fopen and H5Fcreate functions then fill in various
 *		fields.  If SHARED is a non-null pointer then the shared info
 *		to which it points has the reference count incremented.
 *		Otherwise a new, empty shared info struct is created.
 *
 * Errors:
 *
 * Return:	Success:	Ptr to a new file struct.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5F_t *
H5F_new (H5F_file_t *shared)
{
   H5F_t	*f = NULL;
   FUNC_ENTER (H5F_new, H5F_init_interface, NULL);

   f = H5MM_xcalloc (1, sizeof(H5F_t));
   f->shared = shared;
   
   if (!f->shared) {
      f->shared = H5MM_xcalloc (1, sizeof(H5F_file_t));

      /* Create a main cache */
      H5AC_new (f, H5AC_NSLOTS);

      /* Create the shadow hash table */
      f->shared->nshadows = H5G_NSHADOWS;
      f->shared->shadow = H5MM_xcalloc (f->shared->nshadows,
					sizeof(struct H5G_hash_t*));

      /* Create a root symbol slot */
      f->shared->root_sym = H5G_ent_calloc ();
   }

   f->shared->nrefs++;

   FUNC_LEAVE (f);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_dest
 *
 * Purpose:	Destroys a file structure.  This function does not flush
 *		the cache or anything else; it only frees memory associated
 *		with the file struct.  The shared info for the file is freed
 *		only when its reference count reaches zero.
 *
 * Errors:
 *
 * Return:	Success:	NULL
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5F_t *
H5F_dest (H5F_t *f)
{
   FUNC_ENTER (H5F_dest, H5F_init_interface, NULL);
   
   if (f) {
      if (0 == --(f->shared->nrefs)) {
	 H5AC_dest (f);
	 f->shared->root_sym = H5MM_xfree (f->shared->root_sym);
	 f->shared->nshadows = 0;
	 f->shared->shadow = H5MM_xfree (f->shared->shadow);
	 f->shared = H5MM_xfree (f->shared);
      }
      f->name = H5MM_xfree (f->name);
      H5MM_xfree (f);
   }
   
   FUNC_LEAVE (NULL);
}



/*-------------------------------------------------------------------------
 * Function:	H5F_open
 *
 * Purpose:	Opens (or creates) a file.  This function understands the
 *		following flags which are similar in nature to the Posix
 *		open(2) flags.
 *
 *		H5F_ACC_WRITE:	Open with read/write access. If the file is
 *				currently open for read-only access then it
 *				will be reopened. Absence of this flag
 *				implies read-only access.
 *
 *		H5F_ACC_CREAT:	Create a new file if it doesn't exist yet.
 *				The permissions are 0666 bit-wise AND with
 *				the current umask.  H5F_ACC_WRITE must also
 *				be specified.
 *
 *		H5F_ACC_EXCL:	This flag causes H5F_open() to fail if the
 *				file already exists.
 *
 *		H5F_ACC_TRUNC:	The file is truncated and a new HDF5 boot
 *				block is written.  This operation will fail
 *				if the file is already open.
 *
 *		Unlinking the file name from the directory hierarchy while
 *		the file is opened causes the file to continue to exist but
 *		one will not be able to upgrade the file from read-only
 *		access to read-write access by reopening it. Disk resources
 *		for the file are released when all handles to the file are
 *		closed. NOTE: This paragraph probably only applies to Unix;
 *		deleting the file name in other OS's has undefined results.
 *
 * Errors:
 *		FILE      BADVALUE      Can't create file without write
 *		                        intent. 
 *		FILE      BADVALUE      Can't truncate without write intent. 
 *		FILE      CANTCREATE    Can't create file. 
 *		FILE      CANTCREATE    Can't stat file. 
 *		FILE      CANTCREATE    Can't truncate file. 
 *		FILE      CANTINIT      Can't write file boot block. 
 *		FILE      CANTINIT      Cannot determine file size. 
 *		FILE      CANTOPENFILE  Bad boot block version number. 
 *		FILE      CANTOPENFILE  Bad free space version number. 
 *		FILE      CANTOPENFILE  Bad length size. 
 *		FILE      CANTOPENFILE  Bad object dir version number. 
 *		FILE      CANTOPENFILE  Bad offset size. 
 *		FILE      CANTOPENFILE  Bad shared header version number. 
 *		FILE      CANTOPENFILE  Bad small object heap version number. 
 *		FILE      CANTOPENFILE  Bad symbol table internal node 1/2
 *		                        rank. 
 *		FILE      CANTOPENFILE  Bad symbol table leaf node 1/2 rank. 
 *		FILE      CANTOPENFILE  Can't read root symbol entry. 
 *		FILE      CANTOPENFILE  Cannot open existing file. 
 *		FILE      CANTOPENFILE  File cannot be reopened with write
 *		                        access. 
 *		FILE      CANTOPENFILE  File does not exist. 
 *		FILE      FILEEXISTS    File already exists - CREAT EXCL
 *		                        failed. 
 *		FILE      FILEOPEN      File already open - TRUNC failed. 
 *		FILE      NOTHDF5       Can't read boot block. 
 *		FILE      READERROR     File is not readable. 
 *		FILE      WRITEERROR    File is not writable. 
 *
 * Return:	Success:	Ptr to the file pointer.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Tuesday, September 23, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5F_t *
H5F_open (const char *name, uintn flags,
	  const file_create_temp_t *create_parms)
{
   H5F_t	*f = NULL;		/*return value			*/
   H5F_t	*ret_value = NULL;	/*a copy of `f'			*/
   H5F_t	*old = NULL;		/*a file already opened		*/
   struct stat	sb;			/*file stat info		*/
   H5F_search_t search;			/*file search key		*/
   hdf_file_t	fd = H5F_INVALID_FILE;	/*low level file desc		*/
   hbool_t	empty_file = FALSE;	/*is file empty?		*/
   hbool_t	file_exists = FALSE;	/*file already exists		*/
   uint8	buf[256], *p=NULL;	/*I/O buffer and ptr into it	*/
   size_t	fixed_size = 24;	/*size of fixed part of boot blk*/
   size_t	variable_size;		/*variable part of boot block	*/
   intn		i;
   file_create_temp_t *cp=NULL;		/*file creation parameters	*/
   
   FUNC_ENTER (H5F_open, H5F_init_interface, NULL);

   assert (name && *name);

   /*
    * Does the file exist?  If so, get the device and i-node values so we can
    * compare them with other files already open.  On Unix (and other systems
    * with hard or soft links) it doesn't work to compare files based only on
    * their full path name.
    */
   file_exists = (stat (name, &sb)>=0);

   /*
    * Open the low-level file (if necessary) and create an H5F_t struct that
    * points to an H5F_file_t struct.
    */
   if (file_exists) {
      if (flags & H5F_ACC_EXCL) {
	 /* File already exists - CREAT EXCL failed */
	 HRETURN_ERROR (H5E_FILE, H5E_FILEEXISTS, NULL);
      }
      if (access (name, R_OK)<0) {
	 /* File is not readable */
	 HRETURN_ERROR (H5E_FILE, H5E_READERROR, NULL);
      }
      if ((flags & H5F_ACC_WRITE) && access (name,  W_OK)<0) {
	 /* File is not writable */
	 HRETURN_ERROR (H5E_FILE, H5E_WRITEERROR, NULL);
      }

      search.dev = sb.st_dev;
      search.ino = sb.st_ino;
      if ((old=H5Asearch_atom (H5_FILE, H5F_compare_files, &search))) {
	 if (flags & H5F_ACC_TRUNC) {
	    /* File already open - TRUNC failed */
	    HRETURN_ERROR (H5E_FILE, H5E_FILEOPEN, NULL);
	 }
	 if ((flags & H5F_ACC_WRITE) &&
	     0==(old->shared->flags & H5F_ACC_WRITE)) {
	    if (H5F_INVALID_FILE==(fd = H5F_OPEN (name, H5ACC_WRITE))) {
	       /* File cannot be reopened with write access */
	       HRETURN_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	    }
	    H5F_CLOSE (old->shared->file_handle);
	    old->shared->file_handle = fd;
	    old->shared->flags |=  H5F_ACC_WRITE;
	    fd = H5F_INVALID_FILE; /*so we don't close it during error*/
	 }
	 f = H5F_new (old->shared);
	 
      } else if (flags & H5F_ACC_TRUNC) {
	 /* Truncate existing file */
	 if (0==(flags & H5F_ACC_WRITE)) {
	    /* Can't truncate without write intent */
	    HRETURN_ERROR (H5E_FILE, H5E_BADVALUE, NULL);
	 }
	 if (H5F_INVALID_FILE==(fd=H5F_CREATE (name))) {
	    /* Can't truncate file */
	    HRETURN_ERROR (H5E_FILE, H5E_CANTCREATE, NULL);
	 }
	 f = H5F_new (NULL);
	 f->shared->key.dev = sb.st_dev;
	 f->shared->key.ino = sb.st_ino;
	 f->shared->flags = flags;
	 f->shared->file_handle = fd;
	 empty_file = TRUE;
	 
      } else {
	 fd = H5F_OPEN (name, (flags & H5F_ACC_WRITE)?H5ACC_WRITE : 0);
	 if (H5F_INVALID_FILE==fd) {
	    /* Cannot open existing file */
	    HRETURN_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }
	 f = H5F_new (NULL);
	 f->shared->key = search;
	 f->shared->flags = flags;
	 f->shared->file_handle = fd;
      }
      
   } else if (flags & H5F_ACC_CREAT) {
      if (0==(flags & H5F_ACC_WRITE)) {
	 /* Can't create file without write intent */
	 HRETURN_ERROR (H5E_FILE, H5E_BADVALUE, NULL);
      }
      if (H5F_INVALID_FILE==(fd=H5F_CREATE (name))) {
	 /* Can't create file */
	 HRETURN_ERROR (H5E_FILE, H5E_CANTCREATE, NULL);
      }
      if (stat (name, &sb)<0) {
	 /* Can't stat file */
	 HRETURN_ERROR (H5E_FILE, H5E_CANTCREATE, NULL);
      }
      f = H5F_new (NULL);
      f->shared->key.dev = sb.st_dev;
      f->shared->key.ino = sb.st_ino;
      f->shared->flags = flags;
      f->shared->file_handle = fd;
      empty_file = TRUE;
      
   } else {
      /* File does not exist */
      HRETURN_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
   }
   assert (f);

   /*
    * The intent at the top level file struct are not necessarily the same as
    * the flags at the bottom.  The top level describes how the file can be
    * accessed through the HDF5 library.  The bottom level describes how the
    * file can be accessed through the C library.
    */
   f->intent = flags;
   f->name = H5MM_xstrdup (name);
   
   /*
    * Update the file creation parameters with default values if this is the
    * first time this file is opened.
    */
   if (1==f->shared->nrefs) {
      HDmemcpy (&(f->shared->file_create_parms),
		create_parms,
		sizeof(file_create_temp_t));
   }
   cp = &(f->shared->file_create_parms);

   /*
    * Read or write the file boot block.
    */
   if (empty_file) {
      /* For new files we must write the boot block. */
      f->shared->consist_flags = 0x03;
      if (H5F_flush (f, FALSE)<0) {
	 /* Can't write file boot block */
	 HGOTO_ERROR (H5E_FILE, H5E_CANTINIT, NULL);
      }
   } else if (1==f->shared->nrefs) {
      /* For existing files we must read the boot block. */
      assert (fixed_size <= sizeof buf);
      for (i=8; i<32*sizeof(haddr_t); i++) {
	 cp->userblock_size = (8==i ? 0 : 1<<i);

	 /* Read the fixed size part of the boot block */
	 if (H5F_block_read (f, 0, fixed_size, buf)<0) {
	    /*can't read boot block*/
	    HGOTO_ERROR (H5E_FILE, H5E_NOTHDF5, NULL);
	 }
	 
	 /*
	  * Decode the fixed size part of the boot block.  For each of the
	  * version parameters, check that the library is able to handle that
	  * version.
	  */
	 p = buf;
	 if (HDmemcmp (p, H5F_SIGNATURE, H5F_SIGNATURE_LEN)) continue;
	 p += H5F_SIGNATURE_LEN;

	 cp->bootblock_ver = *p++;
	 if (cp->bootblock_ver != HDF5_BOOTBLOCK_VERSION) {
	    /* Bad boot block version number */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->smallobject_ver = *p++;
	 if (cp->smallobject_ver != HDF5_SMALLOBJECT_VERSION) {
	    /* Bad small object heap version number */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->freespace_ver = *p++;
	 if (cp->freespace_ver != HDF5_FREESPACE_VERSION) {
	    /* Bad free space version number */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->objectdir_ver = *p++;
	 if (cp->objectdir_ver != HDF5_OBJECTDIR_VERSION) {
	    /* Bad object dir version number */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->sharedheader_ver = *p++;
	 if (cp->sharedheader_ver != HDF5_SHAREDHEADER_VERSION) {
	    /* Bad shared header version number */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->offset_size = *p++;
	 if (cp->offset_size!=2 &&
	     cp->offset_size!=4 &&
	     cp->offset_size!=8) {
	    /* Bad offset size */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 cp->length_size = *p++;
	 if (cp->length_size!=2 &&
	     cp->length_size!=4 &&
	     cp->length_size!=8) {
	    /* Bad length size */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 /* Reserved byte */
	 p++;

	 UINT16DECODE (p, cp->sym_leaf_k);
	 if (cp->sym_leaf_k<1) {
	    /* Bad symbol table leaf node 1/2 rank */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }
	 
	 UINT16DECODE (p, cp->btree_k[H5B_SNODE_ID]);
	 if (cp->btree_k[H5B_SNODE_ID]<1) {
	    /* Bad symbol table internal node 1/2 rank */
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }

	 UINT32DECODE (p, f->shared->consist_flags);
	 /* nothing to check for consistency flags */

	 assert (p-buf == fixed_size);

	 /* Read the variable length part of the boot block... */
	 variable_size = H5F_SIZEOF_OFFSET (f) + /*global small obj heap*/
	                 H5F_SIZEOF_OFFSET (f) + /*global free list addr*/
	                 H5F_SIZEOF_SIZE (f)   + /*logical file size*/
	                 H5G_SIZEOF_ENTRY (f);
	 assert (variable_size <= sizeof buf);
	 if (H5F_block_read (f, fixed_size, variable_size, buf)<0) {
	    /*can't read boot block*/
	    HGOTO_ERROR (H5E_FILE, H5E_NOTHDF5, NULL);
	 }
	 
	 p = buf;
	 H5F_decode_offset (f, p, f->shared->smallobj_off);
	 H5F_decode_offset (f, p, f->shared->freespace_off);
	 H5F_decode_length (f, p, f->shared->logical_len);
	 if (H5G_ent_decode (f, &p, f->shared->root_sym)<0) {
	    /*can't read root symbol entry*/
	    HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, NULL);
	 }
	 break;
      }
   }

   /* What is the current size of the file? */
#ifndef LATER
   /*
    * Remember the current position so we can reset it.  If not, the seek()
    * optimization stuff gets confused.  Eventually we should have a
    * get_filesize() method for the various file types we'll have.
    */
   {
      size_t curpos = H5F_TELL (f->shared->file_handle);
#endif
      if (H5F_SEEKEND (f->shared->file_handle)<0) {
	 /* Cannot determine file size */
	 HGOTO_ERROR (H5E_FILE, H5E_CANTINIT, NULL);
      }
      f->shared->logical_len = H5F_TELL (f->shared->file_handle);
#ifndef LATER
      H5F_SEEK (f->shared->file_handle, curpos);
   }
#endif
   

   /* Success! */
   ret_value = f;

 done:
   if (!ret_value) {
      if (f) H5F_dest (f);
      if (H5F_INVALID_FILE!=fd) H5F_CLOSE (fd);
   }
   
   FUNC_LEAVE (ret_value);
}

/*--------------------------------------------------------------------------
 NAME
    H5Fcreate

 PURPOSE
    Create a new HDF5 file.

 USAGE
    int32 H5Fcreate(filename, flags, create_temp, access_temp)
        const char *filename;   IN: Name of the file to create
        uintn flags;            IN: Flags to indicate various options.
        hid_t create_temp;    IN: File-creation template ID
        hid_t access_temp;    IN: File-access template ID

 ERRORS
    ARGS      BADVALUE      Invalid file name. 
    ARGS      BADVALUE      Invalid flags. 
    ATOM      BADATOM       Can't unatomize template. 
    ATOM      CANTREGISTER  Can't atomize file. 
    FILE      CANTOPENFILE  Can't create file. 

 RETURNS
    Returns file ID on success, FAIL on failure

 DESCRIPTION
        This is the primary function for creating HDF5 files . The flags
    parameter determines whether an existing file will be overwritten or not.
    All newly created files are opened for both reading and writing.  All flags
    may be combined with the "||" (logical OR operator) to change the behavior
    of the file open call.
        The flags currently defined:
            H5ACC_OVERWRITE - Truncate file, if it already exists. The file
 	        will be truncated, erasing all data previously stored in the
 		file.
        The more complex behaviors of a file's creation and access are
    controlled through the file-creation and file-access templates.  The value
    of 0 for a template value indicates that the library should use the default
    values for the appropriate template.  (Documented in the template module).
    [Access templates are currently unused in this routine, although they will
    be implemented in the future]

 MODIFICATIONS:
    Robb Matzke, 18 Jul 1997
    File struct creation and destruction is through H5F_new() H5F_dest().
    Writing the root symbol table entry is done with H5G_encode().

    Robb Matzke, 29 Aug 1997
    Moved creation of the boot block to H5F_flush().
 
    Robb Matzke, 23 Sep 1997
    Most of the work is now done by H5F_open() since H5Fcreate() and H5Fopen()
    originally contained almost identical code.
--------------------------------------------------------------------------*/
hid_t H5Fcreate(const char *filename, uintn flags, hid_t create_temp,
		hid_t access_temp)
{
    H5F_t	*new_file=NULL;     /* file struct for new file */
    const file_create_temp_t *create_parms; /* pointer to the parameters to
					     * use when creating the file
					     */
    hid_t	ret_value = FAIL;

    FUNC_ENTER(H5Fcreate, H5F_init_interface, FAIL);
    H5ECLEAR;

    /* Check/fix arguments */
    if (!filename || !*filename)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL); /*invalid file name*/
    if (flags & ~H5ACC_OVERWRITE)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL); /*invalid flags*/
    flags = (H5F_ACC_WRITE | H5F_ACC_CREAT) |
            (H5ACC_OVERWRITE==flags ? H5F_ACC_TRUNC : H5F_ACC_EXCL);
    if (0==create_temp)
        create_temp = H5C_get_default_atom (H5_TEMPLATE);
    if (NULL==(create_parms=H5Aatom_object(create_temp)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL); /*can't unatomize template*/
#ifdef LATER
    if (0==access_temp)
        access_temp = H5CPget_default_atom(H5_TEMPLATE);
    if (NULL==(access_parms=H5Aatom_object(access_temp)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL); /*can't unatomize template*/
#endif
    
    /*
     * Create a new file or truncate an existing file.
     */
    if (NULL==(new_file = H5F_open (filename, flags, create_parms))) {
       HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, FAIL); /*can't create file */
    }

    /* Get an atom for the file */
    if ((ret_value=H5Aregister_atom (H5_FILE, new_file))<0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL); /*can't atomize file*/

 done:
    if (ret_value<0 && new_file) {
       /* Error condition cleanup */
       H5F_close (new_file);
    }

    /* Normal function cleanup */

    FUNC_LEAVE(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5Fopen

 PURPOSE
    Open an existing HDF5 file.

 USAGE
    hid_t H5Fopen(filename, flags, access_temp)
        const char *filename;   IN: Name of the file to create
        uintn flags;            IN: Flags to indicate various options.
        hid_t access_temp;    IN: File-access template

 ERRORS
    ARGS      BADRANGE      Invalid file name. 
    ATOM      BADATOM       Can't unatomize template. 
    ATOM      CANTREGISTER  Can't atomize file. 
    FILE      CANTOPENFILE  Cant open file. 

 RETURNS
    Returns file ID on success, FAIL on failure

 DESCRIPTION
        This is the primary function for accessing existing HDF5 files. The
    flags parameter determines whether writing to an existing file will be
    allowed or not.  All flags may be combined with the "||" (logical OR
    operator) to change the behavior of the file open call.
        The flags currently defined:
            H5ACC_WRITE - Allow writing to the file.
        The more complex behaviors of a file's access are controlled through
    the file-access template.

 MODIFICATIONS:
    Robb Matzke, 18 Jul 1997
    File struct creation and destruction is through H5F_new() H5F_dest().
    Reading the root symbol table entry is done with H5G_decode().

    Robb Matzke, 23 Sep 1997
    Most of the work is now done by H5F_open() since H5Fcreate() and H5Fopen()
    originally contained almost identical code.
--------------------------------------------------------------------------*/
hid_t H5Fopen(const char *filename, uintn flags, hid_t access_temp)
{
    H5F_t	*new_file=NULL;     	/* file struct for new file */
    hid_t 	create_temp;            /* file-creation template ID */
    const file_create_temp_t *f_create_parms;    /* pointer to the parameters
						  * to use when creating the
						  * file
						  */
    hid_t 	ret_value = FAIL;

    FUNC_ENTER(H5Fopen, H5F_init_interface, FAIL);
    H5ECLEAR;

    /* Check/fix arguments. */
    if (!filename || !*filename)
        HGOTO_ERROR (H5E_ARGS, H5E_BADRANGE, FAIL);/*invalid file name*/
    flags = flags & H5ACC_WRITE ? H5F_ACC_WRITE : 0;

    create_temp = H5C_get_default_atom (H5_TEMPLATE);
    if (NULL==(f_create_parms=H5Aatom_object(create_temp)))
        HGOTO_ERROR (H5E_ATOM, H5E_BADATOM, FAIL);/*can't unatomize template*/

#ifdef LATER
    if (access_temp<=0)
        access_temp = H5CPget_default_atom (H5_TEMPLATE);
    if (NULL==(f_access_parms=H5Aatom_object (access_temp)))
        HGOTO_ERROR (H5E_ATOM, H5E_BADATOM, FAIL);/*can't unatomize template*/
#endif

    /* Open the file */
    if (NULL==(new_file=H5F_open (filename, flags, f_create_parms))) {
       HGOTO_ERROR (H5E_FILE, H5E_CANTOPENFILE, FAIL); /*cant open file*/
    }

    /* Get an atom for the file */
    if ((ret_value = H5Aregister_atom (H5_FILE, new_file))<0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL);/*can't atomize file*/

 done:
    if (ret_value<0 && new_file) {
       H5F_close (new_file);
    }

    /* Normal function cleanup */

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_flush
 *
 * Purpose:	Flushes (and optionally invalidates) cached data plus the
 *		file boot block.  If the logical file size field is zero
 *		then it is updated to be the length of the boot block.
 *
 * Errors:
 *		CACHE     CANTFLUSH     Can't flush cache. 
 *		IO        WRITEERROR    Can't write header. 
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	FAIL
 *				-2 if the there are open objects and
 * 				INVALIDATE was non-zero.
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 29 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_flush (H5F_t *f, hbool_t invalidate)
{
   uint8	buf[2048], *p=buf;
   herr_t	shadow_flush;
   
   FUNC_ENTER (H5F_flush, H5F_init_interface, FAIL);

   /*
    * Nothing to do if the file is read only.  This determination is made at
    * the shared open(2) flags level, implying that opening a file twice,
    * once for read-only and once for read-write, and then calling
    * H5F_flush() with the read-only handle, still causes data to be flushed.
    */
   if (0==(H5F_ACC_WRITE & f->shared->flags)) HRETURN (SUCCEED);

   /*
    * Flush all open object info. If this fails just remember it and return
    * failure at the end.  At least that way we get a consistent file.
    */
   shadow_flush = H5G_shadow_flush (f,  invalidate);
      
   /* flush (and invalidate) the entire cache */
   if (H5AC_flush (f, NULL, 0, invalidate)<0) {
      HRETURN_ERROR (H5E_CACHE, H5E_CANTFLUSH, FAIL); /*can't flush cache*/
   }

   /* encode the file boot block */
   HDmemcpy (p, H5F_SIGNATURE, H5F_SIGNATURE_LEN);
   p += H5F_SIGNATURE_LEN;
   
   *p++ = f->shared->file_create_parms.bootblock_ver;
   *p++ = f->shared->file_create_parms.smallobject_ver;
   *p++ = f->shared->file_create_parms.freespace_ver;
   *p++ = f->shared->file_create_parms.objectdir_ver;
   *p++ = f->shared->file_create_parms.sharedheader_ver;
   *p++ = H5F_SIZEOF_OFFSET (f);
   *p++ = H5F_SIZEOF_SIZE (f);
   *p++ = 0; /*reserved*/
   UINT16ENCODE (p, f->shared->file_create_parms.sym_leaf_k);
   UINT16ENCODE (p, f->shared->file_create_parms.btree_k[H5B_SNODE_ID]);
   UINT32ENCODE (p, f->shared->consist_flags);
   H5F_encode_offset (f, p, f->shared->smallobj_off);
   H5F_encode_offset (f, p, f->shared->freespace_off);
   H5F_encode_length (f, p, f->shared->logical_len);
   H5G_ent_encode (f, &p, f->shared->root_sym);

   /* write the boot block to disk */
   if (H5F_block_write (f, 0, p-buf, buf)<0) {
      HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL); /*can't write header*/
   }

   /* update file length if necessary */
   if (f->shared->logical_len<=0) f->shared->logical_len = p-buf;

   /* Did shadow flush fail above? */
   if (shadow_flush<0) {
      HRETURN_ERROR (H5E_CACHE, H5E_CANTFLUSH, -2);/*object are still open*/
   }
   
   FUNC_LEAVE (SUCCEED);
}

/*--------------------------------------------------------------------------
 NAME
    H5Fflush

 PURPOSE
    Flush all cached data to disk and optionally invalidates all cached
    data.

 USAGE
    herr_t H5Fclose(fid, invalidate)
        hid_t fid;      	IN: File ID of file to close.
        hbool_t invalidate;	IN: Invalidate all of the cache?

 ERRORS
    ARGS      BADTYPE       Not a file atom. 
    ATOM      BADATOM       Can't get file struct. 
    CACHE     CANTFLUSH     Flush failed. 

 RETURNS
    SUCCEED/FAIL

 DESCRIPTION
        This function flushes all cached data to disk and, if INVALIDATE
    is non-zero, removes cached objects from the cache so they must be
    re-read from the file on the next access to the object.

 MODIFICATIONS:
--------------------------------------------------------------------------*/
herr_t
H5Fflush (hid_t fid, hbool_t invalidate)
{
   H5F_t	*file = NULL;

   FUNC_ENTER (H5Fflush, H5F_init_interface, FAIL);
   H5ECLEAR;

   /* check arguments */
   if (H5_FILE!=H5Aatom_group (fid)) {
      HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL); /*not a file atom*/
   }
   if (NULL==(file=H5Aatom_object (fid))) {
      HRETURN_ERROR (H5E_ATOM, H5E_BADATOM, FAIL); /*can't get file struct*/
   }

   /* do work */
   if (H5F_flush (file, invalidate)<0) {
      HRETURN_ERROR (H5E_CACHE, H5E_CANTFLUSH, FAIL); /*flush failed*/
   }

   FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_close
 *
 * Purpose:	Closes an open HDF5 file.
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	FAIL, or -2 if the failure is due to objects
 *				still being open.
 *
 * Programmer:	Robb Matzke
 *              Tuesday, September 23, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_close (H5F_t *f)
{
   herr_t	ret_value = FAIL;

   FUNC_ENTER (H5F_close, H5F_init_interface, FAIL);
   
   if (-2==(ret_value=H5F_flush (f, TRUE))) {
      /*objects are still open, but don't fail yet*/
   } else if (ret_value<0) {
      /*can't flush cache*/
      HRETURN_ERROR (H5E_CACHE, H5E_CANTFLUSH, FAIL);
   }
   H5F_CLOSE(f->shared->file_handle);
   H5F_dest (f);

   /* Did the H5F_flush() fail because of open objects? */
   if (ret_value<0) {
      HRETURN_ERROR (H5E_SYM, H5E_CANTFLUSH, ret_value);
   }

   FUNC_LEAVE (ret_value);
}

/*--------------------------------------------------------------------------
 NAME
    H5Fclose

 PURPOSE
    Close an open HDF5 file.

 USAGE
    int32 H5Fclose(fid)
        int32 fid;      IN: File ID of file to close

 ERRORS
    ARGS      BADTYPE       Not a file atom. 
    ATOM      BADATOM       Can't remove atom. 
    ATOM      BADATOM       Can't unatomize file. 
    CACHE     CANTFLUSH     Can't flush cache. 

 RETURNS
    SUCCEED/FAIL

 DESCRIPTION
        This function terminates access to an HDF5 file.  If this is the last
    file ID open for a file and if access IDs are still in use, this function
    will fail.

 MODIFICATIONS:
    Robb Matzke, 18 Jul 1997
    File struct destruction is through H5F_dest().

    Robb Matzke, 29 Aug 1997
    The file boot block is flushed to disk since it's contents may have
    changed.
--------------------------------------------------------------------------*/
herr_t H5Fclose(hid_t fid)
{
    H5F_t	*file=NULL;         /* file struct for file to close */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER(H5Fclose, H5F_init_interface, FAIL);
    H5ECLEAR;

    /* Check/fix arguments. */
    if (H5_FILE!=H5Aatom_group (fid))
       HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL);/*not a file atom*/
    if (NULL==(file=H5Aatom_object (fid)))
       HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL);/*can't unatomize file*/

    /* Close the file */
    ret_value = H5F_close (file);
    
    /* Remove the file atom */
    if (NULL==H5Aremove_atom(fid)) {
       HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL);/*can't remove atom*/
    }

done:
    FUNC_LEAVE (ret_value<0?FAIL:SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_block_read
 *
 * Purpose:	Reads some data from a file/server/etc into a buffer.
 *		The data is contiguous.
 *
 * Errors:
 *		IO        READERROR     Low-level read failure. 
 *		IO        SEEKERROR     Low-level seek failure. 
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_read (H5F_t *f, haddr_t addr, size_t size, void *buf)
{
   FUNC_ENTER (H5F_block_read, H5F_init_interface, FAIL);

   if (0==size) return 0;
   addr += f->shared->file_create_parms.userblock_size;
   
  /* Check for switching file access operations or mis-placed seek offset */
#ifdef H5F_OPT_SEEK
  if(f->shared->last_op!=OP_READ || f->shared->f_cur_off!=addr)
    {
#endif
      f->shared->last_op=OP_READ;
      if (H5F_SEEK (f->shared->file_handle, addr)<0) {
          /* low-level seek failure */
          HRETURN_ERROR (H5E_IO, H5E_SEEKERROR, FAIL);
        } /* end if */
#ifdef H5F_OPT_SEEK
    } /* end if */
#endif
   if (H5F_READ (f->shared->file_handle, buf, size)<0) {
      /* low-level read failure */
      HRETURN_ERROR (H5E_IO, H5E_READERROR, FAIL);
   }
   f->shared->f_cur_off=addr+size;

   FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_block_write
 *
 * Purpose:	Writes some data from memory to a file/server/etc.  The
 *		data is contiguous.
 *
 * Errors:
 *		IO        SEEKERROR     Low-level seek failure. 
 *		IO        WRITEERROR    Low-level write failure. 
 *		IO        WRITEERROR    No write intent. 
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_write (H5F_t *f, haddr_t addr, size_t size, void *buf)
{
   FUNC_ENTER (H5F_block_write, H5F_init_interface, FAIL);

   if (0==size) return 0;
   addr += f->shared->file_create_parms.userblock_size;

   if (0==(f->intent & H5F_ACC_WRITE)) {
      /* no write intent */
      HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL);
   }

  /* Check for switching file access operations or mis-placed seek offset */
#ifdef H5F_OPT_SEEK
  if(f->shared->last_op!=OP_WRITE || f->shared->f_cur_off!=addr)
    {
#endif
      f->shared->last_op=OP_WRITE;
      if (H5F_SEEK (f->shared->file_handle, addr)<0) {
         /* low-level seek failure */
         HRETURN_ERROR (H5E_IO, H5E_SEEKERROR, FAIL);
      }
#ifdef H5F_OPT_SEEK
    } /* end if */
#endif

   if (H5F_WRITE (f->shared->file_handle, buf, size)<0) {
      /* low-level write failure */
      HRETURN_ERROR (H5E_IO, H5E_WRITEERROR, FAIL);
   }
   f->shared->f_cur_off=addr+size;

   FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_debug
 *
 * Purpose:	Prints a file header to the specified stream.  Each line
 *		is indented and the field name occupies the specified width
 *		number of characters.
 *
 * Errors:
 *
 * Return:	Success:	SUCCEED
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  1 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_debug (H5F_t *f, haddr_t addr, FILE *stream, intn indent, intn fwidth)
{
   FUNC_ENTER (H5F_debug, H5F_init_interface, FAIL);

   /* check args */
   assert (f);
   assert (addr>=0);
   assert (stream);
   assert (indent>=0);
   assert (fwidth>=0);

   /* debug */
   fprintf (stream, "%*sFile Boot Block...\n", indent, "");
   
   fprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
	    "File name:",
	    f->name);
   fprintf (stream, "%*s%-*s 0x%08x\n", indent, "", fwidth,
	    "Flags",
	    (unsigned)(f->shared->flags));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Reference count:",
	    (unsigned)(f->shared->nrefs));
   fprintf (stream, "%*s%-*s 0x%08lx\n", indent, "", fwidth,
	    "Consistency flags:",
	    (unsigned long)(f->shared->consist_flags));
   fprintf (stream, "%*s%-*s %ld\n", indent, "", fwidth,
	    "Small object heap address:",
	    (long)(f->shared->smallobj_off));
   fprintf (stream, "%*s%-*s %ld\n", indent, "", fwidth,
	    "Free list address:",
	    (long)(f->shared->freespace_off));
   fprintf (stream, "%*s%-*s %lu\n", indent, "", fwidth,
	    "Logical file length:",
	    (unsigned long)(f->shared->logical_len));
   fprintf (stream, "%*s%-*s %lu\n", indent, "", fwidth,
	    "Size of user block:",
	    (unsigned long)(f->shared->file_create_parms.userblock_size));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Size of file size_t type:",
	    (unsigned)(f->shared->file_create_parms.offset_size));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Size of file off_t type:",
	    (unsigned)(f->shared->file_create_parms.length_size));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Symbol table leaf node 1/2 rank:",
	    (unsigned)(f->shared->file_create_parms.sym_leaf_k));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Symbol table internal node 1/2 rank:",
	    (unsigned)(f->shared->file_create_parms.btree_k[H5B_SNODE_ID]));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Boot block version number:",
	    (unsigned)(f->shared->file_create_parms.bootblock_ver));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Small object heap version number:",
	    (unsigned)(f->shared->file_create_parms.smallobject_ver));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Free list version number:",
	    (unsigned)(f->shared->file_create_parms.freespace_ver));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Object directory version number:",
	    (unsigned)(f->shared->file_create_parms.objectdir_ver));
   fprintf (stream, "%*s%-*s %u\n", indent, "", fwidth,
	    "Shared header version number:",
	    (unsigned)(f->shared->file_create_parms.sharedheader_ver));

   fprintf (stream, "%*sRoot symbol table entry:\n", indent, "");
   H5G_ent_debug (f, f->shared->root_sym, stream, indent+3, MAX(0, fwidth-3));
	    
   FUNC_LEAVE (SUCCEED);
}
