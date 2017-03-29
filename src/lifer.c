/*********************************************************
**                                                      **
**                      lifer                           **
**                                                      **
**            A Windows link file analyser              **
**                                                      **
**         Copyright Paul Tew 2011 to 2017              **
**                                                      **
** Usage:                                               **
** lifer [-vh]                                          **
** lifer [-s] [-o csv|tsv|txt] dir|file(s)              **
**                                                      **
*********************************************************/

/*
This file is part of Lifer.

    Lifer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifer.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <wchar.h>
/* local headers */
#include "./liblife/liblife.h"
#include "./version.h"

/*Conditional includes dependant on OS*/
#ifdef _WIN32
/* Windows */
#include <io.h>
#include "./win/dirent.h"
#include "./win/getopt.h"
#else
/* *nix */
#include <unistd.h>
#include <dirent.h>
#endif

//Global stuff
enum otype {csv, tsv, txt};
enum otype output_type;
int filecount;

//Function help_message() prints a help message to stdout
void help_message()
{
  printf("*********************************************************************\
***********\n");
  printf("\nlifer - A Windows link file analyser\n");
  printf("lifer - A Windows link file analyser\n");
  printf("Version: %u.%u.%u\n", _MAJOR,_MINOR,_BUILD);
  printf("Usage: lifer  [-vhs] [-o csv|tsv|txt] file(s)|directory\
\n\n");
  printf("Options:\n  -v    print version number\n  -h    print this help\n");
  printf("  -s    shortened output (default is to output all fields)\n");
  printf("  -o    output type (choose from csv, tsv or txt). \n");
  printf("        The default is txt.\n\n");
  printf("Output is to standard output, to send to a file, use the \
redirection\n");
  printf("operator '>'.\n\n");
  printf("Example:\n  lifer -o csv *.* > Links.csv\n\n");
  printf("This will create a comma seperated file named Links.csv in the \
current\n");
  printf("directory. The file can be viewed in a spreadsheet and will \
contain details\n");
  printf("of all the link files found in the current directory.\n\n");
  printf("*********************************************************************\
***********\n");
}
//
//Function: sv_out(FILE * fp) processes the link file and outputs the csv or tsv
//          version of the decoded data.
void sv_out(FILE* fp, char* fname, int less, char sep)
{
  struct LIF   lif;
  struct LIF_A lif_a;
  struct stat statbuf;
  char buf[40];
  int  i;


  // Get the stat info for the file itself
  stat(fname, &statbuf);

  if(get_lif(fp, statbuf.st_size, &lif) < 0)
    {
      fprintf(stderr, "Error processing file \'%s\' - sorry\n", fname);
      return;
    }
  if(get_lif_a(&lif, &lif_a))
    {
      fprintf(stderr, "Could not make ASCII version of \'%s\' - sorry\n", fname);
      return;
    }
  //Now print the header if needed
  if(filecount == 0)
    {
      printf("File Name%c",sep);
      if(less == 0)
        {
          printf("Link File Size%c",sep);
        }
      printf("Link File Last Accessed%cLink File Last Modified%c",sep,sep);
      printf("Link File Last Changed%c",sep);
      if(less == 0)
        {
          printf("Hdr Size%cHdr CLSID%cHdr Flags%c",sep,sep,sep);
        }
      printf("Hdr Attributes%c", sep);
      printf("Hdr FileCreate%cHdr FileAccess%c",sep,sep);
      printf("Hdr FileWrite%c", sep);
      printf("Hdr TargetSize%c",sep);
      if(less == 0)
        {
          printf("Hdr IconIndex%cHdr WindowState%cHdr HotKeys%c",sep,sep,sep);
          printf("Hdr Reserved1%cHdr Reserved2%cHdr Reserved3%c",sep,sep,sep);
          printf("IDList Size%c",sep);
          printf("IDList No Items%c",sep);
          printf("LinkInfo Size%c", sep);
          printf("LinkInfo Hdr Size%c", sep);
          printf("LinkInfo Flags%c", sep);
          printf("LinkInfo VolID Offset%c", sep);
          printf("LinkInfo Base Path Offset%c", sep);
          printf("LinkInfo CNR Offset%c", sep);
          printf("LinkInfo CPS Offset%c", sep);
          printf("LinkInfo LBP Offset Unicode%c", sep);
          printf("LinkInfo CPS Offset Unicode%c", sep);
          printf("LinkInfo VolID Size%c", sep);
        }
      printf("LinkInfo VolID Drive Type%c", sep);
      printf("LinkInfo VolID Drive Ser No%c", sep);
      if(less == 0)
        {
          printf("LinkInfo VolID VLOffset%c", sep);
          printf("LinkInfo VolID VLOffsetU%c", sep);
        }
      printf("LinkInfo VolID Vol Label%c",sep);
      printf("LinkInfo VolID Vol LabelU%c", sep);
      printf("LinkInfo Local Base Path%c", sep);
      if(less == 0)
        {
          printf("LinkInfo CNR Size%c", sep);
          printf("LinkInfo CNR Flags%c", sep);
          printf("LinkInfo CNR NetNameOffset%c", sep);
          printf("LinkInfo CNR DeviceNameOffset%c", sep);
        }
      printf("LinkInfo CNR NetwkProviderType%c", sep);
      if(less == 0)
        {
          printf("LinkInfo CNR NetNameOffsetU%c", sep);
          printf("LinkInfo CNR DeviceNameOffsetU%c", sep);
        }
      printf("LinkInfo CNR NetName%c", sep);
      printf("LinkInfo CNR DeviceName%c", sep);
      printf("LinkInfo CNR NetNameU%c", sep);
      printf("LinkInfo CNR DeviceNameU%c", sep);

      printf("LinkInfo Common Path Suffix%c", sep);
      printf("LinkInfo Local Base Path Unicode%c", sep);
      printf("LinkInfo Common Path Suffix Unicode%c", sep);

      if(less == 0)
        {
          printf("StrData Total Size (bytes)%c", sep);
          printf("StrData Name Num Chars%c", sep);
        }
      printf("StrData - Name%c", sep);
      if(less == 0)
        {
          printf("StrData Rel Path Num Chars%c", sep);
        }
      printf("StrData Relative Path%c", sep);
      if(less == 0)
        {
          printf("StrData Working Dir Num Chars%c", sep);
        }
      printf("StrData Working Dir%c", sep);
      if(less == 0)
        {
          printf("StrData Cmd Line Args Num Chars%c", sep);
        }
      printf("StrData Cmd Line Args%c", sep);
      if(less == 0)
        {
          printf("StrData Icon Loc Num Chars%c", sep);
        }
      printf("StrData Icon Location%c", sep);
      //Extra data
      if(less == 0)
        {
          printf("ExtraData Total Size (bytes)%c", sep);
        }
      printf("ExtraData Structures%c", sep);
      if(less == 0)
        //TODO Other ExtraData structures

        //Property Store
        {
          /* Commented out because this didn't work in liblife
          //TODO Restore this

          printf("ED PS Size (bytes)%c", sep);
          printf("ED PS Signature%c", sep);
          printf("ED PS Number of Stores %c", sep);
          */
        }
      //ED Special Folder Data
      if(less == 0)
        {
          printf("ED SFolderData Size (bytes)%c", sep);
          printf("ED SFolderData Signature%c", sep);
          printf("ED SFolderData ID%c", sep);
          printf("ED SFolderData Offset%c", sep);
        }
      // ED Tracker Data
      if(less == 0)
        {
          printf("ED TrackerData Size (bytes)%c", sep);
          printf("ED TrackerData Signature%c", sep);
          printf("ED TrackerData Length%c", sep);
          printf("ED TrackerData Version%c", sep);
        }
      printf("ED TrackerData MachineID%c", sep);
      printf("ED TrackerData Droid1%c", sep);
      if(less == 0)
        {
          printf("ED TD Droid1 Version%c", sep);
          printf("ED TD Droid1 Variant%c", sep);
        }
      printf("ED TD Droid1 Time%c", sep);
      printf("ED TD Droid1 Clock Seq%c", sep);
      printf("ED TD Droid1 Node%c", sep);
      printf("ED TrackerData Droid2%c", sep);
      if(less == 0)
        {
          printf("ED TD Droid2 Version%c", sep);
          printf("ED TD Droid2 Variant%c", sep);
        }
      printf("ED TD Droid2 Time%c", sep);
      printf("ED TD Droid2 Clock Seq%c", sep);
      printf("ED TD Droid2 Node%c", sep);
      printf("ED TrackerData DroidBirth1%c", sep);
      if(less == 0)
        {
          printf("ED TD DroidBirth1 Version%c", sep);
          printf("ED TD DroidBirth1 Variant%c", sep);
        }
      printf("ED TD DroidBirth1 Time%c", sep);
      printf("ED TD DroidBirth1 Clock Seq%c", sep);
      printf("ED TD DroidBirth1 Node%c", sep);
      printf("ED TrackerData DroidBirth2%c", sep);
      if(less == 0)
        {
          printf("ED TD DroidBirth2 Version%c", sep);
          printf("ED TD DroidBirth2 Variant%c", sep);
        }
      printf("ED TD DroidBirth2 Time%c", sep);
      printf("ED TD DroidBirth2 Clock Seq%c", sep);
      printf("ED TD DroidBirth2 Node%c", sep);
      //ED Vista & above IDList
      if(less == 0)
        {
          printf("ED >= Vista IDList Size%c", sep);
          printf("ED >= Vista IDList Signature%c", sep);
          printf("ED >= Vista IDList Num Items%c", sep);
        }

      printf("\n");
    }

  //Print a record
  printf("%s%c", fname, sep);
  if(less == 0)
    {
      printf("%u%c", (unsigned int)statbuf.st_size, sep);
    }
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_atime));
  printf("%s%c", buf, sep );
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_mtime));
  printf("%s%c", buf, sep );
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_ctime));
  printf("%s%c", buf, sep );
  if(less == 0)
    {
      printf("%s%c%s%c",lif_a.lha.H_size,sep,lif_a.lha.CLSID,sep);
      printf("%s%c",lif_a.lha.Flags,sep);
    }
  printf("%s%c",lif_a.lha.Attr,sep);
  if(less == 0)
    {
      printf("%s%c",lif_a.lha.CrDate_long,sep);
      printf("%s%c%s%c",lif_a.lha.AcDate_long,sep,lif_a.lha.WtDate_long,sep);
    }
  else
    {
      printf("%s%c",lif_a.lha.CrDate,sep);
      printf("%s%c%s%c",lif_a.lha.AcDate,sep,lif_a.lha.WtDate,sep);
    }
  printf("%s%c",lif_a.lha.Size,sep);
  if(less == 0)
    {
      printf("%s%c%s%c",lif_a.lha.IconIndex,sep,lif_a.lha.ShowState,sep);
      printf("%s%c%s%c",lif_a.lha.Hotkey,sep,lif_a.lha.Reserved1,sep);
      printf("%s%c%s%c",lif_a.lha.Reserved2,sep,lif_a.lha.Reserved3,sep);
      printf("%s%c",lif_a.lidla.IDL_size,sep);
      printf("%s%c",lif_a.lidla.NumItemIDs,sep);
      printf("%s%c", lif_a.lia.Size,sep);
      printf("%s%c", lif_a.lia.HeaderSize,sep);
      printf("%s%c", lif_a.lia.Flags,sep);
      printf("%s%c", lif_a.lia.IDOffset,sep);
      printf("%s%c", lif_a.lia.LBPOffset,sep);
      printf("%s%c", lif_a.lia.CNRLOffset,sep);
      printf("%s%c", lif_a.lia.CPSOffset,sep);
      printf("%s%c", lif_a.lia.LBPOffsetU,sep);
      printf("%s%c", lif_a.lia.CPSOffsetU,sep);
      printf("%s%c", lif_a.lia.VolID.Size,sep);
    }
  printf("%s%c", lif_a.lia.VolID.DriveType,sep);
  printf("%s%c", lif_a.lia.VolID.DriveSN,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.lia.VolID.VLOffset,sep);
      printf("%s%c", lif_a.lia.VolID.VLOffsetU,sep);
    }
  printf("%s%c", lif_a.lia.VolID.VolumeLabel,sep);
  printf("%s%c", lif_a.lia.VolID.VolumeLabelU,sep);

  printf("%s%c", lif_a.lia.LBP,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.lia.CNR.Size,sep);
      printf("%s%c", lif_a.lia.CNR.Flags,sep);
      printf("%s%c", lif_a.lia.CNR.NetNameOffset,sep);
      printf("%s%c", lif_a.lia.CNR.DeviceNameOffset,sep);
    }
  printf("%s%c", lif_a.lia.CNR.NetworkProviderType,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.lia.CNR.NetNameOffsetU,sep);
      printf("%s%c", lif_a.lia.CNR.DeviceNameOffsetU,sep);
    }
  printf("%s%c", lif_a.lia.CNR.NetName,sep);
  printf("%s%c", lif_a.lia.CNR.DeviceName,sep);
  printf("%s%c", lif_a.lia.CNR.NetNameU,sep);
  printf("%s%c", lif_a.lia.CNR.DeviceNameU,sep);
  printf("%s%c", lif_a.lia.CPS,sep);
  printf("%s%c", lif_a.lia.LBPU,sep);
  printf("%s%c", lif_a.lia.CPSU,sep);

  if(less == 0)
    {
      printf("%s%c", lif_a.lsda.Size,sep);
    }
  for(i = 0; i < 5; i++)
    {
      if(less == 0)
        {
          printf("%s%c", lif_a.lsda.CountChars[i],sep);
        }
      printf("%s%c", lif_a.lsda.Data[i],sep);
    }
  //Extra data
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.Size,sep);
    }
  printf("%s%c", lif_a.leda.edtypes,sep);
  //TODO Other ED structures

  //Property Store Props
  if(less == 0)
    {
      /* Commented out because it does not work in liblife
      //TODO Restore this
      printf("%s%c", lif_a.leda.lpspa.Size,sep);
      printf("%s%c", lif_a.leda.lpspa.sig,sep);
      printf("%s%c", lif_a.leda.lpspa.NumStores,sep);
      */
    }
  //Special Folder Props
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.lsfpa.Size,sep);
      printf("%s%c", lif_a.leda.lsfpa.sig,sep);
      printf("%s%c", lif_a.leda.lsfpa.SpecialFolderID,sep);
      printf("%s%c", lif_a.leda.lsfpa.Offset,sep);
    }
  //Tracker data
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.ltpa.Size,sep);
      printf("%s%c", lif_a.leda.ltpa.sig,sep);
      printf("%s%c", lif_a.leda.ltpa.Length,sep);
      printf("%s%c", lif_a.leda.ltpa.Version,sep);
    }
  printf("%s%c", lif_a.leda.ltpa.MachineID,sep);
  printf("%s%c", lif_a.leda.ltpa.Droid1.UUID,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.ltpa.Droid1.Version,sep);
      printf("%s%c", lif_a.leda.ltpa.Droid1.Variant,sep);
      printf("%s%c", lif_a.leda.ltpa.Droid1.Time_long,sep);
    }
  else
    {
      printf("%s%c", lif_a.leda.ltpa.Droid1.Time,sep);
    }
  printf("%s%c", lif_a.leda.ltpa.Droid1.ClockSeq,sep);
  printf("%s%c", lif_a.leda.ltpa.Droid1.Node,sep);
  printf("%s%c", lif_a.leda.ltpa.Droid2.UUID,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.ltpa.Droid2.Version,sep);
      printf("%s%c", lif_a.leda.ltpa.Droid2.Variant,sep);
      printf("%s%c", lif_a.leda.ltpa.Droid2.Time_long,sep);
    }
  else
    {
      printf("%s%c", lif_a.leda.ltpa.Droid2.Time,sep);
    }
  printf("%s%c", lif_a.leda.ltpa.Droid2.ClockSeq,sep);
  printf("%s%c", lif_a.leda.ltpa.Droid2.Node,sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.UUID,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Version,sep);
      printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Variant,sep);
      printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Time_long,sep);
    }
  else
    {
      printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Time,sep);
    }
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.ClockSeq,sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Node,sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.UUID,sep);
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Version,sep);
      printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Variant,sep);
      printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Time_long,sep);
    }
  else
    {
      printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Time,sep);
    }
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.ClockSeq,sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Node,sep);
  //Vista & above IDList
  if(less == 0)
    {
      printf("%s%c", lif_a.leda.lvidlpa.Size, sep);
      printf("%s%c", lif_a.leda.lvidlpa.sig, sep);
      printf("%s%c", lif_a.leda.lvidlpa.NumItemIDs, sep);
    }

  printf("\n");

}
//
//Function: text_out(FILE * fp) processes the link file and outputs the text
//          version of the decoded data.
void text_out(FILE* fp, char* fname, int less)
{
  struct LIF     lif;
  struct LIF_A   lif_a;
  struct stat    statbuf;
  char           buf[35];

  // Get the stat info for the file itself
  stat(fname, &statbuf);

  if(get_lif(fp, statbuf.st_size, &lif) < 0)
    {
      fprintf(stderr, "Error processing file \'%s\' - sorry\n", fname);
      return;
    }
  if(get_lif_a(&lif, &lif_a))
    {
      fprintf(stderr, "Could not make ASCII version of \'%s\' - sorry\n", fname);
      return;
    }
  //Print out the results
  printf("LINK FILE -------------- %s\n", fname);
  printf("  {stat DATA}\n");
  //Print a record
  if(less == 0) //omit this stuff if short info required
    {
      printf("    File Size:           %u bytes\n", (unsigned int)statbuf.st_size);
    }
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_atime));
  printf("    Last Accessed:       %s\n", buf);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_mtime));
  printf("    Last Modified:       %s\n", buf);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_ctime));
  printf("    Last Changed:        %s\n\n", buf);

  printf("  {LINK FILE - HEADER}\n");
  if(less == 0)
    {
      printf("    Header Size:         %s bytes\n",lif_a.lha.H_size);
      printf("    Link File Class ID:  %s\n",lif_a.lha.CLSID);
      printf("    Flags:               %s\n",lif_a.lha.Flags);
    }
  printf("    Attributes:          %s\n",lif_a.lha.Attr);
  if(less == 0)
    {
      printf("    Creation Time:       %s\n",lif_a.lha.CrDate_long);
      printf("    Access Time:         %s\n",lif_a.lha.AcDate_long);
      printf("    Write Time:          %s\n",lif_a.lha.WtDate_long);
    }
  else
    {
      printf("    Creation Time:       %s\n",lif_a.lha.CrDate);
      printf("    Access Time:         %s\n",lif_a.lha.AcDate);
      printf("    Write Time:          %s\n",lif_a.lha.WtDate);
    }
  printf("    Target Size:         %s bytes\n",lif_a.lha.Size);
  if(less == 0) //omit this stuff if short info required
    {
      printf("    Icon Index:          %s\n",lif_a.lha.IconIndex);
      printf("    Window State:        %s\n",lif_a.lha.ShowState);
      printf("    Hot Keys:            %s\n",lif_a.lha.Hotkey);
      printf("    Reserved1:           %s\n",lif_a.lha.Reserved1);
      printf("    Reserved2:           %s\n",lif_a.lha.Reserved2);
      printf("    Reserved3:           %s\n",lif_a.lha.Reserved3);
    }
  if(lif.lh.Flags & 0x00000001) //If there is an ItemIDList
    {
      if(less == 0)
        {
          printf("  {LINK FILE - TARGET ID LIST}\n");
          printf("    IDList Size:         %s bytes\n",
                 lif_a.lidla.IDL_size);
          printf("    Number of Items:     %s\n",lif_a.lidla.NumItemIDs);
        }
    }
  if(lif.lh.Flags & 0x00000002) //If there is a LinkInfo
    {
      printf("  {LINK FILE - LINK INFO}\n");
      if(less == 0)
        {
          printf("    Total Size:          %s bytes\n", lif_a.lia.Size);
          printf("    Header Size:         %s bytes\n", lif_a.lia.HeaderSize);
          printf("    Flags:               %s\n", lif_a.lia.Flags);
          printf("    Volume ID Offset:    %s\n", lif_a.lia.IDOffset);
          printf("    Base Path Offset:    %s\n", lif_a.lia.LBPOffset);
          printf("    CNR Link Offset:     %s\n", lif_a.lia.CNRLOffset);
          printf("    CPS Offset:          %s\n", lif_a.lia.CPSOffset);
          printf("    LBP Offset Unicode:  %s\n", lif_a.lia.LBPOffsetU);
          printf("    CPS Offset Unicode:  %s\n", lif_a.lia.CPSOffsetU);
        }
      //There is a Volume ID structure (& LBP)
      if(lif.li.Flags & 0x00000001)
        {
          printf("    {LINK INFO - VOLUME ID}\n");
          if(less == 0)
            {
              printf("      Vol ID Size:       %s bytes\n", lif_a.lia.VolID.Size);
            }
          printf("      Drive Type:        %s\n", lif_a.lia.VolID.DriveType);
          printf("      Drive Serial No:   %s\n", lif_a.lia.VolID.DriveSN);
          if(less == 0)
            {
              if(!(lif.li.HeaderSize >= 0x00000024))//Which to use?
                //ANSI or Unicode versions
                {
                  printf("      Vol Label Offset:  %s\n", lif_a.lia.VolID.VLOffset);
                }
              else
                {
                  printf("      Vol Label OffsetU: %s\n", lif_a.lia.VolID.VLOffsetU);
                }
            }
          if(!(lif.li.HeaderSize >= 0x00000024))
            {
              printf("      Volume Label:      %s\n", lif_a.lia.VolID.VolumeLabel);
            }
          else
            {
              printf("      Volume LabelU:     %s\n", lif_a.lia.VolID.VolumeLabelU);
            }
          printf("    Local Base Path:     %s\n", lif_a.lia.LBP);
        }//End of VolumeID
      //CommonNetworkRelativeLink
      if(lif.li.Flags & 0x00000002)
        {
          printf("    {LINK INFO - COMMON NETWORK RELATIVE LINK}\n");
          if(less == 0)
            {
              printf("      CNR Size:          %s\n", lif_a.lia.CNR.Size);
              printf("      Flags:             %s\n", lif_a.lia.CNR.Flags);
              printf("      Net Name Offset:   %s\n", lif_a.lia.CNR.NetNameOffset);
              printf("      Device Name Off:   %s\n", lif_a.lia.CNR.DeviceNameOffset);
            }
          printf("      Net Provider Type: %s\n",lif_a.lia.CNR.NetworkProviderType);
          if((less == 0) && (lif.li.CNR.NetNameOffset > 0x00000014))
            {
              printf("      Net Name Offset U: %s\n", lif_a.lia.CNR.NetNameOffsetU);
              printf("      Device Name Off U: %s\n",lif_a.lia.CNR.DeviceNameOffsetU);
            }
          printf("      Net Name:          %s\n", lif_a.lia.CNR.NetName);
          printf("      Device Name:       %s\n", lif_a.lia.CNR.DeviceName);
          if(lif.li.CNR.NetNameOffset > 0x00000014)
            {
              printf("      Net Name Unicode:  %s\n", lif_a.lia.CNR.NetNameU);
              printf("      Device Name Uni:   %s\n", lif_a.lia.CNR.DeviceNameU);
            }
          printf("    Common Path Suffix:  %s\n", lif_a.lia.CPS);
        }//End of CNR
      if(lif.li.LBPOffsetU > 0)
        {
          printf("    Local Base Path Uni: %s\n", lif_a.lia.LBPU);
        }
      if(lif.li.CPSOffsetU > 0)
        {
          printf("    Local Base Path Uni: %s\n", lif_a.lia.CPSU);
        }
    }//End of Link Info
  //STRINGDATA
  if(lif.lh.Flags & 0x0000007C)
    {
      printf("  {LINK FILE - STRING DATA}\n");
      if(less == 0)
        {
          printf("    StringData Size:     %s bytes\n", lif_a.lsda.Size);
        }
      if(lif.lh.Flags & 0x00000004)
        {
          printf("    {STRING DATA - NAME STRING}\n");
          if(less == 0)
            {
              printf("      CountCharacters:   %s characters\n",
                     lif_a.lsda.CountChars[0]);
            }
          printf("      Name String:       %s\n", lif_a.lsda.Data[0]);
        }
      if(lif.lh.Flags & 0x00000008)
        {
          printf("    {STRING DATA - RELATIVE PATH}\n");
          if(less == 0)
            {
              printf("      CountCharacters:   %s characters\n",
                     lif_a.lsda.CountChars[1]);
            }
          printf("      Relative Path:     %s\n", lif_a.lsda.Data[1]);
        }
      if(lif.lh.Flags & 0x00000010)
        {
          printf("    {STRING DATA - WORKING DIR}\n");
          if(less == 0)
            {
              printf("      CountCharacters:   %s characters\n",
                     lif_a.lsda.CountChars[2]);
            }
          printf("      Working Dir:       %s\n", lif_a.lsda.Data[2]);
        }
      if(lif.lh.Flags & 0x00000020)
        {
          printf("    {STRING DATA - CMD LINE ARGS}\n");
          if(less == 0)
            {
              printf("      CountCharacters:   %s characters\n",
                     lif_a.lsda.CountChars[3]);
            }
          printf("      Cmd Line Args:     %s\n", lif_a.lsda.Data[3]);
        }
      if(lif.lh.Flags & 0x00000040)
        {
          printf("    {STRING DATA - ICON LOCATION}\n");
          if(less == 0)
            {
              printf("      CountCharacters:   %s characters\n",
                     lif_a.lsda.CountChars[4]);
            }
          printf("      Icon Location:     %s\n", lif_a.lsda.Data[4]);
        }

    }// End of STRINGDATA

  //EXTRADATA
  printf("  {LINK FILE - EXTRA DATA}\n");
  if(less == 0)
    {
      printf("    Extra Data Size:     %s bytes\n", lif_a.leda.Size);
      //debug
      printf("    ED Structures:       %s\n", lif_a.leda.edtypes);
    }
  //TODO other ED structures here
  if(lif.led.edtypes & PROPERTY_STORE_PROPS)
    {
      printf("    {EXTRA DATA - PROPERTY STORE}\n");
      if(less == 0)
        {
          printf("      BlockSize:         %s bytes\n", lif_a.leda.lpspa.Size);
          printf("      BlockSignature:    %s\n", lif_a.leda.lpspa.sig);
          // TODO Restore this: printf("      Number of Stores:  %s\n", lif_a.leda.lpspa.NumStores);
        }
    }
  if(lif.led.edtypes & SPECIAL_FOLDER_PROPS)
    {
      printf("    {EXTRA DATA - SPECIAL FOLDER DATA}\n");
      if(less == 0)
        {
          printf("      BlockSize:         %s bytes\n", lif_a.leda.lsfpa.Size);
          printf("      BlockSignature:    %s\n", lif_a.leda.lsfpa.sig);
          printf("      Folder ID:         %s\n",
                 lif_a.leda.lsfpa.SpecialFolderID);
          printf("      Offset:            %s\n", lif_a.leda.lsfpa.Offset);
        }
    }
  if(lif.led.edtypes & TRACKER_PROPS)
    {
      printf("    {EXTRA DATA - TRACKER DATA}\n");
      if(less == 0)
        {
          printf("      BlockSize:         %s bytes\n", lif_a.leda.ltpa.Size);
          printf("      BlockSignature:    %s\n", lif_a.leda.ltpa.sig);
          printf("      Length:            %s bytes\n", lif_a.leda.ltpa.Length);
          printf("      Version:           %s\n", lif_a.leda.ltpa.Version);
        }
      printf("      MachineID:         %s\n", lif_a.leda.ltpa.MachineID);
      printf("      Droid1:            %s\n", lif_a.leda.ltpa.Droid1.UUID);
      if(less == 0)
        {
          printf("        UUID Version:      %s\n", lif_a.leda.ltpa.Droid1.Version);
          printf("        UUID Variant:      %s\n", lif_a.leda.ltpa.Droid1.Variant);
        }
      if((lif_a.leda.ltpa.Droid1.Version[0] == '1')
          & (lif_a.leda.ltpa.Droid1.Version[1] == ' '))
        {
          printf("        UUID Sequence:     %s\n",
                 lif_a.leda.ltpa.Droid1.ClockSeq);
          if(less == 0)
            {
              printf("        UUID Time:         %s\n",
                     lif_a.leda.ltpa.Droid1.Time_long);
            }
          else
            {
              printf("        UUID Time:         %s\n", lif_a.leda.ltpa.Droid1.Time);
            }
          printf("        UUID Node (MAC):   %s\n",
                 lif_a.leda.ltpa.Droid1.Node);
        }
      printf("      Droid2:            %s\n", lif_a.leda.ltpa.Droid2.UUID);
      if(less == 0)
        {
          printf("        UUID Version:      %s\n", lif_a.leda.ltpa.Droid2.Version);
          printf("        UUID Variant:      %s\n", lif_a.leda.ltpa.Droid2.Variant);
        }
      if((lif_a.leda.ltpa.Droid2.Version[0] == '1')
          & (lif_a.leda.ltpa.Droid2.Version[1] == ' '))
        {
          printf("        UUID Sequence:     %s\n",
                 lif_a.leda.ltpa.Droid2.ClockSeq);
          if(less == 0)
            {
              printf("        UUID Time:         %s\n",
                     lif_a.leda.ltpa.Droid2.Time_long);
            }
          else
            {
              printf("        UUID Time:         %s\n", lif_a.leda.ltpa.Droid2.Time);
            }
          printf("        UUID Node (MAC):   %s\n",
                 lif_a.leda.ltpa.Droid2.Node);
        }
      //Rather a simplistic test to see if the two sets of Droids are the same
      if(!((lif.led.ltp.Droid1.Data1 == lif.led.ltp.DroidBirth1.Data1)
           & (lif.led.ltp.Droid2.Data1 == lif.led.ltp.DroidBirth2.Data1)
           & (less != 0)))
        {
          printf("      DroidBirth1:       %s\n",
                 lif_a.leda.ltpa.DroidBirth1.UUID);
          if(less == 0)
            {
              printf("        UUID Version:      %s\n",
                     lif_a.leda.ltpa.DroidBirth1.Version);
              printf("        UUID Variant:      %s\n",
                     lif_a.leda.ltpa.DroidBirth1.Variant);
            }
          if((lif_a.leda.ltpa.DroidBirth1.Version[0] == '1')
              & (lif_a.leda.ltpa.DroidBirth1.Version[1] == ' '))
            {
              printf("        UUID Sequence:     %s\n",
                     lif_a.leda.ltpa.DroidBirth1.ClockSeq);
              if(less == 0)
                {
                  printf("        UUID Time:         %s\n",
                         lif_a.leda.ltpa.DroidBirth1.Time_long);
                }
              else
                {
                  printf("        UUID Time:         %s\n",
                         lif_a.leda.ltpa.DroidBirth1.Time);
                }
              printf("        UUID Node (MAC):   %s\n",
                     lif_a.leda.ltpa.DroidBirth1.Node);
            }
          printf("      DroidBirth2:       %s\n",
                 lif_a.leda.ltpa.DroidBirth2.UUID);
          if(less == 0)
            {
              printf("        UUID Version:      %s\n",
                     lif_a.leda.ltpa.DroidBirth2.Version);
              printf("        UUID Variant:      %s\n",
                     lif_a.leda.ltpa.DroidBirth2.Variant);
            }
          if((lif_a.leda.ltpa.DroidBirth2.Version[0] == '1')
              & (lif_a.leda.ltpa.DroidBirth2.Version[1] == ' '))
            {
              printf("        UUID Sequence:     %s\n",
                     lif_a.leda.ltpa.DroidBirth2.ClockSeq);
              if(less == 0)
                {
                  printf("        UUID Time:         %s\n",
                         lif_a.leda.ltpa.DroidBirth2.Time_long);
                }
              else
                {
                  printf("        UUID Time:         %s\n",
                         lif_a.leda.ltpa.DroidBirth2.Time);
                }
              printf("        UUID Node (MAC):   %s\n",
                     lif_a.leda.ltpa.DroidBirth2.Node);
            }
        }
    }
  if(lif.led.edtypes & VISTA_AND_ABOVE_IDLIST_PROPS)
    {
      if(less == 0)
        {
          printf("    {EXTRA DATA - VISTA & ABOVE IDLIST}\n");
          printf("      BlockSize:         %s bytes\n", lif_a.leda.lvidlpa.Size);
          printf("      BlockSignature:    %s\n", lif_a.leda.lvidlpa.sig);
          printf("      Number of Items:     %s\n",lif_a.leda.lvidlpa. NumItemIDs);
        }
    }

  printf("\n");

}
//
//Function: proc_file() processes regular files
void proc_file(char* fname, int less)
{
  FILE *fp;
  struct stat statbuf;

  //Try to open a file pointer
  if((fp = fopen(fname, "rb")) == NULL)
    {
      //unsuccessful
      perror("Error in function proc_file()");
      fprintf(stderr, "whilst processing file: \'%s\'\n", fname);
    }
  else
    {
      stat(fname, &statbuf);
      if(statbuf.st_size >= 76) //Don't bother with files that aren't big enough
        {
          //successful
          if(test_link(fp) == 0)
            {
              switch(output_type)
                {
                case csv:
                  sv_out(fp, fname, less, ',');
                  break;
                case tsv:
                  sv_out(fp, fname, less, '\t');
                  break;
                case txt:
                default:       //Anything other than these 3 options should have been
                  //trapped already - this is just belt & braces!
                  text_out(fp, fname, less);
                }
              filecount++;
            }
          else
            {
              fprintf(stderr, "Not a Link File:\t%s\n", fname);
            }

          if(fclose(fp) != 0)
            {
              //Can't close the file for some reason
              perror("Error in function proc_file()");
              fprintf(stderr, "whilst closing file: \'%s\'\n", fname);
              exit(EXIT_FAILURE);
            }
        }
      else
        {
          fprintf(stderr, "Not a Link File:\t%s\n", fname);
        }
    }
}
//
//Function: read_dir() iterates through the files in a directory and processes
//them
void read_dir(char* dirname, int less)
{
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;
  char olddir[512], *returnbuf;

  if((dp = opendir(dirname)) == NULL)
    {
      perror("Error in function read_dir()");
      fprintf(stderr, "whilst processing directory: \'%s\'\n", dirname);
      return;
    }
  //Preserve the old directory and move to the new one
  returnbuf = getcwd(olddir, strlen(olddir));
  if(returnbuf == NULL)
    fprintf(stderr,"Could not get current directory name");
  if(!chdir(dirname))
    fprintf(stderr,"Unable to preserve old directory name");
  //Iterate through the directory entries
  while((entry = readdir(dp)) != NULL)
    {
      stat(entry->d_name, &statbuf);
      //Don't want anything but regular files
      if((statbuf.st_mode & S_IFMT) == S_IFREG)
        {
          proc_file(entry->d_name, less);
        }
    }//End of iterating through directory entries
  //Restore the old directory
  if(!chdir(olddir))
    fprintf(stderr,"Unable to restore old directory");
}
//
//Main function
int main(int argc, char *argv[])
{
  int opt, process=1, less=0; //less is the flag for short info
  //(can't use short, it's a keyword)
  int proc_dir = 0;          //A flag to deal with processing just one directory
  struct stat statbuffer;    //file details buffer

  output_type = txt;      //default output type
  filecount = 0;

  //if someone calls lifer with no options whatsoever then print help
  if(argc == 1)
    {
      help_message();
      process = 0;
    }

  //Parse the options
  while((opt = getopt(argc, argv, "vhso:")) != -1)
    {
      switch(opt)
        {
        case 'v':
          printf("lifer - A Windows link file analyser\n");
          printf("Version: %u.%u.%u\n", _MAJOR,_MINOR,_BUILD);
          process = 0; // Turn off any further processing
          break;
        case 'h':
          help_message();
          process = 0;
          break;
        case '?':
          printf("Usage: lifer [-vhs] [-o csv|tsv|txt] file(s)|\
directory\n");
          process = 0;
          break;
        case 's':
          less++;
          break;
        case 'o':
          if(strcmp(optarg, "csv") == 0)
            {
              output_type = csv;
            }
          else if(strcmp(optarg, "tsv") == 0)
            {
              output_type = tsv;
            }
          else if(strcmp(optarg, "txt") == 0)
            {
              output_type = txt;
            }
          else
            {
              printf("Invalid argument to option \'-o\'\n");
              printf("Valid arguments are: \'csv\', \'tsv\', or \'txt\'\
           [default]\n");
              process = 0;
            }
          break;
        default:
          help_message();
        }
    }

  if(process)
    {
      //Deal with the situation where valid options have been supplied
      //but no files or directory argument.
      if (optind >= argc)
        {
          fprintf(stderr, "No file(s) or directory supplied.\n");
          help_message();
          exit(EXIT_FAILURE);
        }
      for(; optind < argc; optind++ )
        {
          if(stat(argv[optind], &statbuffer) != 0)
            {
              //Getting file stats failed so report the error
              perror("Error in function main()");
              fprintf(stderr, "whilst processing argument: \'%s\'\n", argv[optind]);
            }
          //Process directory
          if(((statbuffer.st_mode & S_IFMT) == S_IFDIR) & (proc_dir == 0))
            {
              //Deal with the erroneous situation where the user provides a directory
              //argument followed by other arguments
              if(argc > (optind+1))
                {
                  fprintf(stderr, "Sorry, only one directory argument allowed\n");
                  help_message();
                  exit(EXIT_FAILURE);
                }
              else
                {
                  read_dir(argv[optind], less);
                }
            }
          //Process regular files
          else if(((statbuffer.st_mode & S_IFMT) == S_IFREG))
            {
              proc_file(argv[optind], less);
            }
          proc_dir = 1; //Prevent processing of directories after first argument
          //(The default behaviour is to process 1 directory OR
          //several files)
        }
    }
  exit(EXIT_SUCCESS);
}
