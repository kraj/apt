// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: cdromutl.cc,v 1.3 1999/04/03 01:05:24 jgg Exp $
/* ######################################################################
   
   CDROM Utilities - Some functions to manipulate CDROM mounts.
   
   These are here for the cdrom method and apt-cdrom.
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/cdromutl.h"
#endif
#include <apt-pkg/cdromutl.h>
#include <apt-pkg/error.h>
#include <apt-pkg/md5.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/configuration.h>

#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
									/*}}}*/

// IsMounted - Returns true if the mount point is mounted		/*{{{*/
// ---------------------------------------------------------------------
/* This is a simple algorithm that should always work, we stat the mount point
   and the '..' file in the mount point and see if they are on the same device.
   By definition if they are the same then it is not mounted. This should 
   account for symlinked mount points as well. */
bool IsMounted(string &Path)
{
   if (Path.empty() == true)
      return false;
   
   // Need that trailing slash for directories
   if (Path[Path.length() - 1] != '/')
      Path += '/';
   
   /* First we check if the path is actualy mounted, we do this by
      stating the path and the previous directory (carefull of links!)
      and comparing their device fields. */
   struct stat Buf,Buf2;
   if (stat(Path.c_str(),&Buf) != 0 || 
       stat((Path + "../").c_str(),&Buf2) != 0)
      return _error->Errno("stat","Unable to stat the mount point %s",Path.c_str());

   if (Buf.st_dev == Buf2.st_dev)
      return false;
   return true;
}
									/*}}}*/
// UnmountCdrom - Unmount a cdrom					/*{{{*/
// ---------------------------------------------------------------------
/* Forking umount works much better than the umount syscall which can 
   leave /etc/mtab inconsitant. We drop all messages this produces. */
bool UnmountCdrom(string Path)
{
   if (IsMounted(Path) == false)
      return true;
   
   int Child = fork();
   if (Child < -1)
      return _error->Errno("fork","Failed to fork");

   // The child
   if (Child == 0)
   {
      // Make all the fds /dev/null
      for (int I = 0; I != 10; I++)
	 close(I);
      for (int I = 0; I != 3; I++)
	 dup2(open("/dev/null",O_RDWR),I);

      if (_config->Exists("Acquire::cdrom::"+Path+"::UMount") == true)
      {
	 if (system(_config->Find("Acquire::cdrom::"+Path+"::UMount").c_str()) != 0)
	    _exit(100);
	 _exit(0);	 	 
      }
      else
      {
	 const char *Args[10];
	 Args[0] = "umount";
	 Args[1] = Path.c_str();
	 Args[2] = 0;
	 execvp(Args[0],(char **)Args);      
	 _exit(100);
      }      
   }

   // Wait for mount
   int Status = 0;
   while (waitpid(Child,&Status,0) != Child)
   {
      if (errno == EINTR)
	 continue;
      return _error->Errno("waitpid","Couldn't wait for subprocess");
   }
   
   // Check for an error code.
   if (WIFEXITED(Status) == 0 || WEXITSTATUS(Status) != 0)
      return false;
   return true;
}
									/*}}}*/
// MountCdrom - Mount a cdrom						/*{{{*/
// ---------------------------------------------------------------------
/* We fork mount and drop all messages */
bool MountCdrom(string Path)
{
   if (IsMounted(Path) == true)
      return true;
   
   int Child = fork();
   if (Child < -1)
      return _error->Errno("fork","Failed to fork");

   // The child
   if (Child == 0)
   {
      // Make all the fds /dev/null
      for (int I = 0; I != 10; I++)
	 close(I);      
      for (int I = 0; I != 3; I++)
	 dup2(open("/dev/null",O_RDWR),I);
      
      if (_config->Exists("Acquire::cdrom::"+Path+"::Mount") == true)
      {
	 if (system(_config->Find("Acquire::cdrom::"+Path+"::Mount").c_str()) != 0)
	    _exit(100);
	 _exit(0);	 
      }
      else
      {
	 const char *Args[10];
	 Args[0] = "mount";
	 Args[1] = Path.c_str();
	 Args[2] = 0;
	 execvp(Args[0],(char **)Args);      
	 _exit(100);
      }      
   }

   // Wait for mount
   int Status = 0;
   while (waitpid(Child,&Status,0) != Child)
   {
      if (errno == EINTR)
	 continue;
      return _error->Errno("waitpid","Couldn't wait for subprocess");
   }
   
   // Check for an error code.
   if (WIFEXITED(Status) == 0 || WEXITSTATUS(Status) != 0)
      return false;
   return true;
}
									/*}}}*/
// IdentCdrom - Generate a unique string for this CD			/*{{{*/
// ---------------------------------------------------------------------
/* We convert everything we hash into a string, this prevents byte size/order
   from effecting the outcome. */
bool IdentCdrom(string CD,string &Res)
{
   MD5Summation Hash;

   string StartDir = SafeGetCWD();
   if (chdir(CD.c_str()) != 0)
      return _error->Errno("chdir","Unable to change to %s",CD.c_str());
   
   DIR *D = opendir(".");
   if (D == 0)
      return _error->Errno("opendir","Unable to read %s",CD.c_str());
      
   /* Run over the directory, we assume that the reader order will never
      change as the media is read-only. In theory if the kernel did
      some sort of wacked caching this might not be true.. */
   char S[300];
   for (struct dirent *Dir = readdir(D); Dir != 0; Dir = readdir(D))
   {
      // Skip some files..
      if (strcmp(Dir->d_name,".") == 0 ||
	  strcmp(Dir->d_name,"..") == 0)
	 continue;
   
      sprintf(S,"%lu",Dir->d_ino);
      Hash.Add(S);
      Hash.Add(Dir->d_name);
   };
   
   chdir(StartDir.c_str());
   closedir(D);
   
   // Some stats from the fsys
   struct statfs Buf;
   if (statfs(CD.c_str(),&Buf) != 0)
      return _error->Errno("statfs","Failed to stat the cdrom");

   // We use a kilobyte block size to advoid overflow
   sprintf(S,"%u %u",Buf.f_blocks*(Buf.f_bsize/1024),
	   Buf.f_bfree*(Buf.f_bsize/1024));
   Hash.Add(S);
   
   Res = Hash.Result().Value();
   return true;   
}
									/*}}}*/
