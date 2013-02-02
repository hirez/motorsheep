/*--------------------------------------------------------------*/
/*                                                              */
/*    This is MOTORSHEEP, from the AoT.                         */
/*                                                              */
/*  JTL admits responsibility for all the code,                 */
/*  (unless otherwise advertised...) even the really bad        */ 
/*  bits.  Send bug reports, suggestions                        */
/*  (apart from 'bog off') and stuff (live grenades, cash)      */
/*  to Jhawkes@CIX.                                             */
/*                                                              */
/*  This is version 1.9(ish)  10-Nov-88                         */
/*                                                              */
/*--------------------------------------------------------------*/
 
#include <dos.h>                /* Int86 stuff */
#include <disp.h>               /* ZC fast screen package */
#include <stdio.h>              /* Need I explain this ?  */

#define READDISK 2
#define DISKINT 0x0013
#define ESC 27
#define TRUE 1                  /* These are predefined on the Amiga, */
#define FALSE 0                 /* why not on the PeeCee??        */

#define TIMEOUT 128             /* These are all the errors returned */
#define BADSEEK 64              /* by the disk controller, according */
#define BADCTRL 32              /* to 'Norton's guide to cat-boiling */
#define BADCRC 16               /* and PC-welding' >B-)              */
#define BADDMA 8
#define BADSECT 4
#define BADADDR 2
#define BADCOMD 1
#define DMABOUND 9              /* Sorry, Mr Norton... >B-)      */

char TrackBuffer[512*10];       /* Better safe than crash-spotting?  */


main()
{
  int drive,head,track;
  static int AlignTracks[4]={0,1,16,34};
  int ch,x,y,error,ct,sodoff;

  disp_open();                    /* Initialise fast screen routines  */
  disp_move(0,0);
  disp_eeop();                    /* ...and clear the screen */

  DoTitles();      
  ResetController();              /* Recalibrate drive on next access */

  sodoff=FALSE;

  track=0;
  head=0;
  drive=0;        /* Drive A: */
  x=0;            /* Current track for disk alignment purposes */

  ShowDisk(head,track,219);

  while(sodoff==FALSE)
  {
    if (KeyReady())                   /* The power of CIX! */
    {
      ch=getch();
      ShowDisk(head,track,32);
      if (!ch) ch=getch();    /* Deal with cursor keys */
      switch(ch)
      {
        case 77:                        /* Right */
          x++;
          if (x>3) x=0;
          track=AlignTracks[x];
          break;
        case 75:                        /* Left */
          x--;
          if (x<0) x=3;
          track=AlignTracks[x];
          break;
        case 72:                        /* Up   */
        case 80:                        /* Down */
          head=!head;
          break;
        case 100:                 /* D - dump track */
        case  68:
          DisplayTrack(track,head);
          DoTitles();
          break;
        case  87:
        case 119:                 /* W - write track */
          WriteTrack(track,head);
          break;
        case  43:                 /* '+' Step in */
          track += (track<39);
          break;
        case  45:                 /* '-' Step out */
          track -= (track>0);
          break;
        case 114:
        case  82:     /* R - recalibrate */
          ResetController();      
          head=0;
          drive=0;
          track=0;
          break;
        case  59:             /* Alternative help... */
          DoWP();
          DoTitles();
          break;
        case  63:           /* ? - Help key */
          DoHelp();
          DoTitles();
          break;
        case ESC:     /* Esc - Bog off */
          sodoff=TRUE;
          break;
      }       
    }
    error=BIOSRead(drive,head,track);
    DisplayProblem(error);
    disp_move(5,46);
    disp_printf("Drive %d. Head %d. Track %d.  ",drive,head,track);
    ShowDisk(head,track,219);
  }
  disp_move(0,0);   /* Clear the screen. Such tidyness... */
  disp_eeop();
  disp_close();
  ResetController();   /* ...And make sure the drive's back to normal */
}


BIOSRead(drive,head,track)  /* Low-level, whole-track read */
int drive,head,track;
{
  union REGS RegsIn,RegsOut;
  struct SREGS SegRegs;
  int result;

  RegsIn.h.ah=READDISK;
  RegsIn.h.dl=(char)drive;
  RegsIn.h.dh=(char)head;
  RegsIn.h.ch=(char)track;
  RegsIn.h.cl=1;                          /* first sector */
  RegsIn.h.al=9;                          /* read whole track */
  RegsIn.x.bx=FP_OFF(TrackBuffer);  /* Point BIOS at buffer */
  SegRegs.es=FP_SEG(TrackBuffer);   /* More use for large model? */

  int86x(DISKINT,&RegsIn,&RegsOut,&SegRegs);

  result=RegsOut.h.ah;
  return(result);
}

ResetController()  /* Guess... >B-) Just in case it gets confused... */
{
  union REGS regs;

  regs.h.ah=0;
  int86(DISKINT,&regs,&regs);
}

DisplayProblem(error)   /* Well Brian, what went wrong ? */
int error;
{
  int x,y;

  for(y=1;y<3;y++)
  {
    disp_move(y,0);
    disp_eeol();
  }
  disp_move(1,0);
  /* JTL's helpful error messages... */
  if (error & TIMEOUT)
          disp_printf("Timeout. Drive is dead.\n");
  if (error & BADSEEK)
          disp_printf("Bad seek. Drive did not step to right track.\n");
  if (error & BADCTRL)
          disp_printf("Diskette controller knackered.\n");
  if (error & BADCRC)
          disp_printf("CRC error on this track.\n");
  if (error & BADDMA)
          disp_printf("DMA failure. Buy a new logic board.\n");
  if (error & BADSECT)
          disp_printf("Sector has fallen off disk.\n");
  if (error & BADADDR)
          disp_printf("Sector ID NFG. Share and enjoy.\n");
  if (error & BADCOMD)
          disp_printf("Diskette controller confused.\n");
  if (error & DMABOUND)
          disp_printf("DMA outside 64K area. Have a nice day.\n");
}

ShowDisk(head,track,block)  /* Draw pretty pictures of the disk */
int head,track,block;
{
  static int side[2]={6,4};

  disp_move(5,0);
  disp_printf("ÃÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄ´");
  disp_move(side[head],track);
  disp_putc(block);
  return(0);  /* Force of (lattice IV) habit... */
}

WriteTrack(track,head)  /* Dump current track to hard-disk */
int track,head;
{
  FILE *fp;
  char filename[16];

  sprintf(filename,"c:track%02d.%d\0",track,head);
  fp=fopen(filename,"wb");
  fwrite(TrackBuffer,1,sizeof(TrackBuffer),fp);
  fclose(fp);
}

DisplayTrack(track,head)  /* Display current track */
int track,head;
{
  int ch,sector;

  sector=0; /* A bodge. Sectors start at 1, C starts at 0... */
  disp_move(15,0);
  DisplaySector(sector);        /* Bodge */
  disp_move(13,0);        /* Bodge */
  disp_printf("Sector %d. ",sector+1);    /* Bodge */
        
  while((ch=getch()) !=ESC)       /* Esc - universal bugger off key */
  {
    disp_move(15,0);
    disp_eeop();
    switch(ch)
    {
      case 43:                /* '+' - next sector */
        sector += (sector<8);
        DisplaySector(sector);
        break;
      case 45:                /* '-' - previous sector */
        sector -= (sector>0);
        DisplaySector(sector);
        break;
      case 87:
      case 119:   /* W - write track */
        WriteTrack(track,head);
        break;
      default:
        DisplaySector(sector);
        break;
    }
    disp_move(13,0);
    disp_printf("Sector %d. ",sector+1);
  }
  disp_move(13,0);
  disp_eeop();
}
                                
DisplaySector(z)        /* This version chops out ascii < 32. Bodge city. */
int z;
{
  int x,y,byte,offset;

  offset=512*z;
          
  for(y=0;y<8;y++)
  {
    for(x=0;x<64;x++)
    {
      if (TrackBuffer[offset] >31) 
        disp_putc(TrackBuffer[offset]);
      else disp_putc('.');
      offset++;
    }
    disp_putc('\n');
  }
}

KeyReady()    /* This code from sspencer@CIX. Thanks! */
{
  union REGS r;
  unsigned x;
  r.h.ah = 1;
  x = int86(0x16,&r,&r);

/* I've never got this bit to work... God, I must be thick. */
/*  return (x&0x40)?-1:0;  */

  return(!(x & 0x40));
}

DoTitles()
{
  disp_move(0,0);
  disp_printf("                ---=== MOTORSHEEP. AN AoT PRODUCTION. ===---");                
  disp_move(24,0);
  disp_printf(" Press F1 or '?' for HELP...                   ");
}

DoWP()    /* I couldn't resist this... :-)  */
{
  int x;
  disp_move(24,0);
  disp_printf(" Get stuffed! This isn't 1-2-3... :-) ");
  for (x=0;x<20000;x++){ ; }
}

DoHelp()
{
  int ch;

  disp_move(0,0);
  disp_eeop();
  disp_move(0,0);
  disp_printf("                        MOTORSHEEP - BASIC HELP\n\n");
  disp_printf(" Esc - Exit program.\n\n");
  disp_printf(" +   - Step heads in.\n\n");
  disp_printf(" -   - Step heads out.\n\n");
  disp_printf(" D   - Dump current track to screen, one sector at a time. (+/- to\n");
  disp_printf("       scroll through sectors, esc to exit.)\n\n");
  disp_printf(" W   - Write current track to hard disk. (c:) The file is called\n");
  disp_printf("       'tracknn.s', where nn is the track number and s is the side.\n\n");
  disp_printf(" R   - Recalibrate drive & controller. Resets disk controller and steps\n");
  disp_printf("       drive back to track zero.\n\n");
  disp_printf(" The up & down arrow keys swap between heads 0 and 1. The left & right\n");
  disp_printf(" arrows step to tracks 0,1,16 and 34. If you know the significance of\n");
  disp_printf(" these tracks, you don't need any more help from me...\n");
  disp_printf("                                                      ...JTL\n\n");
  disp_printf(" Press 'esc' to return...");
  while((ch=getch()) !=ESC)
  { ; }
  disp_move(0,0);
  disp_eeop();
}
