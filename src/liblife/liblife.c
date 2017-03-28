/*********************************************************
**                                                      **
**                 liblife.c                            **
**                                                      **
** A library to handle the data from Windows link files **
**                                                      **
**         Copyright Paul Tew 2011 to 2012              **
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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./liblife.h"

//Declaration of functions used privately
int get_lhdr(FILE *, struct LIF *);
int get_lhdr_a(struct LIF_HDR *, struct LIF_HDR_A *);
int get_idlist(FILE *, int, int, struct LIF *);
int get_idlist_a(struct LIF_IDLIST *, struct LIF_IDLIST_A *);
int get_linkinfo(FILE *, int, int, struct LIF *);
int get_linkinfo_a(struct LIF_INFO *, struct LIF_INFO_A *);
int get_stringdata(FILE *, int, struct LIF *);
int get_stringdata_a(struct LIF_STRINGDATA *, struct LIF_STRINGDATA_A *);
int get_extradata(FILE *, int, struct LIF *);
int get_extradata_a(struct LIF_EXTRA_DATA *, struct LIF_EXTRA_DATA_A *);
void get_flag_a(unsigned char [], struct LIF_HDR *);
void get_attr_a(unsigned char [], struct LIF_HDR *);
uint64_t get_le_uint64(unsigned char [], int);
uint32_t  get_le_uint32(unsigned char [], int);
uint16_t get_le_uint16(unsigned char [], int);
int32_t get_le_int32(unsigned char [], int);
void get_chars(unsigned char [], int, int, unsigned char []);
int get_le_unistr(unsigned char [], int, int, wchar_t[]);
void get_filetime_a_short(int64_t, unsigned char []);
void get_filetime_a_long(int64_t, unsigned char[]);
void get_ltp(struct LIF_TRACKER_PROPS *, unsigned char[]);
void get_droid_a(struct LIF_CLSID *, struct LIF_CLSID_A *);
void led_setnull(struct LIF_EXTRA_DATA *);


//Function get_lif(FILE* fp, int size, struct LIF lif) takes an open file
//pointer and populates the LIF with relevant data.
extern int get_lif(FILE* fp, int size, struct LIF* lif)
{
  int pos = 0;

  assert(size >= 0x4C);   //Min size for a LIF (must contain a header at least)
  if(get_lhdr(fp, lif) < 0)
    {
      return -1;
    }
  pos += 0x4C;

  if(get_idlist(fp, size, pos, lif) < 0)
    {
      return -2;
    }
  pos += (lif->lidl.IDL_size);

  if(get_linkinfo(fp, size, pos, lif) < 0)
    {
      return -3;
    }
  pos += (lif->li.Size);

  if(get_stringdata(fp, pos, lif) < 0)
    {
      return -4;
    }
  pos += (lif->lsd.Size);

  if(pos < size) //Only get the extra data if it exists
    {
      if(get_extradata(fp, pos, lif) < 0)
        {
          return -5;
        }
      pos += (lif->led.Size);
    }
  else //If it does not exist then set it to null
    {
      led_setnull(&lif->led);
    }

  return 0;
}
//
//Function get_lif_a(struct LIF* lif, struct LIF_A* lif_a) populates the LIF_A
//structure with the ASCII representation of a LIF
extern int get_lif_a(struct LIF* lif, struct LIF_A* lif_a)
{
  if(get_lhdr_a(&lif->lh, &lif_a->lha) < 0)
    {
      return -1;
    }
  if(get_idlist_a(&lif->lidl, &lif_a->lidla) < 0)
    {
      return -2;
    }
  if(get_linkinfo_a(&lif->li, &lif_a->lia) < 0)
    {
      return -3;
    }
  if(get_stringdata_a(&lif->lsd, &lif_a->lsda) < 0)
    {
      return -4;
    }
  if(get_extradata_a(&lif->led, &lif_a->leda) < 0)
    {
      return -5;
    }

  //TODO: get other parts of the LIF_A here
  return 0;
}
//
//Function test_link(FILE *fp) takes an open file pointer as an argument
//and returns 0 if the file IS a Windows link file or -1 if not.
extern int test_link(FILE* fp)
{
  struct LIF lif;
  int i;

  assert(fp >= 0); //Ensure we have a live file pointer - this kills execution
  if(fp < 0)
    {
      return -1; //Same as the previous but won't kill execution if NDEBUG defined
    }

  get_lhdr(fp, &lif);
  //Check the value of HeaderSize
  if(lif.lh.H_size != 0x0000004C)
    return -1;
  //Check the CLSID
  if(lif.lh.CLSID.Data1 != 0x00021401)
    return -2;
  if(lif.lh.CLSID.Data2 != 0x0000)
    return -3;
  if(lif.lh.CLSID.Data3 != 0x0000)
    return -4;
  if(!((lif.lh.CLSID.Data4hi[0] == 0xC0) && (lif.lh.CLSID.Data4hi[1] == 0)))
    return -5;
  for(i = 0; i < 5; i++)
    {
      if(lif.lh.CLSID.Data4lo[i] != 0)
        {
          return -6;
        }
    }
  if(lif.lh.CLSID.Data4lo[5] != 0x46)
    return -7;
  //Now check that the reserved data areas are 0 (as specified in MS-SHLLINK)
  if(lif.lh.Reserved1 != 0x0000)
    return -8;
  if(lif.lh.Reserved2 != 0x00000000)
    return -9;
  if(lif.lh.Reserved3 != 0x00000000)
    return -10;
  return 0;
}
//
//Function get_lhdr(FILE *fp, struct LIF_HDR *lh) takes an open file pointer
//and a pointer to a LIF_HDR structure.
//On exit the LIF_HDR will be populated.
int get_lhdr(FILE *fp, struct LIF *lif)
{
  unsigned char header[0x4C];
  int chr;
  int i;


  assert(fp >= 0); //Ensure we have a live file pointer - this kills execution
  if(fp < 0)
    {
      return -1; //Same as the previous but won't kill execution if NDEBUG defined
    }

  rewind(fp);
  //I'd love to use 'read()' here but I'm trying to avoid using unistd.h
  //because I want the library to compile under Windoze
  for(i = 0; i < 0x4C; i++)
    {
      chr = getc(fp);
      if(chr != EOF)
        {
          header[i] = (unsigned char) chr;
        }
      else
        {
          perror("Error in function get_lhdr()");
        }
    }
  lif->lh.H_size = get_le_uint32(header, 0);
  lif->lh.CLSID.Data1 = get_le_uint32(header, 4);
  lif->lh.CLSID.Data2 = get_le_uint16(header, 8);
  lif->lh.CLSID.Data3 = get_le_uint16(header, 10);
  get_chars(header, 12, 2, lif->lh.CLSID.Data4hi);
  get_chars(header, 14, 6, lif->lh.CLSID.Data4lo);
  lif->lh.Flags = get_le_uint32(header, 20);
  lif->lh.Attr = get_le_uint32(header, 24);
  lif->lh.CrDate = get_le_uint64(header, 28);
  lif->lh.AcDate = get_le_uint64(header, 36);
  lif->lh.WtDate = get_le_uint64(header, 44);
  lif->lh.Size = get_le_uint32(header, 52);
  lif->lh.IconIndex = get_le_int32(header, 56);
  lif->lh.ShowState = get_le_uint32(header, 60);
  lif->lh.Hotkey.LowKey = header[64];
  lif->lh.Hotkey.HighKey = header[65];
  lif->lh.Reserved1 = get_le_uint16(header, 66);
  lif->lh.Reserved2 = get_le_uint32(header, 68);
  lif->lh.Reserved3 = get_le_uint32(header, 72);
  return 0;
}
//
//Function get_lhdr_a(LIF_HDR*, LIF_HDR_A*) converts the data in a LIF_HDR
//into a readable form and populates the strings in LIF_HDR_A
int get_lhdr_a(struct LIF_HDR* lh, struct LIF_HDR_A* lha)
{
  unsigned char lk[20], hk1[10], hk2[10], hk3[10], attr_str[115], flag_str[420];

  snprintf((char *)lha->H_size, 10, "%"PRIu32, lh->H_size);
  snprintf((char *)lha->CLSID, 40, "{00021401-0000-0000-C000-000000000046}");
  get_flag_a(flag_str, lh);
  snprintf((char *)lha->Flags, 550, "0x%.8"PRIX32"  %s", lh->Flags, flag_str);
  get_attr_a(attr_str, lh);
  snprintf((char *)lha->Attr, 250, "0x%.8"PRIX32"  %s", lh->Attr, attr_str);
  get_filetime_a_short(lh->CrDate, lha->CrDate);
  get_filetime_a_long(lh->CrDate, lha->CrDate_long);
  get_filetime_a_short(lh->AcDate, lha->AcDate);
  get_filetime_a_long(lh->AcDate, lha->AcDate_long);
  get_filetime_a_short(lh->WtDate, lha->WtDate);
  get_filetime_a_long(lh->WtDate, lha->WtDate_long);
  snprintf((char *)lha->Size, 25, "%"PRIu32, lh->Size);
  snprintf((char *)lha->IconIndex, 25, "%"PRId32, lh->IconIndex);
  switch(lh->ShowState)
    {
    case 0x3:
      snprintf((char *)lha->ShowState, 40, "SW_SHOWMAXIMIZED");
      break;
    case 0x7:
      snprintf((char *)lha->ShowState, 40, "SW_SHOWMINNOACTIVE");
      break;
    default:
      snprintf((char *)lha->ShowState, 40, "SW_SHOWNORMAL");
    }
  // Low key
  if(((lh->Hotkey.LowKey > 0x2F ) && (lh->Hotkey.LowKey < 0x5B)))
    {
      sprintf((char *)lk, "\'%u\' ", (unsigned int)lh->Hotkey.LowKey);
    }
  else if(((lh->Hotkey.LowKey > 0x6F ) && (lh->Hotkey.LowKey < 0x88)))
    {
      sprintf((char *)lk, "\'F%u\' ", (unsigned int)lh->Hotkey.LowKey - 111);
    }
  else if(lh->Hotkey.LowKey == 0x90)
    {
      sprintf((char *)lk, "\'NUM LOCK\' ");
    }
  else if(lh->Hotkey.LowKey == 0x90)
    {
      sprintf((char *)lk, "\'SCROLL LOCK\' ");
    }
  else
    {
      sprintf((char *)lk, "[NOT DEFINED] ");
    }
  // High key
  if(lh->Hotkey.HighKey & 0x01)
    {
      sprintf((char *)hk1, "\'SHIFT\' ");
    }
  else
    {
      sprintf((char *)hk1, " ");
    }
  if(lh->Hotkey.HighKey & 0x02)
    {
      sprintf((char *)hk2, "\'CTRL\' ");
    }
  else
    {
      sprintf((char *)hk2, " ");
    }
  if(lh->Hotkey.HighKey & 0x04)
    {
      sprintf((char *)hk3, "\'ALT\'");
    }
  else
    {
      sprintf((char *)hk3, " ");
    }
  //Now write the keys to the LIF_HDR_A
  snprintf((char *)lha->Hotkey, 40, "%s%s%s%s", lk, hk1, hk2, hk3);
  snprintf((char *)lha->Reserved1, 10,"0x0000");
  snprintf((char *)lha->Reserved2, 20,"0x00000000");
  snprintf((char *)lha->Reserved3, 20,"0x00000000");

  return 0; //There aren't any fail states built into this function just yet
  //but feel free to return -1 if you need to
}
//
// Function 'get_idlist()' fills a LIF_IDLIST structure with data from the
// opened file fp
int get_idlist(FILE * fp, int size, int pos, struct LIF * lif)
{
  unsigned char   size_buf[2];   //A small buffer to hold the size element
  int             numItems = 0, posn = pos+2;
  uint16_t        u16;

  if(lif->lh.Flags & 0x00000001)
    {
      fseek(fp, pos, SEEK_SET);
      size_buf[0] = getc(fp);
      size_buf[1] = getc(fp);
      lif->lidl.IDL_size = get_le_uint16(size_buf, 0);
      if(lif->lidl.IDL_size > 0)
        {
          //posn points to the first ItemID relative to the start of TargetIDList
          while(posn < (pos + 2 + lif->lidl.IDL_size))
            {
              fseek(fp, posn, SEEK_SET);
              size_buf[0] = getc(fp);
              size_buf[1] = getc(fp);
              u16 = get_le_uint16(size_buf, 0);
              if(u16 == 0)
                {
                  break;
                }
              numItems++;
              posn = posn + u16;
            }
          lif->lidl.NumItemIDs = numItems;
//      lif->lidl.Items = NULL; //Not filled in this version - sorry
          lif->lidl.IDL_size += 2;
        }
    }
  else //No ID List
    {
      lif->lidl.IDL_size = 0;
      lif->lidl.NumItemIDs = 0;
//    lif->lidl.Items = NULL;
    }

  return 0;
}
//
// Converts the data in a LIF_IDLIST into its ASCII representation
int get_idlist_a(struct LIF_IDLIST * lidl, struct LIF_IDLIST_A * lidla)
{
  if(!(lidl->IDL_size == 0))
    {
      snprintf((char *)lidla->IDL_size, 10,"%"PRIu16, lidl->IDL_size);
      snprintf((char *)lidla->NumItemIDs, 10, "%"PRIu16, lidl->NumItemIDs);
    }
  else
    {
      snprintf((char *)lidla->IDL_size, 10, "[N/A]");
      snprintf((char *)lidla->NumItemIDs, 10, "[N/A]");
    }

  return 0;
}
//
// Fills a LIF_INFO structure with data
// This includes filling the VolID and CNR structures (a lot of data, hence the
// big function)
int get_linkinfo(FILE * fp, int size, int pos, struct LIF * lif)
{
  unsigned char      size_buf[4];   //A small buffer to hold the size element
  unsigned char *    data_buf;
  int       i;
  //uint16_t  u16;

  if(lif->lh.Flags & 0x00000002) //There is a LinkInfo structure
    {
      if(size < (pos + 4)) // There is something wrong because the size of the
        // file is less than the current position
        // (just a sanity check)
        {
          return -1;
        }
      fseek(fp, pos, SEEK_SET);
      for(i = 0; i < 4; i++) // Get the initial size
        {
          size_buf[i] = getc(fp);
        }
      lif->li.Size = get_le_uint32(size_buf, 0);
      // The general idea here is to fill a temporary buffer with the characters
      // (rather than read the data directly) I then have control over reading the
      // data from the buffer as little/big endian or ANSI vs Unicode too.
      data_buf = (unsigned char*) malloc((size_t)(lif->li.Size-4));
      assert(data_buf != NULL);
      for(i = 0; i < (lif->li.Size-4); i++)
        {
          data_buf[i] = getc(fp);
        }
      lif->li.HeaderSize = get_le_uint32(data_buf, 0);
      lif->li.Flags = get_le_uint32(data_buf, 4);
      lif->li.IDOffset = get_le_uint32(data_buf, 8);
      lif->li.LBPOffset = get_le_uint32(data_buf, 12);
      lif->li.CNRLOffset = get_le_uint32(data_buf, 16);
      lif->li.CPSOffset = get_le_uint32(data_buf, 20);
      if(lif->li.HeaderSize >= 0x00000024)
        {
          lif->li.LBPOffsetU = get_le_uint32(data_buf, 24);
          lif->li.CPSOffsetU = get_le_uint32(data_buf, 28);
        }
      else
        {
          lif->li.LBPOffsetU = 0;
          lif->li.CPSOffsetU = 0;
        }
      if(lif->li.Flags & 0x00000001) //There is a Volume ID structure
        {
          //IDOffset is from start of LinkInfo but our buffer starts at pos 4
          lif->li.VolID.Size = get_le_uint32(data_buf, ((lif->li.IDOffset)-4));

          lif->li.VolID.DriveType = \
                                    get_le_uint32(data_buf, ((lif->li.IDOffset)-4)+4);

          lif->li.VolID.DriveSN = get_le_uint32(data_buf, ((lif->li.IDOffset)-4)+8);

          lif->li.VolID.VLOffset = \
                                   get_le_uint32(data_buf, ((lif->li.IDOffset)-4)+12);

          //Is the volume label ANSI or Unicode?
          //There are two ways to work this out...
          //1) lif->li.HeaderSize < 0x00000024 = ANSI (MSSHLLINK Sec 2.3)   or
          //2) lif->li.VolID.VLOffset != 0x00000014 = ANSI  (MSSHLLINK Sec 2.3.1)
          if(lif->li.HeaderSize < 0x00000024) //ANSI
            {
              snprintf((char *)lif->li.VolID.VolumeLabel, 33, "%s", \
                       & data_buf[(lif->li.VolID.VLOffset) + ((lif->li.IDOffset)-4)]);

              if(strlen((char *)lif->li.VolID.VolumeLabel) == 0)
                {
                  snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[EMPTY]");
                }
              //The Unicode Volume Label is not used if the ANSI one is
              lif->li.VolID.VLOffsetU = 0;
              lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;
            }
          else //Unicode
            {
              //Fetch the unicode string
              get_le_unistr(data_buf, \
                            ((lif->li.VolID.VLOffsetU) + ((lif->li.IDOffset)-4)), \
                            33, \
                            lif->li.VolID.VolumeLabelU);

              snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT USED]");
            }

          //Get the Local Base Path string
          //We get this now because it is dependant on the
          //VolumeIDAndLocalBasePath flag being set just as the VolumeID is
          snprintf((char *)lif->li.LBP, 300, "%s", &data_buf[(lif->li.LBPOffset-4)]);
        }
      else //There isn't a VolumeID structure so fill that part of the LIF with
        //empty values
        {
          lif->li.VolID.Size = 0;
          lif->li.VolID.DriveType = 0;
          lif->li.VolID.DriveSN = 0;
          lif->li.VolID.VLOffset = 0;
          lif->li.VolID.VLOffsetU = 0;
          snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT SET]");
          lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;

          snprintf((char *)lif->li.LBP, 300, "[NOT SET]");
        }

      //There is a CNR structure
      if(lif->li.Flags & 0x00000002)
        {
          lif->li.CNR.Size = get_le_uint32(data_buf, (lif->li.CNRLOffset)-4);
          lif->li.CNR.Flags = get_le_uint32(data_buf, (lif->li.CNRLOffset+4)-4);
          lif->li.CNR.NetNameOffset = get_le_uint32(data_buf, \
                                      (lif->li.CNRLOffset+8)-4);
          lif->li.CNR.DeviceNameOffset = get_le_uint32(data_buf, \
                                         (lif->li.CNRLOffset+12)-4);
          lif->li.CNR.NetworkProviderType = get_le_uint32(data_buf, \
                                            (lif->li.CNRLOffset+16)-4);
          if(lif->li.CNR.NetNameOffset > 0x00000014)
            {
              lif->li.CNR.NetNameOffsetU = get_le_uint32(data_buf, \
                                           (lif->li.CNRLOffset+20)-4);
              lif->li.CNR.DeviceNameOffsetU = get_le_uint32(data_buf, \
                                              (lif->li.CNRLOffset+24)-4);
            }
          else
            {
              lif->li.CNR.NetNameOffsetU = 0;
              lif->li.CNR.DeviceNameOffsetU = 0;
            }
          //Get the NetName
          if(lif->li.CNR.NetNameOffset > 0)
            {
              snprintf((char *)lif->li.CNR.NetName, 300, "%s", \
                       & data_buf[(lif->li.CNR.NetNameOffset) + ((lif->li.CNRLOffset)-4)]);
            }
          else
            {
              snprintf((char *)lif->li.CNR.NetName, 300, "[NOT USED]");
            }
          //Get the DeviceName
          if(lif->li.CNR.DeviceNameOffset > 0)
            {
              snprintf((char *)lif->li.CNR.DeviceName, 300, "%s", \
                       & data_buf[(lif->li.CNR.DeviceNameOffset) + ((lif->li.CNRLOffset)-4)]);
            }
          else
            {
              snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT USED]");
            }
          //Get the NetNameUnicode and DeviceNameUnicode
          if(lif->li.CNR.NetNameOffset > 0x00000014)
            {
              get_le_unistr(data_buf, \
                            ((lif->li.CNR.NetNameOffsetU) + ((lif->li.IDOffset)-4)), \
                            300, \
                            lif->li.CNR.NetNameU);
              get_le_unistr(data_buf, \
                            ((lif->li.CNR.DeviceNameOffsetU) + \
                             ((lif->li.IDOffset)-4)), \
                            300, \
                            lif->li.CNR.DeviceNameU);
            }
          else
            {
              lif->li.CNR.NetNameU[0] = (wchar_t)0;
              lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
            }
        }
      else // No CNR
        {
          lif->li.CNR.Size = 0;
          lif->li.CNR.Flags = 0;
          lif->li.CNR.NetNameOffset = 0;
          lif->li.CNR.DeviceNameOffset = 0;
          lif->li.CNR.NetworkProviderType = 0;
          lif->li.CNR.NetNameOffsetU = 0;
          lif->li.CNR.DeviceNameOffsetU = 0;
          snprintf((char *)lif->li.CNR.NetName, 300, "[NOT SET]");
          snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT SET]");
          lif->li.CNR.NetNameU[0] = (wchar_t)0;
          lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
        }

      //There is a common path suffix
      if(lif->li.CPSOffset > 0)
        {
          snprintf((char *)lif->li.CPS, 100, "%s", &data_buf[(lif->li.CPSOffset - 4)]);
        }
      //There is a LocalBasePathUnicode
      if(lif->li.LBPOffsetU > 0)
        {
          //Fetch the unicode string
          get_le_unistr(data_buf, \
                        ((lif->li.LBPOffsetU)-4), \
                        300, \
                        lif->li.LBPU);
        }
      else
        {
          lif->li.LBPU[0] = (wchar_t)0;
        }
      //There is a CommonPathPathSuffixUnicode
      if(lif->li.CPSOffsetU > 0)
        {
          //Fetch the unicode string
          get_le_unistr(data_buf, \
                        ((lif->li.LBPOffsetU)-4), \
                        100, \
                        lif->li.CPSU);
        }
      else
        {
          lif->li.CPSU[0] = (wchar_t)0;
        }

      free(data_buf);
    }
  else // What to fill the LinkInfo structure with, in case it does not exist
    {
      lif->li.Size = 0;
      lif->li.HeaderSize = 0;
      lif->li.Flags = 0;
      lif->li.IDOffset = 0;
      lif->li.LBPOffset = 0;
      lif->li.CNRLOffset = 0;
      lif->li.CPSOffset = 0;
      lif->li.LBPOffsetU = 0;
      lif->li.CPSOffsetU = 0;

      lif->li.VolID.Size = 0;
      lif->li.VolID.DriveType = 0;
      lif->li.VolID.DriveSN = 0;
      lif->li.VolID.VLOffset = 0;
      lif->li.VolID.VLOffsetU = 0;
      snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT SET]");
      lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;
      snprintf((char *)lif->li.LBP, 300, "[NOT SET]");
      lif->li.CNR.Size = 0;
      lif->li.CNR.Flags = 0;
      lif->li.CNR.NetNameOffset = 0;
      lif->li.CNR.DeviceNameOffset = 0;
      lif->li.CNR.NetworkProviderType = 0;
      lif->li.CNR.NetNameOffsetU = 0;
      lif->li.CNR.DeviceNameOffsetU = 0;
      snprintf((char *)lif->li.CNR.NetName, 300, "[NOT SET]");
      snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT SET]");
      lif->li.CNR.NetNameU[0] = (wchar_t)0;
      lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
      snprintf((char *)lif->li.CPS, 100, "[NOT SET]");
      lif->li.LBPU[0] = (wchar_t)0;
      lif->li.CPSU[0] = (wchar_t)0;
    }

  return 0;
}
//
// Converts a LIF_INFO structure into its ASCII representation
int get_linkinfo_a(struct LIF_INFO *li, struct LIF_INFO_A *lia)
{
  if(li->Size > 0)
    {
      snprintf((char *)lia->Size, 10, "%"PRIu32, li->Size);
      snprintf((char *)lia->HeaderSize, 10, "%"PRIu32, li->HeaderSize);
      snprintf((char *)lia->Flags, 100, "0x%.8"PRIX32"  ", li->Flags);
      if(li->Flags & 0x00000001)
        strcat((char *)lia->Flags, "VolumeIDAndLocalBasePath | ");
      if(li->Flags & 0x00000002)
        strcat((char *)lia->Flags, "CommonNetworkRelativeLinkAndPathSuffix | ");
      if(strlen((char *)lia->Flags) > 11)
        lia->Flags[strlen((char *)lia->Flags)-3] = (char)0;
      snprintf((char *)lia->IDOffset, 10, "%"PRIu32, li->IDOffset);
      snprintf((char *)lia->LBPOffset, 10, "%"PRIu32, li->LBPOffset);
      snprintf((char *)lia->CNRLOffset, 10, "%"PRIu32, li->CNRLOffset);
      snprintf((char *)lia->CPSOffset, 10, "%"PRIu32, li->CPSOffset);
      if(li->HeaderSize >= 0x00000024)
        {
          snprintf((char *)lia->LBPOffsetU, 10, "%"PRIu32, li->LBPOffsetU);
          snprintf((char *)lia->CPSOffsetU, 10, "%"PRIu32, li->CPSOffsetU);
        }
      else
        {
          snprintf((char *)lia->LBPOffsetU, 10, "[NOT SET]");
          snprintf((char *)lia->CPSOffsetU, 10, "[NOT SET]");
        }
      //There is a Volume ID structure (and a LBP)
      if(li->Flags & 0x00000001)
        {
          snprintf((char *)lia->VolID.Size, 10, "%"PRIu32, li->VolID.Size);
          switch(li->VolID.DriveType)
            {
            case 0x00000000:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_UNKNOWN");
              break;
            case 0x00000001:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_NO_ROOT_DIR");
              break;
            case 0x00000002:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_REMOVABLE");
              break;
            case 0x00000003:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_FIXED");
              break;
            case 0x00000004:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_REMOTE");
              break;
            case 0x00000005:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_CDROM");
              break;
            case 0x00000006:
              snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_RAMDISK");
              break;
            default:
              snprintf((char *)lia->VolID.DriveType, 20, "ERROR");
            }
          snprintf((char *)lia->VolID.DriveSN, 20, "%"PRIX32, li->VolID.DriveSN);
          snprintf((char *)lia->VolID.VLOffset, 20, "%"PRIu32, li->VolID.VLOffset);
          snprintf((char *)lia->VolID.VLOffsetU, 20, "%"PRIu32, li->VolID.VLOffsetU);
          snprintf((char *)lia->VolID.VolumeLabel, 33, "%s", li->VolID.VolumeLabel);
          snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
          switch(li->VolID.VolumeLabelU[0] )
            {
            case 0:
              snprintf((char *)lia->VolID.VolumeLabelU, 33, "[NOT SET]");
              break;
            case 1:
              snprintf((char *)lia->VolID.VolumeLabelU, 33, "[EMPTY]");
              break;
            default:
              snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
            }

          snprintf((char *)lia->LBP, 300, "%s", li->LBP);

        }
      else //If there is No VolID and LBP
        {
          sprintf((char *)lia->VolID.Size, "[N/A]");
          sprintf((char *)lia->VolID.DriveType, "[N/A]");
          sprintf((char *)lia->VolID.DriveSN, "[N/A]");
          sprintf((char *)lia->VolID.VLOffset, "[N/A]");
          sprintf((char *)lia->VolID.VLOffsetU, "[N/A]");
          snprintf((char *)lia->VolID.VolumeLabel, 33, "%s", li->VolID.VolumeLabel);
          snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
          sprintf((char *)lia->VolID.VolumeLabelU, "[NOT SET]");
          snprintf((char *)lia->LBP, 300, "%s", li->LBP);
        }
      if(strlen((char *)li->CPS) > 0)
        {
          snprintf((char *)lia->CPS, 100, "%s", li->CPS);
        }
      else
        {
          snprintf((char *)lia->CPS, 100, "[NOT SET]");
        }
      //Is there a CNR?
      if(li->Flags & 0x00000002)
        {
          snprintf((char *)lia->CNR.Size, 10, "%"PRIu32, li->CNR.Size);
          switch(li->CNR.Flags)
            {
            case 0:
              snprintf((char *)lia->CNR.Flags, 30, "[NO FLAGS SET]");
              break;
            case 1:
              snprintf((char *)lia->CNR.Flags, 30, "ValidDevice");
              break;
            case 2:
              snprintf((char *)lia->CNR.Flags, 30, "ValidNetType");
              break;
            case 3:
              snprintf((char *)lia->CNR.Flags, 30, "ValidDevice | ValidNetType");
              break;
            default:
              snprintf((char *)lia->CNR.Flags, 30, "[INVALID VALUE]");
            }
          snprintf((char *)lia->CNR.NetNameOffset, 10, \
                   "%"PRIu32, li->CNR.NetNameOffset);
          snprintf((char *)lia->CNR.DeviceNameOffset, 10, \
                   "%"PRIu32, li->CNR.DeviceNameOffset);
          if(li->CNR.Flags & 0x00000002)
            {
              switch(li->CNR.NetworkProviderType)
                {
                case 0x001A0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_AVID");
                  break;
                case 0x001B0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DOCUSPACE");
                  break;
                case 0x001C0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MANGOSOFT");
                  break;
                case 0x001D0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SERNET");
                  break;
                case 0x001E0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RIVERFRONT1");
                  break;
                case 0x001F0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RIVERFRONT2");
                  break;
                case 0x00200000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DECORB");
                  break;
                case 0x00210000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_PROTSTOR");
                  break;
                case 0x00220000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_FJ_REDIR");
                  break;
                case 0x00230000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DISTINCT");
                  break;
                case 0x00240000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_TWINS");
                  break;
                case 0x00250000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RDR2SAMPLE");
                  break;
                case 0x00260000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_CSC");
                  break;
                case 0x00270000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_3IN1");
                  break;
                case 0x00290000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_EXTENDNET");
                  break;
                case 0x002A0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_STAC");
                  break;
                case 0x002B0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_FOXBAT");
                  break;
                case 0x002C0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_YAHOO");
                  break;
                case 0x002D0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_EXIFS");
                  break;
                case 0x002E0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DAV");
                  break;
                case 0x002F0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_KNOWARE");
                  break;
                case 0x00300000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_OBJECT_DIRE");
                  break;
                case 0x00310000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MASFAX");
                  break;
                case 0x00320000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_HOB_NFS");
                  break;
                case 0x00330000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SHIVA");
                  break;
                case 0x00340000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_IBMAL");
                  break;
                case 0x00350000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_LOCK");
                  break;
                case 0x00360000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_TERMSRV");
                  break;
                case 0x00370000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SRT");
                  break;
                case 0x00380000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_QUINCY");
                  break;
                case 0x00390000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_OPENAFS");
                  break;
                case 0x003A0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_AVID1");
                  break;
                case 0x003B0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DFS");
                  break;
                case 0x003C0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_KWNP");
                  break;
                case 0x003D0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_ZENWORKS");
                  break;
                case 0x003E0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DRIVEONWEB");
                  break;
                case 0x003F0000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_VMWARE");
                  break;
                case 0x00400000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RSFX");
                  break;
                case 0x00410000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MFILES");
                  break;
                case 0x00420000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MS_NFS");
                  break;
                case 0x00430000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_GOOGLE");
                  break;
                case 0x00020000:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, \
                           "[UNKNOWN (Possibly Local Server)]");
                  break;
                default:
                  snprintf((char *)lia->CNR.NetworkProviderType, 35, \
                           "0x%.8"PRIX32" [UNKNOWN TYPE]", li->CNR.NetworkProviderType);
                }
            }
          else //Not a valid Net Type
            {
              snprintf((char *)lia->CNR.NetworkProviderType, 35, "N/A");
            }
          if(li->CNR.NetNameOffset > 0x00000014)
            {
              snprintf((char *)lia->CNR.NetNameOffsetU, 10, \
                       "%"PRIu32, li->CNR.NetNameOffsetU);
              snprintf((char *)lia->CNR.DeviceNameOffsetU, 10, \
                       "%"PRIu32, li->CNR.DeviceNameOffsetU);
            }
          else //Unicode strings not used
            {
              sprintf((char *)lia->CNR.NetNameOffsetU, "N/A");
              sprintf((char *)lia->CNR.DeviceNameOffsetU, "N/A");
            }
          snprintf((char *)lia->CNR.NetName, 300, "%s", li->CNR.NetName);
          snprintf((char *)lia->CNR.DeviceName, 300, "%s", li->CNR.DeviceName);
          if(li->CNR.NetNameOffset > 0x00000014)
            {
              snprintf((char *)lia->CNR.NetNameU, 300, "%ls", li->CNR.NetNameU);
              snprintf((char *)lia->CNR.DeviceNameU, 300, "%ls", li->CNR.DeviceNameU);
            }
          else //Unicode strings not used
            {
              sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
              sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
            }
        }
      //No CNR
      else
        {
          sprintf((char *)lia->CNR.Size, "N/A");
          sprintf((char *)lia->CNR.Flags, "N/A");
          sprintf((char *)lia->CNR.NetNameOffset, "N/A");
          sprintf((char *)lia->CNR.DeviceNameOffset, "N/A");
          sprintf((char *)lia->CNR.NetworkProviderType, "N/A");
          sprintf((char *)lia->CNR.NetNameOffsetU, "N/A");
          sprintf((char *)lia->CNR.DeviceNameOffsetU, "N/A");
          sprintf((char *)lia->CNR.DeviceName, "[NOT SET]");
          sprintf((char *)lia->CNR.NetName, "[NOT SET]");
          sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
          sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
        }
      if(li->LBPOffsetU > 0) //There is a unicode local base path
        {
          snprintf((char *)lia->LBPU, 300, "%ls", li->LBPU);
        }
      else // There is no Unicode local base path
        {
          snprintf((char *)lia->LBPU, 300, "[NOT SET]");
        }
      if(li->CPSOffsetU > 0) //There is a unicode common path suffix
        {
          snprintf((char *)lia->CPSU, 100, "%ls", li->CPSU);
        }
      else // There is no Unicode common path suffix
        {
          snprintf((char *)lia->CPSU, 100, "[NOT SET]");
        }
    }
  else //If there is no LinkInfo
    {
      sprintf((char *)lia->Size, "[N/A]");
      sprintf((char *)lia->HeaderSize, "[N/A]");
      sprintf((char *)lia->Flags, "[N/A]");
      sprintf((char *)lia->IDOffset, "[N/A]");
      sprintf((char *)lia->LBPOffset, "[N/A]");
      sprintf((char *)lia->CNRLOffset, "[N/A]");
      sprintf((char *)lia->CPSOffset, "[N/A]");
      sprintf((char *)lia->LBPOffsetU, "[N/A]");
      sprintf((char *)lia->CPSOffsetU, "[N/A]");
      sprintf((char *)lia->VolID.Size, "[N/A]");
      sprintf((char *)lia->VolID.DriveType, "[N/A]");
      sprintf((char *)lia->VolID.DriveSN, "[N/A]");
      sprintf((char *)lia->VolID.VLOffset, "[N/A]");
      sprintf((char *)lia->VolID.VLOffsetU, "[N/A]");
      snprintf((char *)lia->VolID.VolumeLabel, 33, "[NOT SET]");
      snprintf((char *)lia->VolID.VolumeLabelU, 33, "[NOT SET]");
      snprintf((char *)lia->LBP, 300, "%s", li->LBP);
      sprintf((char *)lia->CNR.Size, "N/A");
      sprintf((char *)lia->CNR.Flags, "N/A");
      sprintf((char *)lia->CNR.NetNameOffset, "N/A");
      sprintf((char *)lia->CNR.DeviceNameOffset, "N/A");
      sprintf((char *)lia->CNR.NetworkProviderType, "N/A");
      sprintf((char *)lia->CNR.NetNameOffsetU, "N/A");
      sprintf((char *)lia->CNR.DeviceNameOffsetU, "N/A");
      sprintf((char *)lia->CNR.DeviceName, "[NOT SET]");
      sprintf((char *)lia->CNR.NetName, "[NOT SET]");
      sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
      sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
      snprintf((char *)lia->CPS, 100, "[NOT SET]");
      snprintf((char *)lia->LBPU, 300, "[NOT SET]");
      snprintf((char *)lia->CPSU, 100, "[NOT SET]");
    }

  return 0;
}
//
//Fills the LIF_STRINGDATA structure with the necessary data (converting
//Unicode strings to ASCII if necessary)
int get_stringdata(FILE * fp, int pos, struct LIF * lif)
{
  unsigned char      size_buf[2];   //A small buffer to hold the size element
  uint32_t           tsize = 0, str_size = 0;
  int                i, j;
  unsigned char               data_buf[600];
  wchar_t            uni_buf[300];

  // Initialise the lif->lsd values to 0
  for(i = 0; i < 5; i++)
    {
      lif->lsd.CountChars[i] = 0;
      lif->lsd.Data[i][0] = 0;
    }
  if(lif->lh.Flags & 0x00000080)  //Unicode
    {
      for(i = 0; i < 5; i++)
        {
          if(lif->lh.Flags & (0x00000004 << i))
            {
              fseek(fp, (pos + (tsize)), SEEK_SET);
              size_buf[0] = getc(fp);
              size_buf[1] = getc(fp);
              str_size = get_le_uint16(size_buf, 0);
              lif->lsd.CountChars[i] = str_size;
              if(str_size > 299)
                {
                  str_size = 299;
                }
              for(j = 0; j < (str_size * 2); j++)
                {
                  data_buf[j] = getc(fp);
                }
              data_buf[str_size * 2] = 0;
              data_buf[(str_size * 2) + 1] = 0;
              get_le_unistr(data_buf, 0, str_size+1, uni_buf);
              snprintf((char *)lif->lsd.Data[i], 300, "%ls", uni_buf);

              tsize += ((lif->lsd.CountChars[i] * 2) + 2);
            }
        }
    }
  else //ASCII
    {
      for(i = 0; i < 5; i++)
        {
          if(lif->lh.Flags & (0x00000004 << i))
            {
              fseek(fp, (pos + (tsize)), SEEK_SET);
              size_buf[0] = getc(fp);
              size_buf[1] = getc(fp);
              str_size = get_le_uint16(size_buf, 0);
              lif->lsd.CountChars[i] = str_size;
              if(str_size > 299)
                {
                  str_size = 299;
                }
              for(j = 0; j < lif->lsd.CountChars[i]; j++)
                {
                  data_buf[j] = getc(fp);
                }
              get_chars(data_buf, 0, str_size+1, lif->lsd.Data[i]);
              if(str_size == 299)
                {
                  lif->lsd.Data[i][str_size] = 0;
                }

              tsize += (lif->lsd.CountChars[i] + 2);
            }
        }
    }

  lif->lsd.Size = tsize;
  return 0;
}
//
//Function get_stringdata_a(struct LIF_STRINGDATA *, struct LIF_STRINGDATA_A)
//copies the strings and creates an ASCII representation of the CountChars
//and Size values.
int get_stringdata_a(struct LIF_STRINGDATA * lsd, struct LIF_STRINGDATA_A * lsda)
{
  int i;

  snprintf((char *)lsda->Size, 10, "%"PRIu32, lsd->Size);
  for(i=0; i<5; i++)
    {
      snprintf((char *)lsda->CountChars[i], 10, "%"PRIu32, lsd->CountChars[i]);
      if(lsd->CountChars[i] > 0)
        {
          snprintf((char *)lsda->Data[i], 300, "%s", lsd->Data[i]);
        }
      else
        {
          snprintf((char *)lsda->Data[i], 300, "[EMPTY]");
        }
    }
  return 0;
}
//
//Fills the LIF_EXTRA_DATA structure with the necessary data (converting
//Unicode strings to ASCII if necessary)
int get_extradata(FILE * fp, int pos, struct LIF * lif)
{
  int                i, posn = 0;
  uint32_t           blocksize, blocksig, datasize;
  unsigned char      size_buf[4];   //A small buffer to hold the size element
  unsigned char      sig_buf[4];
  unsigned char      data_buf[4096];

  led_setnull(&lif->led); //set all the extradata sections to 0 initially
  lif->led.edtypes = EMPTY;

  fseek(fp, pos, SEEK_SET);
  size_buf[0] = getc(fp);
  size_buf[1] = getc(fp);
  size_buf[2] = getc(fp);
  size_buf[3] = getc(fp);
  blocksize = get_le_uint32(size_buf, 0);
  while (blocksize > 3) //The spec is that anything less than 4 signifies
    // a terminal block
    {
      if(blocksize >= 4096)    //Don't want to exceed the limits of the buffer
        //4KiB seems a reasonable limit (for now)
        {
          fprintf(stderr, "ExtraData block is too large\n");
          fprintf(stderr, "Processing of ExtraData block terminated.\n");
          return -1;
        }
      datasize = blocksize - 8;
      sig_buf[0] = getc(fp);
      sig_buf[1] = getc(fp);
      sig_buf[2] = getc(fp);
      sig_buf[3] = getc(fp);
      blocksig = get_le_uint32(sig_buf, 0);
      for (i = 0; i < datasize; i++)
        {
          data_buf[i] = getc(fp);
        }
      switch(blocksig)
        {
        case 0xA0000001: // Signature for a EnvironmentVariableDataBlock
          lif->led.lep.Size = blocksize;
          lif->led.lep.sig = blocksig;
          lif->led.edtypes += ENVIRONMENT_PROPS;
          //TODO Fill this
          break;
        case 0xA0000002: // Signature for a ConsoleDataBlock
          lif->led.lcp.Size = blocksize;
          lif->led.lcp.sig = blocksig;
          lif->led.edtypes += CONSOLE_PROPS;
          //TODO Fill this
          break;
        case 0xA0000003: //Signature for a TrackerDataBlock
          lif->led.ltp.Size = blocksize;
          lif->led.ltp.sig = blocksig;
          lif->led.edtypes += TRACKER_PROPS;
          get_ltp(&lif->led.ltp, data_buf);
          break;
        case 0xA0000004: // Signature for a ConsoleFEDataBlock
          lif->led.lcfep.Size = blocksize;
          lif->led.lcfep.sig = blocksig;
          lif->led.edtypes += CONSOLE_FE_PROPS;
          //TODO Fill this
          break;
        case 0xA0000005: // Signature for a SpecialFolderDataBlock
          lif->led.lsfp.Size = blocksize;
          lif->led.lsfp.sig = blocksig;
          lif->led.edtypes += SPECIAL_FOLDER_PROPS;
          lif->led.lsfp.SpecialFolderID = get_le_uint32(data_buf, 0);
          lif->led.lsfp.Offset = get_le_uint32(data_buf, 4);
          break;
        case 0xA0000006: // Signature for a DarwinDataBlock
          lif->led.ldp.Size = blocksize;
          lif->led.ldp.sig = blocksig;
          lif->led.edtypes += DARWIN_PROPS;
          //TODO Fill this
          break;
        case 0xA0000007: // Signature for a IconEnvironmentDataBlock
          lif->led.liep.Size = blocksize;
          lif->led.liep.sig = blocksig;
          lif->led.edtypes += ICON_ENVIRONMENT_PROPS;
          //TODO Fill this
          break;
        case 0xA0000008: // Signature for a ShimDataBlock
          lif->led.lsp.Size = blocksize;
          lif->led.lsp.sig = blocksig;
          lif->led.edtypes += SHIM_PROPS;
          if(get_le_unistr(data_buf, 0, 600, lif->led.lsp.LayerName) < 0)
            {
              lif->led.lsp.LayerName[0] = (wchar_t)0;
            }
          break;
        case 0xA0000009: // Signature for a PropertyStoreDataBlock
          lif->led.lpsp.Size = blocksize;
          lif->led.lpsp.sig = blocksig;
          lif->led.edtypes += PROPERTY_STORE_PROPS;
          lif->led.lpsp.NumStores = 0;
          i = 0;
          //TODO Fill This
          break;
        case 0xA000000A: // Signature for a VistaAndAboveIDListDataBlock
          lif->led.lvidlp.Size = blocksize;
          lif->led.lvidlp.sig = blocksig;
          lif->led.lvidlp.NumItemIDs = 0;
          lif->led.edtypes += VISTA_AND_ABOVE_IDLIST_PROPS;
          while(posn < (lif->led.lvidlp.Size - 8))
            {
              i = get_le_uint16(data_buf, posn);
              posn += i;
              lif->led.lvidlp.NumItemIDs++;
            }
          break;
        case 0xA000000B: // Signature for a KnownFolderDataBlock
          lif->led.lkfp.Size = blocksize;
          lif->led.lkfp.sig = blocksig;
          lif->led.edtypes += KNOWN_FOLDER_PROPS;
          lif->led.lkfp.KFGUID.Data1 = get_le_uint32(data_buf, 0);
          lif->led.lkfp.KFGUID.Data2 = get_le_uint16(data_buf, 4);
          lif->led.lkfp.KFGUID.Data3 = get_le_uint16(data_buf, 6);
          get_chars(data_buf, 8, 2, lif->led.lkfp.KFGUID.Data4hi);
          get_chars(data_buf, 10, 6, lif->led.lkfp.KFGUID.Data4lo);
          lif->led.lkfp.KFOffset = get_le_uint32(data_buf, 16);
          break;
        }

      size_buf[0] = getc(fp); //Get the next block size (or the terminal block)
      size_buf[1] = getc(fp);
      size_buf[2] = getc(fp);
      size_buf[3] = getc(fp);
      blocksize = get_le_uint32(size_buf, 0);
    }//End of the while loop that parses each ExtraData block

  lif->led.terminal = blocksize;

  //Compute the size of the ExtraData section
  lif->led.Size = lif->led.lcp.Size +
                  lif->led.lcfep.Size +
                  lif->led.ldp.Size +
                  lif->led.lep.Size +
                  lif->led.liep.Size +
                  lif->led.lkfp.Size +
                  lif->led.lpsp.Size +
                  lif->led.lsp.Size +
                  lif->led.lsfp.Size +
                  lif->led.ltp.Size +
                  lif->led.lvidlp.Size +
                  4;
  return lif->led.Size;
}
//
//Function get_extradata_a(struct LIF_EXTRA_DATA*, struct LIF_EXTRA_DATA_A)
//copies the strings and creates an ASCII representation of the data.
int get_extradata_a(struct LIF_EXTRA_DATA * led, struct LIF_EXTRA_DATA_A * leda)
{
  int i;

  snprintf((char *)leda->Size, 10, "%"PRIu32, led->Size);
  snprintf((char *)leda->edtypes, 2, " ");
  //Get Console Data block
  if(led->edtypes & CONSOLE_PROPS)
    {
      strcat((char *)leda->edtypes, "CONSOLE_PROPS | ");

    }
  else
    {

    }
  //Get Console FE Data block
  if(led->edtypes & CONSOLE_FE_PROPS)
    {
      strcat((char *)leda->edtypes, "CONSOLE_FE_PROPS | ");

    }
  else
    {

    }
  //Get Darwin Data block
  if(led->edtypes & DARWIN_PROPS)
    {
      strcat((char *)leda->edtypes, "DARWIN_PROPS | ");

    }
  else
    {

    }
  //Get Environment Variable Data block
  if(led->edtypes & ENVIRONMENT_PROPS)
    {
      strcat((char *)leda->edtypes, "ENVIRONMENT_PROPS | ");

    }
  else
    {

    }
  //Get Icon Environment Data block
  if(led->edtypes & ICON_ENVIRONMENT_PROPS)
    {
      strcat((char *)leda->edtypes, "ICON_ENVIRONMENT_PROPS | ");

    }
  else
    {

    }
  //Get Known Folder data block
  if(led->edtypes & KNOWN_FOLDER_PROPS)
    {
      strcat((char *)leda->edtypes, "KNOWN_FOLDER_PROPS | ");
      snprintf((char *)leda->lkfpa.Size, 10, "%"PRIu32, led->lkfp.Size);
      snprintf((char *)leda->lkfpa.sig, 12, "0x%.8"PRIX32, led->lkfp.sig);
      get_droid_a(&led->lkfp.KFGUID, &leda->lkfpa.KFGUID);
      snprintf((char *)leda->lkfpa.KFOffset, 10, "%"PRIu32, led->lkfp.KFOffset);
    }
  else
    {
      snprintf((char *)leda->lkfpa.Size, 10, "[N/A]");
      snprintf((char *)leda->lkfpa.sig, 10, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.UUID, 40, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.Version, 40, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.Variant, 40, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.Time, 30, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.Time_long, 40, "[N/A]");
      snprintf((char *)leda->lkfpa.KFGUID.ClockSeq, 10, "[N/A]");
      snprintf((char *)leda->lkfpa.KFOffset, 10, "[N/A]");
    }
  //Get Property Store data block
  if(led->edtypes & PROPERTY_STORE_PROPS)
    {
      strcat((char *)leda->edtypes, "PROPERTY_STORE_PROPS | ");
      snprintf((char *)leda->lpspa.Size, 10, "%"PRIu32, led->lpsp.Size);
      snprintf((char *)leda->lpspa.sig, 12, "0x%.8"PRIX32, led->lpsp.sig);
      snprintf((char *)leda->lpspa.NumStores, 10, "%"PRIu32, led->lpsp.NumStores);
    }
  else
    {
      snprintf((char *)leda->lpspa.Size, 10, "[N/A]");
      snprintf((char *)leda->lpspa.sig, 10, "[N/A]");
      snprintf((char *)leda->lpspa.NumStores, 10, "[N/A]");
    }
  //Get Shim Data block
  if(led->edtypes & SHIM_PROPS)
    {
      strcat((char *)leda->edtypes, "SHIM_PROPS | ");
      snprintf((char *)leda->lspa.Size, 10, "%"PRIu32, led->lsp.Size);
      snprintf((char *)leda->lspa.sig, 12, "0x%.8"PRIX32, led->lsp.sig);
      snprintf((char *)leda->lspa.LayerName, 600, "%ls", led->lsp.LayerName);
    }
  else
    {
      snprintf((char *)leda->lspa.Size, 10, "[N/A]");
      snprintf((char *)leda->lspa.sig, 10, "[N/A]");
      snprintf((char *)leda->lspa.LayerName, 600, "[N/A]");
    }
  //Get Special Folder Data block
  if(led->edtypes & SPECIAL_FOLDER_PROPS)
    {
      strcat((char *)leda->edtypes, "SPECIAL_FOLDER_PROPS | ");
      snprintf((char *)leda->lsfpa.Size, 10, "%"PRIu32, led->lsfp.Size);
      snprintf((char *)leda->lsfpa.sig, 12, "0x%.8"PRIX32, led->lsfp.sig);
      snprintf((char *)leda->lsfpa.SpecialFolderID, 10, "%"PRIu32,\
               led->lsfp.SpecialFolderID);
      snprintf((char *)leda->lsfpa.Offset, 10, "%"PRIu32,\
               led->lsfp.Offset);
    }
  else
    {
      snprintf((char *)leda->lsfpa.Size, 10, "[N/A]");
      snprintf((char *)leda->lsfpa.sig, 10, "[N/A]");
      snprintf((char *)leda->lsfpa.SpecialFolderID, 10, "[N/A]");
      snprintf((char *)leda->lsfpa.Offset, 10, "[N/A]");
    }
  //Get the Link File Tracker Properties
  if(led->edtypes & TRACKER_PROPS)
    {
      strcat((char *)leda->edtypes, "TRACKER_PROPS | ");
      snprintf((char *)leda->ltpa.Size, 10, "%"PRIu32, led->ltp.Size);
      snprintf((char *)leda->ltpa.sig, 12, "0x%.8"PRIX32, led->ltp.sig);
      snprintf((char *)leda->ltpa.Length, 10, "%"PRIu32, led->ltp.Length);
      snprintf((char *)leda->ltpa.Version, 10, "%"PRIu32, led->ltp.Version);
      snprintf((char *)leda->ltpa.MachineID, 17, "%s", led->ltp.MachineID);
      get_droid_a(&led->ltp.Droid1, &leda->ltpa.Droid1);
      get_droid_a(&led->ltp.Droid2, &leda->ltpa.Droid2);
      get_droid_a(&led->ltp.DroidBirth1, &leda->ltpa.DroidBirth1);
      get_droid_a(&led->ltp.DroidBirth2, &leda->ltpa.DroidBirth2);
    }
  else
    {
      snprintf((char *)leda->ltpa.Size, 10, "[N/A]");
      snprintf((char *)leda->ltpa.sig, 10, "[N/A]");
      snprintf((char *)leda->ltpa.Length, 10, "[N/A]");
      snprintf((char *)leda->ltpa.Version, 10, "[N/A]");
      snprintf((char *)leda->ltpa.MachineID, 17, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.UUID, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.Version, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.Variant, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.Time, 30, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.Time_long, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.ClockSeq, 10, "[N/A]");
      snprintf((char *)leda->ltpa.Droid1.Node, 20, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.UUID, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.Version, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.Variant, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.Time, 30, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.Time_long, 40, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.ClockSeq, 10, "[N/A]");
      snprintf((char *)leda->ltpa.Droid2.Node, 20, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.UUID, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.Version, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.Variant, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.Time, 30, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.Time_long, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.ClockSeq, 10, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth1.Node, 20, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.UUID, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.Version, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.Variant, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.Time, 30, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.Time_long, 40, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.ClockSeq, 10, "[N/A]");
      snprintf((char *)leda->ltpa.DroidBirth2.Node, 20, "[N/A]");
    }
  //Get Vista and above ID List
  if(led->edtypes & VISTA_AND_ABOVE_IDLIST_PROPS)
    {
      strcat((char *)leda->edtypes, "VISTA_AND_ABOVE_IDLIST_PROPS | ");
      snprintf((char *)leda->lvidlpa.Size, 10, "%"PRIu32, led->lvidlp.Size);
      snprintf((char *)leda->lvidlpa.sig, 12, "0x%.8"PRIX32, led->lvidlp.sig);
      snprintf((char *)leda->lvidlpa.NumItemIDs, 10, "%"PRIu32, led->lvidlp.NumItemIDs);
    }
  else
    {
      snprintf((char *)leda->lvidlpa.Size, 10, "[N/A]");
      snprintf((char *)leda->lvidlpa.sig, 12, "[N/A]");
      snprintf((char *)leda->lvidlpa.NumItemIDs, 10, "[N/A]");
    }
  //Trim the edtypes string (if necessary)
  i = strlen((char *)leda->edtypes);
  if(i > 2)
    {
      leda->edtypes[i - 3] = (unsigned char)0;
    }
  else
    {
      snprintf((char *)leda->edtypes, 300, "No EXTRADATA structures");
    }

  //Finaly the terminal block
  snprintf((char *)leda->terminal, 15, "0x.8%"PRIX32, led->terminal);

  return 0;

}
//
//Function get_attr_a(char *attr_str, struct LIF_HDR *lh) converts the
//attributes in the LIF header to a readable string
void get_flag_a(unsigned char *flag_str, struct LIF_HDR *lh)
{
  sprintf((char *)flag_str, " ");

  //check for the states that are constant
  if(lh->Attr == 0) //No attributes set
    {
      sprintf((char *)flag_str, "NONE");
      return;
    }
  if(lh->Flags & 0x1)
    strcat((char *)flag_str, "TARGET_ID_LIST | ");
  if(lh->Flags & 0x2)
    strcat((char *)flag_str, "LINK_INFO | ");
  if(lh->Flags & 0x4)
    strcat((char *)flag_str, "NAME | ");
  if(lh->Flags & 0x8)
    strcat((char *)flag_str, "RELATIVE_PATH | ");
  if(lh->Flags & 0x10)
    strcat((char *)flag_str, "WORKING_DIR | ");
  if(lh->Flags & 0x20)
    strcat((char *)flag_str, "ARGUMENTS | ");
  if(lh->Flags & 0x40)
    strcat((char *)flag_str, "ICON_LOCATION | ");
  if(lh->Flags & 0x80)
    strcat((char *)flag_str, "UNICODE | ");
  if(lh->Flags & 0x100)
    strcat((char *)flag_str, "FORCE_NO_LINK_INFO | ");
  if(lh->Flags & 0x200)
    strcat((char *)flag_str, "EXP_STRING | ");
  if(lh->Flags & 0x400)
    strcat((char *)flag_str, "RUN_SEP_PROCESS | ");
  if(lh->Flags & 0x800)
    strcat((char *)flag_str, "UNUSED_FLAG1 | ");
  if(lh->Flags & 0x1000)
    strcat((char *)flag_str, "DARWIN_ID | ");
  if(lh->Flags & 0x2000)
    strcat((char *)flag_str, "RUN_AS_USER | ");
  if(lh->Flags & 0x4000)
    strcat((char *)flag_str, "EXP_ICON | ");
  if(lh->Flags & 0x8000)
    strcat((char *)flag_str, "NO_PIDL_ALIAS | ");
  if(lh->Flags & 0x10000)
    strcat((char *)flag_str, "UNUSED_FLAG_2 | ");
  if(lh->Flags & 0x20000)
    strcat((char *)flag_str, "SHIM_LAYER | ");
  if(lh->Flags & 0x40000)
    strcat((char *)flag_str, "FORCE_NO_LINK_TRACKER | ");
  if(lh->Flags & 0x80000)
    strcat((char *)flag_str, "TARGET_METADATA | ");
  if(lh->Flags & 0x100000)
    strcat((char *)flag_str, "DISABLE_LINK_PATH_TRACKING | ");
  if(lh->Flags & 0x200000)
    strcat((char *)flag_str, "DISABLE_KNOWN_FOLDER_TRACKING | ");
  if(lh->Flags & 0x400000)
    strcat((char *)flag_str, "DISABLE_KNOWN_FOLDER_ALIAS | ");
  if(lh->Flags & 0x800000)
    strcat((char *)flag_str, "LINK_TO_LINK | ");
  if(lh->Flags & 0x1000000)
    strcat((char *)flag_str, "UNALIAS_ON_SAVE | ");
  if(lh->Flags & 0x2000000)
    strcat((char *)flag_str, "PREFER_ENVIRONMENT_PATH | ");
  if(lh->Flags & 0x4000000)
    strcat((char *)flag_str, "KEEP_LOCAL_ID_LIST | ");

  if(strlen((char *)flag_str) > 0)
    flag_str[strlen((char *)flag_str)-3] = (unsigned char)0;
  return;
}
//
//Function get_attr_a(char *attr_str, struct LIF_HDR *lh) converts the
//attributes in the LIF header to a readable string
void get_attr_a(unsigned char *attr_str, struct LIF_HDR *lh)
{
  sprintf((char *)attr_str, " ");

  //check for the states that are constant
  if(lh->Attr == 0) //No attributes set
    {
      sprintf((char *)attr_str, "NONE");
      return;
    }
  if(lh->Attr == 0x80) //'NORMAL attribute set - no others allowed
    {
      sprintf((char *)attr_str, "NORMAL");
      return;
    }
  if(lh->Attr & 0x1)
    strcat((char *)attr_str, "READONLY | ");
  if(lh->Attr & 0x2)
    strcat((char *)attr_str, "HIDDEN | ");
  if(lh->Attr & 0x4)
    strcat((char *)attr_str, "SYSTEM | ");
  if(lh->Attr & 0x10)
    strcat((char *)attr_str, "DIR | ");
  if(lh->Attr & 0x20)
    strcat((char *)attr_str, "ARCHIVE | ");
  if(lh->Attr & 0x100)
    strcat((char *)attr_str, "TEMP | ");
  if(lh->Attr & 0x200)
    strcat((char *)attr_str, "SPARSE | ");
  if(lh->Attr & 0x400)
    strcat((char *)attr_str, "REPARSE | ");
  if(lh->Attr & 0x800)
    strcat((char *)attr_str, "COMPRESSED | ");
  if(lh->Attr & 0x1000)
    strcat((char *)attr_str, "OFFLINE | ");
  if(lh->Attr & 0x2000)
    strcat((char *)attr_str, "NOT_INDEXED | ");
  if(lh->Attr & 0x4000)
    strcat((char *)attr_str, "ENCRYPTED | ");
  attr_str[strlen((char *)attr_str)-3] = (unsigned char)0;
  return;
}
//
//Function get_le_ulong_int(unsigned char *, int pos) reads 4 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint32_t get_le_uint32(unsigned char buf[], int pos)
{
  int i;
  uint32_t result = 0;

  for(i=0; i < 4; i++)
    {
      result += (buf[(i+pos)] << (8*i));
    }
  return result;
}
//
//Function get_le_ulonglong_int(unsigned char *, int pos) reads 8 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint64_t get_le_uint64(unsigned char buf[], int pos)
{
  uint64_t result = 0;
  uint32_t lo = 0, hi = 0; //Have to split the 64 bits in two
  //because '<< 8*i' breaks on my 64 bit machine
  lo=get_le_uint32(buf, pos);
  hi=get_le_uint32(buf, (pos+4));
  result = ((uint64_t)hi << 32) + lo;
  return result;
}
//
//Function get_le_ulonglong_int(unsigned char *, int pos) reads 8 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
int64_t get_le_int64(unsigned char buf[], int pos)
{
  int64_t result = 0;
  uint64_t interim = 0;
  uint32_t lo = 0, hi = 0; //Have to split the 64 bits in two
  //because '<< 8*i' breaks on my 64 bit machine
  lo=get_le_uint32(buf, pos);
  hi=get_le_uint32(buf, (pos+4));
  interim = ((uint64_t)hi << 32) + lo;
  if(interim > 0x7FFFFFFFFFFFFFFFLL)
    {
      result = (int64_t) (interim - 0x8000000000000000LL);
    }
  else
    {
      result = (int64_t) interim;
    }

  return result;
}
//
//Function get_le_slong_int(unsigned char *, int pos) reads 4 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the signed long integer
//The definition of signed long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
int32_t get_le_int32(unsigned char buf[], int pos)
{
  int i;
  int32_t stor = 0;

  for(i=0; i < 4; i++)
    {
      stor += (buf[(i+pos)] << (8*i));
    }
  return stor;
}
//
//Function get_le_u_int(unsigned char *, int pos) reads 2 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned integer
//The definition of unsigned int is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint16_t get_le_uint16(unsigned char buf[], int pos)
{
  int i;
  uint16_t result = 0;

  for(i=0; i < 2; i++)
    {
      result += (buf[(i+pos)] << (8*i));
    }
  return result;
}
//
//Function get_filetime_ashort(struct FILETIME ft) returns the character string
//representation of the Filetime passed in ft. The output is as per the
//ISO 8601 specification (i.e. 'yyyy-mm-dd hh:mm:ss')
void get_filetime_a_short(int64_t ft, unsigned char result[])
{
  struct tm* tms;
  time_t time;
  int64_t epoch_diff = 11644473600LL, cns2sec = 10000000L;

  ft = ft/cns2sec; //Reduce to seconds
  ft = ft-epoch_diff; //Number of seconds between epoch dates

  if((sizeof(time_t) == sizeof(int64_t)) || ((ft>0) && (ft<0x7FFFFFFFL)))
    {
      time = (time_t) ft;
      tms = gmtime(&time);
      strftime((char *)result, 29, "%Y-%m-%d %H:%M:%S (UTC)", tms);
    }
  //Can't cope with large time_t values
  else if(ft == -11644473600LL)
    {
      snprintf((char *)result, 30, "1601-01-01 00:00:00 (UTC)");
    }
  else
    {
      snprintf((char *)result, 30,"Could not convert");
    }
}
//
//Function get_filetime_a_long(struct FILETIME ft) returns the character string
//representation of the Filetime passed in ft. The output is as per the
//ISO 8601 specification (i.e. 'yyyy-mm-dd hh:mm:ss')
void get_filetime_a_long(int64_t ft, unsigned char result[])
{
  struct tm* tms;
  time_t time;
  uint64_t cns; //100 nanosecond component
  int64_t epoch_diff = 11644473600LL, cns2sec = 10000000;
  unsigned char interim[30];

  cns = (uint64_t)ft%cns2sec; //Extract the 100 nanosecond component
  ft = ft/cns2sec; //Reduce to seconds
  ft = ft-epoch_diff; //Number of seconds between epoch dates
  if((sizeof(time_t) == sizeof(int64_t)) || ((ft>0) && (ft<0x7FFFFFFFL)))
    {
      time = (time_t) ft;
      tms = gmtime(&time);
      strftime((char *)interim, 29, "%Y-%m-%d %H:%M:%S", tms);
      snprintf((char *)result, 40, "%s.%"PRIu64" (UTC)", interim, cns);
    }
  //Can't cope with large time_t values
  else if(ft == -11644473600LL)
    {
      snprintf((char *)result, 40, "1601-01-01 00:00:00.0 (UTC)");
    }
  else
    {
      snprintf((char *)result, 40, "Could not convert");
    }
}
//
//Function get_chars(unsigned char buf[], int pos ,int num, unsigned char targ[])
// reads num unsigned characters starting at pos in buf. It will interpret these
// as big endian (straight copy) and place them in targ
void get_chars(unsigned char buf[], int pos, int num, unsigned char targ[])
{
  int i;

  for(i = 0; i < num; i++)
    {
      targ[i]=buf[(i+pos)];
    }
}
//
//Function get_le_unistr(unsigned char buf[], int pos, int max, wchar_t targ[])
//Fetches a unicode string from buf starting at position pos. It quits when a
//(wchar_t) 0 is encountered or max (in whchar_t terms) characters are copied.
//The encoding is considered to be the Windows default (little endian)
//The result is placed in targ. The function returns the number of wchar_t
//characters that have been copied or -1 on failure.
int get_le_unistr(unsigned char buf[], int pos, int max, wchar_t targ[])
{
  int i, n = 0;
  uint16_t widechar;
  unsigned char temp_buf[2];

  for(i = 0; i < (max-1); i++)
    {
      temp_buf[0] = buf[(i*2)];
      temp_buf[1] = buf[((i*2)+1)];
      widechar = get_le_uint16(temp_buf, 0);
      targ[i] = (wchar_t)widechar;
      n++;
      if(widechar == 0x0000)
        {
          n--;
          break;
        }
    }
  targ[max-1] = (wchar_t) 0;
  return n;
}
//
//Function void get_ltp(struct LIF_TRACKER_PROPS *, unsigned char[]);
//fills the LIF_TRACKER_PROPS properties with the relevant data from the
//character buffer
void get_ltp(struct LIF_TRACKER_PROPS * ltp, unsigned char * data_buf)
{
  int pos = 0;

  //Size and sig should be set
  ltp->Length = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Version = get_le_uint32(data_buf, pos);
  pos += 4;
  get_chars(data_buf, pos, 16, ltp->MachineID);
  pos += 16;
  ltp->MachineID[15] = 0; //Make sure that we can read it as a string
  //Get Droid1
  ltp->Droid1.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Droid1.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->Droid1.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->Droid1.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->Droid1.Data4lo);
  pos += 6;
  //Get Droid2
  ltp->Droid2.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Droid2.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->Droid2.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->Droid2.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->Droid2.Data4lo);
  pos += 6;
  //Get DroidBirth1
  ltp->DroidBirth1.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->DroidBirth1.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->DroidBirth1.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->DroidBirth1.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->DroidBirth1.Data4lo);
  pos += 6;
  //Get DroidBirth2
  ltp->DroidBirth2.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->DroidBirth2.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->DroidBirth2.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->DroidBirth2.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->DroidBirth2.Data4lo);
  pos += 6;
}
//
//Function get_Droid_a(struct LIF_CLSID *, char *)
//Converts the Droid data in the Tracker properties to ASCII versions
void get_droid_a(struct LIF_CLSID * droid, struct LIF_CLSID_A * droid_a)
{
  uint8_t  Version, Variant;
  int16_t Timehi, ClockSeq;
  int64_t Time;
// Build the UUID string
  snprintf((char *)droid_a->UUID, 40, "{%.8"PRIX32"-%.4"PRIX16"-%.2"PRIX16"\
-%.2"PRIX8"%.2"PRIX8"-%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"\
%.2"PRIX8"}",
           droid->Data1,
           droid->Data2,
           droid->Data3,
           droid->Data4hi[0],
           droid->Data4hi[1],
           droid->Data4lo[0],
           droid->Data4lo[1],
           droid->Data4lo[2],
           droid->Data4lo[3],
           droid->Data4lo[4],
           droid->Data4lo[5]);

  // Work out the Version Number
  Version = (uint8_t)((droid->Data3 & 0xF000) >> 12);
  switch(Version)
    {
    case 1:
      snprintf((char *)droid_a->Version, 40, "1 - ITU time based");
      break;
    case 2:
      snprintf((char *)droid_a->Version, 40, "2 - DCE security version");
      break;
    case 3:
      snprintf((char *)droid_a->Version, 40, "3 - ITU name based MD5");
      break;
    case 4:
      snprintf((char *)droid_a->Version, 40, "4 - ITU random number");
      break;
    case 5:
      snprintf((char *)droid_a->Version, 40, "5 - ITU name based SHA1");
      break;
    default:
      snprintf((char *)droid_a->Version, 40, "%"PRIu8" - Unknown version", Version);
    }

  // Work out the Variant
  Variant = (uint8_t)((droid->Data4hi[0] & 0xC0) >> 6);
  switch(Variant)
    {
    case 0:
    case 1:
      snprintf((char *)droid_a->Variant, 40, "NCS backward compatible");
      break;
    case 2:
      snprintf((char *)droid_a->Variant, 40, "ITU variant");
      break;
    case 3:
      snprintf((char *)droid_a->Variant, 40, "Microsoft variant");
      break;
    default:
      snprintf((char *)droid_a->Variant, 40, "Unknown variant"); // Shouldn't happen
    }


  //If it's a time based version
  if(Version == 1)
    {
      //Work out the Clock Sequence
      ClockSeq = ((uint16_t)((droid->Data4hi[0] & 0x3F << 8)))
                 | (droid->Data4hi[1]);
      snprintf((char *)droid_a->ClockSeq, 10, "%"PRIu16, ClockSeq);

      //Work out the time
      //****
      // Build up the time in simple steps
      Time = (int64_t)droid->Data1;
      Time += ((int64_t)droid->Data2) << 32;
      Timehi = ((int16_t)droid->Data3 & 0x0FFF);
      Time += ((int64_t)Timehi) << 48;

      //Now convert to filetime
      Time -= (((int64_t)(1000*1000*10))*((int64_t)(60*60*24))*
               ((int64_t)(17+30+31+(365*18)+5)));
      //Now get sensible answers
      get_filetime_a_long(Time, droid_a->Time_long);
      get_filetime_a_short(Time, droid_a->Time);

      // The MAC address (node)
      snprintf((char *)droid_a->Node, 20,
               "%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8,
               droid->Data4lo[0],
               droid->Data4lo[1],
               droid->Data4lo[2],
               droid->Data4lo[3],
               droid->Data4lo[4],
               droid->Data4lo[5]);
    }
  else
    {
      snprintf((char *)droid_a->Time, 30, "[N/A]");
      snprintf((char *)droid_a->Time_long, 40, "[N/A]");
      snprintf((char *)droid_a->ClockSeq, 10, "[N/A]");
      snprintf((char *)droid_a->Node, 20, "[N/A]");
    }
}
//
//Function led_setnull(struct LIF_EXTRA_DATA * led) just sets all the Extra Data
//structures to 0
void led_setnull(struct LIF_EXTRA_DATA * led)
{
  led->Size = 0;
  led->lcp.Size = 0;
  led->lcp.sig = 0;

  led->lcfep.Size = 0;
  led->lcfep.sig = 0;

  led->ldp.Size = 0;
  led->ldp.sig = 0;

  led->lep.Size = 0;
  led->lep.sig = 0;

  led->liep.Size = 0;
  led->liep.sig = 0;

  led->lkfp.Size = 0;
  led->lkfp.sig = 0;

  led->lpsp.Size = 0;
  led->lpsp.sig = 0;

  led->lsp.Size = 0;
  led->lsp.sig = 0;

  led->lsfp.Size = 0;
  led->lsfp.sig = 0;

  led->ltp.Size = 0;
  led->ltp.sig = 0;

  led->lvidlp.Size = 0;
  led->lvidlp.sig = 0;

  led->terminal = 0;
}
//
//
