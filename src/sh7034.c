/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2005, 2013 Theo Berkau
    Copyright 2016 James Laird-Wah

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file sh7034.c
    \brief SH7034 cpu implementation (CD block controller)
*/
#include <stdint.h>
#include "core.h"
#include "sh7034.h"
#include "assert.h"
#include "memory.h"
#include "ygr.h"
#include "debug.h"
#include <stdarg.h>
#include "cd_drive.h"
#ifdef MPEG_CARD
#include "mpeg_card.h"
#endif

//#define SH1_MEM_DEBUG
#ifdef SH1_MEM_DEBUG
#define SH1MEMLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SH1MEMLOG(...)
#endif

//#define WANT_TIMER_TRACE
#ifdef WANT_TIMER_TRACE
#define TIMERTRACE(...) cd_trace_log(__VA_ARGS__)
#else
#define TIMERTRACE(...)
#endif

//#define WANT_PORT_TRACE
#ifdef WANT_PORT_TRACE
#define PORTTRACE(...) cd_trace_log(__VA_ARGS__)
#else
#define PORTTRACE(...)
#endif

u8 transfer_buffer[13] = { 0 };

void update_transfer_buffer()
{
   uint_fast8_t i;
   for (i = 0; i < 13; i++)
   {
      transfer_buffer[i] = T2ReadByte(sh1_cxt.ram, 0x0002D0 + i);//0xF0002D0
   }
}

u16 cr_response[4] = { 0 };

void update_cr_response_values(u32 addr)
{
   if (addr >= 0x0F00026C && addr <= 0x0F000273)
   {
      //0x0F00026C
      cr_response[0] = T2ReadWord(sh1_cxt.ram, 0x00026C + 0);
      cr_response[1] = T2ReadWord(sh1_cxt.ram, 0x00026C + 2);
      cr_response[2] = T2ReadWord(sh1_cxt.ram, 0x00026C + 4);
      cr_response[3] = T2ReadWord(sh1_cxt.ram, 0x00026C + 6);
   }
}

struct Sh1 sh1_cxt;

void onchip_write_timer_byte(struct Onchip * regs, u32 addr, int which_timer, u8 data)
{

   
   //print_serial(1);

   //port_debug();

   //print_timers();

   switch (addr)
   {
   case 0:
      regs->itu.channel[which_timer].tcr = data;
      return;
      break;
   case 1:
      regs->itu.channel[which_timer].tior = data;
      return;
      break;
   case 2:
      regs->itu.channel[which_timer].tier = data;
      return;
      break;
   case 3:
      //can only clear flags
      if (!(data & 1))
         regs->itu.channel[which_timer].tsr &= ~1;

      if (!(data & 2))
         regs->itu.channel[which_timer].tsr &= ~2;

      if (!(data & 4))
         regs->itu.channel[which_timer].tsr &= ~4;
      return;
      break;
   case 4:
      regs->itu.channel[which_timer].tcnt = (regs->itu.channel[which_timer].tcnt & 0xff) | (data << 8);
      return;
      break;
   case 5:
      regs->itu.channel[which_timer].tcnt = (regs->itu.channel[which_timer].tcnt & 0xff00) | data;
      return;
      break;
   case 6:
      regs->itu.channel[which_timer].gra = (regs->itu.channel[which_timer].gra & 0xff) | (data << 8);
      return;
      break;
   case 7:
      regs->itu.channel[which_timer].gra = (regs->itu.channel[which_timer].gra & 0xff00) | data;
      return;
      break;
   case 8:
      regs->itu.channel[which_timer].grb = (regs->itu.channel[which_timer].grb & 0xff) | (data << 8);
      return;
      break;
   case 9:
      regs->itu.channel[which_timer].grb = (regs->itu.channel[which_timer].grb & 0xff00) | data;
      return;
      break;
   case 0xa:
      regs->itu.channel[which_timer].bra = (regs->itu.channel[which_timer].bra & 0xff) | (data << 8);
      return;
      break;
   case 0xb:
      regs->itu.channel[which_timer].bra = (regs->itu.channel[which_timer].bra & 0xff00) | data;
      return;
      break;
   case 0xc:
      regs->itu.channel[which_timer].brb = (regs->itu.channel[which_timer].brb & 0xff) | (data << 8);
      return;
      break;
   case 0xd:
      regs->itu.channel[which_timer].brb = (regs->itu.channel[which_timer].brb & 0xff00) | data;
      return;
      break;
   }

   //assert(0);
}

u8 onchip_read_timer_byte(struct Onchip * regs, u32 addr, int which_timer)
{

   //print_timers();

   switch (addr)
   {
   case 0:
      return regs->itu.channel[which_timer].tcr;
      break;
   case 1:
      return regs->itu.channel[which_timer].tior;
      break;
   case 2:
      return regs->itu.channel[which_timer].tier;
      break;
   case 3:
      return regs->itu.channel[which_timer].tsr;
      break;
   case 4:
      return regs->itu.channel[which_timer].tcnt >> 8;
      break;
   case 5:
      return regs->itu.channel[which_timer].tcnt & 0xff;
      break;
   case 6:
      return regs->itu.channel[which_timer].gra >> 8;
      break;
   case 7:
      return regs->itu.channel[which_timer].gra & 0xff;
      break;
   case 8:
      return regs->itu.channel[which_timer].grb >> 8;
      break;
   case 9:
      return regs->itu.channel[which_timer].grb & 0xff;
      break;
   case 0xa:
      return regs->itu.channel[which_timer].bra >> 8;
      break;
   case 0xb:
      return regs->itu.channel[which_timer].bra & 0xff;
      break;
   case 0xc:
      return regs->itu.channel[which_timer].brb >> 8;
      break;
   case 0xd:
      return regs->itu.channel[which_timer].brb & 0xff;
      break;
   }

   //assert(0);
   return 0;
}

void onchip_write_timer_word(struct Onchip * regs, u32 addr, int which_timer, u16 data)
{
   switch (addr)
   {
   case 0:
   case 1:
   case 2:
   case 3:
      //byte access only
      return;
      break;
   case 4:
      regs->itu.channel[which_timer].tcnt = data;
      return;
      break;
   case 6:
      regs->itu.channel[which_timer].gra = data;
      return;
      break;
   case 8:
      regs->itu.channel[which_timer].grb = data;
      return;
      break;
   case 0xa:
      regs->itu.channel[which_timer].bra = data;
      return;
      break;
   case 0xc:
      regs->itu.channel[which_timer].brb = data;
      return;
      break;
   }

   //assert(0);
}

void onchip_write_byte(struct Onchip * regs, u32 addr, u8 data)
{
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      switch (addr - 0x5FFFEC0)
      {
      case 0:
         regs->sci[0].smr = data;
         return;
         break;
      case 1:
         regs->sci[0].brr = data;
         return;
         break;
      case 2:
         regs->sci[0].scr = data;

         if((data & SCI_TE ) == 0)
            regs->sci[0].ssr |= SCI_TEND;//tend is set
         return;
         break;
      case 3:
         regs->sci[0].tdr = data;
         if (regs->sci[0].tsr_counter == 0) {  // If TSR empty, load it immediately
             regs->sci[0].tsr = data;
             regs->sci[0].tsr_counter = 8;
             regs->sci[0].ssr |= SCI_TDRE;
         } else {
             regs->sci[0].tdr_written = 1; // flag TDR as pending for next time TSR empties
             regs->sci[0].ssr &= ~SCI_TDRE;
         }

         return;
         break;
      case 4:
      {
         if (data == 0)
         {
            int clear_te = 0;
            if (regs->sci[0].ssr & SCI_TDRE)
               clear_te = 1;
            regs->sci[0].ssr &= 0x6;//save tend/mpb bits (read only)

            //tend is cleared when software
            //reads tdre after it has been set to 1
            //then writes 0 in tdre
            if(clear_te)
               regs->sci[0].ssr &= ~SCI_TEND;
            return;
         }
         /*else
            assert(0);*/

      }
         return;
         break;
      case 5:
         //read only
         return;
         break;
      case 6:
         //nothing
         return;
         break;
      case 7:
         //nothing
         return;
         break;
      case 8:
         regs->sci[1].smr = data;
         return;
         break;
      case 9:
         regs->sci[1].brr = data;
         return;
         break;
      case 0xa:
         regs->sci[1].scr = data;
         return;
         break;
      case 0xb:
         regs->sci[1].tdr = data;
         regs->sci[1].tdr_written = 1;//data is present to transmit
         return;
         break;
      case 0xc:
         if (data == 0)//only 0 can be written
            regs->sci[1].ssr = data;
         return;
         break;
      case 0xd:
         //read only
         return;
         break;
      case 0xe:
      case 0xf:
         //nothing
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEEF)
   {
      //a/d
      //read only
      return;
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
   {
      switch (addr - 0x5FFFF00)
      {
      case 0:
         regs->itu.tstr = data;
         return;
      case 1:
         regs->itu.tsnc = data;
         return;
      case 2:
         regs->itu.tmdr = data;
         return;
      case 3:
         regs->itu.tfcr = data;
         return;
      }
   }
   //timer writes
   else if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         onchip_write_timer_byte(regs, addr - 0x5FFFF04, 0, data);
      else if (addr <= 0x5FFFF17)
         onchip_write_timer_byte(regs, addr - 0x5FFFF0E, 1, data);
      else if (addr <= 0x5FFFF21)
         onchip_write_timer_byte(regs, addr - 0x5FFFF18, 2, data);
      else if (addr <= 0x5FFFF2F)
         onchip_write_timer_byte(regs, addr - 0x5FFFF22, 3, data);
      else if (addr == 0x5FFFF30)
      {
         return;//unmapped
      }
      else if (addr == 0x5FFFF31)
      {
         regs->itu.tocr = data;
         return;
      }
      else
         onchip_write_timer_byte(regs, addr - 0x5FFFF32, 4, data);
      return;
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      if (addr == 0x5FFFF4e)
      {
         regs->dmac.channel[0].chcr = (regs->dmac.channel[0].chcr & 0xff) | data << 8;
         return;
      }
      if (addr == 0x5FFFF4F)
      {
         regs->dmac.channel[0].chcr = (regs->dmac.channel[0].chcr & 0xff00) | (data & 0xfd);
         return;
      }
      if (addr == 0x5FFFF5e)
      {
         regs->dmac.channel[1].chcr = (regs->dmac.channel[1].chcr & 0xff) | data << 8;
         return;
      }
      if (addr == 0x5FFFF5F)
      {
         regs->dmac.channel[1].chcr = (regs->dmac.channel[1].chcr & 0xff00) | (data & 0xfd);
         return;
      }
      if (addr == 0x5FFFF6e)
      {
         regs->dmac.channel[2].chcr = (regs->dmac.channel[2].chcr & 0xff) | data << 8;
         return;
      }
      if (addr == 0x5FFFF6F)
      {
         regs->dmac.channel[2].chcr = (regs->dmac.channel[2].chcr & 0xff00) | (data & 0xfd);
         return;
      }
      if (addr == 0x5FFFF7e)
      {
         regs->dmac.channel[3].chcr = (regs->dmac.channel[3].chcr & 0xff) | data << 8;
         return;
      }
      if (addr == 0x5FFFF7F)
      {
         regs->dmac.channel[3].chcr = (regs->dmac.channel[3].chcr & 0xff00) | (data & 0xfd);
         return;
      }
      //dmac

      //rest is inaccessible from byte writes
      return;
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF8F)
   {
      //intc

      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 1:
      case 2:
      case 3:
         return;
         break;
      //ipra
      case 4:
         regs->intc.ipra = (regs->intc.ipra & 0xff) | data << 8;
         return;
         break;
      case 5:
         regs->intc.ipra = (regs->intc.ipra & 0xff00) | data;
         return;
         break;
      //iprb
      case 6:
         regs->intc.iprb = (regs->intc.iprb & 0xff) | data << 8;
         return;
         break;
      case 7:
         regs->intc.iprb = (regs->intc.iprb & 0xff00) | data;
         return;
         break;
      //iprc
      case 8:
         regs->intc.iprc = (regs->intc.iprc & 0xff) | data << 8;
         return;
         break;
      case 9:
         regs->intc.iprc = (regs->intc.iprc & 0xff00) | data;
         return;
         break;
      //iprd
      case 0xa:
         regs->intc.iprd = (regs->intc.iprd & 0xff) | data << 8;
         return;
         break;
      case 0xb:
         regs->intc.iprd = (regs->intc.iprd & 0xff00) | data;
         return;
         break;
      //ipre
      case 0xc:
         regs->intc.ipre = (regs->intc.ipre & 0xff) | data << 8;
         return;
         break;
      case 0xd:
         regs->intc.ipre = (regs->intc.ipre & 0xff00) | data;
         return;
         break;
      //icr
      case 0xe:
         regs->intc.icr = (regs->intc.icr & 0xff) | data << 8;
         return;
         break;
      case 0xf:
         regs->intc.icr = (regs->intc.icr & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      //bar
      case 0:
         regs->ubc.bar = (regs->ubc.bar & 0x00ffffff) | data << 24;
         return;
         break;
      case 1:
         regs->ubc.bar = (regs->ubc.bar & 0xff00ffff) | data << 16;
         return;
         break;
      case 2:
         regs->ubc.bar = (regs->ubc.bar & 0xffff00ff) | data << 8;
         return;
         break;
      case 3:
         regs->ubc.bar = (regs->ubc.bar & 0xffffff00) | data;
         return;
         break;
      //bamr
      case 4:
         regs->ubc.bamr = (regs->ubc.bamr & 0x00ffffff) | data << 24;
         return;
         break;
      case 5:
         regs->ubc.bamr = (regs->ubc.bamr & 0xff00ffff) | data << 16;
         return;
         break;
      case 6:
         regs->ubc.bamr = (regs->ubc.bamr & 0xffff00ff) | data << 8;
         return;
         break;
      case 7:
         regs->ubc.bamr = (regs->ubc.bamr & 0xffffff00) | data;
         return;
         break;
      //bbr
      case 8:
         regs->ubc.bbr = (regs->ubc.bar & 0x00ff) | data << 8;
         return;
         break;
      case 9:
         regs->ubc.bbr = (regs->ubc.bar & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         regs->bsc.bcr = (regs->bsc.bcr & 0x00ff) | data << 8;
         return;
         break;
      case 1:
         regs->bsc.bcr = (regs->bsc.bcr & 0xff00) | data;
         return;
         break;
      case 2:
         regs->bsc.wcr1 = (regs->bsc.wcr1 & 0x00ff) | data << 8;
         return;
         break;
      case 3:
         regs->bsc.wcr1 = (regs->bsc.wcr1 & 0xff00) | data;
         return;
         break;
      case 4:
         regs->bsc.wcr2 = (regs->bsc.wcr2 & 0x00ff) | data << 8;
         return;
         break;
      case 5:
         regs->bsc.wcr2 = (regs->bsc.wcr2 & 0xff00) | data;
         return;
         break;
      case 6:
         regs->bsc.wcr3 = (regs->bsc.wcr3 & 0x00ff) | data << 8;
         return;
         break;
      case 7:
         regs->bsc.wcr3 = (regs->bsc.wcr3 & 0xff00) | data;
         return;
         break;
      case 8:
         regs->bsc.dcr = (regs->bsc.dcr & 0x00ff) | data << 8;
         return;
         break;
      case 9:
         regs->bsc.dcr = (regs->bsc.dcr & 0xff00) | data;
         return;
         break;
      case 0xa:
         regs->bsc.pcr = (regs->bsc.pcr & 0x00ff) | data << 8;
         return;
         break;
      case 0xb:
         regs->bsc.pcr = (regs->bsc.pcr & 0xff00) | data;
         return;
         break;
      case 0xc:
         regs->bsc.rcr = (regs->bsc.rcr & 0x00ff) | data << 8;
         return;
         break;
      case 0xd:
         regs->bsc.rcr = (regs->bsc.rcr & 0xff00) | data;
         return;
         break;
      case 0xe:
         regs->bsc.rtcsr = (regs->bsc.rtcsr & 0x00ff) | data << 8;
         return;
         break;
      case 0xf:
         regs->bsc.rtcsr = (regs->bsc.rtcsr & 0xff00) | data;
         return;
         break;
      //rtcnt
      case 0x10:
         regs->bsc.rtcnt = (regs->bsc.rtcnt & 0x00ff) | data << 8;
         return;
         break;
      case 0x11:
         regs->bsc.rtcnt = (regs->bsc.rtcnt & 0xff00) | data;
         return;
         break;
      //rtcor
      case 0x12:
         regs->bsc.rtcor = (regs->bsc.rtcor & 0x00ff) | data << 8;
         return;
         break;
      case 0x13:
         regs->bsc.rtcor = (regs->bsc.rtcor & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return;
   }
   else if (addr == 0x5FFFFbc)
   {
      regs->sbycr = data;
      return;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a / b

      switch (addr - 0x5FFFFC0)
      {
      case 0:
         regs->padr = (regs->padr & 0x00ff) | data << 8;
         return;
         break;
      case 1:
         regs->padr = (regs->padr & 0xff00) | data;
         return;
         break;
      case 2:
         regs->pbdr = (regs->pbdr & 0x00ff) | data << 8;
         return;
         break;
      case 3:
         regs->pbdr = (regs->pbdr & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc

      switch (addr - 0x5FFFFC0)
      {
      //paior
      case 4:
         regs->pfc.paior = (regs->pfc.paior & 0x00ff) | data << 8;
         return;
         break;
      case 5:
         regs->pfc.paior = (regs->pfc.paior & 0xff00) | data;
         return;
         break;
      //pbior
      case 6:
         regs->pfc.pbior = (regs->pfc.pbior & 0x00ff) | data << 8;
         return;
         break;
      case 7:
         regs->pfc.pbior = (regs->pfc.pbior & 0xff00) | data;
         return;
         break;
      //pacr1
      case 8:
         regs->pfc.pacr1 = (regs->pfc.pacr1 & 0x00ff) | data << 8;
         return;
         break;
      case 9:
         regs->pfc.pacr1 = (regs->pfc.pacr1 & 0xff00) | data;
         return;
         break;
      //pacr2
      case 0xa:
         regs->pfc.pacr2 = (regs->pfc.pacr2 & 0x00ff) | data << 8;
         return;
         break;
      case 0xb:
         regs->pfc.pacr2 = (regs->pfc.pacr2 & 0xff00) | data;
         return;
         break;
      //pbcr1
      case 0xc:
         regs->pfc.pbcr1 = (regs->pfc.pbcr1 & 0x00ff) | data << 8;
         return;
         break;
      case 0xd:
         regs->pfc.pbcr1 = (regs->pfc.pbcr1 & 0xff00) | data;
         return;
         break;
      //pbcr2
      case 0xe:
         regs->pfc.pbcr2 = (regs->pfc.pbcr2 & 0x00ff) | data << 8;
         return;
         break;
      case 0xf:
         regs->pfc.pbcr2 = (regs->pfc.pbcr2 & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFD0 && addr <= 0x5FFFFD1)
   {
      //port a / b

      switch (addr - 0x5FFFFD0)
      {
      case 0:
         regs->pcdr = (regs->pcdr & 0x00ff) | data << 8;
         return;
         break;
      case 1:
         regs->pcdr = (regs->pcdr & 0xff00) | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      regs->cascr = data;
      return;
   }
   else if (addr == 0x5FFFFEF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         regs->tpc.tpmr = data;
         return;
      case 1:
         regs->tpc.tpcr = data;
         return;
      case 2:
         regs->tpc.nderb = data;
         return;
      case 3:
         regs->tpc.ndera = data;
         return;
      case 4:
         regs->tpc.ndrb = data;
         return;
      case 5:
         regs->tpc.ndra = data;
         return;
      case 6:
         regs->tpc.ndrb = data;
         return;
      case 7:
         regs->tpc.ndra = data;
         return;
      }
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return;
   }

   //assert(0);
}

u8 onchip_sci_read_byte(struct Onchip * regs, u32 addr, int which)
{
   switch (addr)
   {
   case 0:
      return regs->sci[which].smr;
   case 1:
      return regs->sci[which].brr;
   case 2:
      return regs->sci[which].scr;
   case 3:
      return regs->sci[which].tdr;
   case 4:
      return regs->sci[which].ssr;
   case 5:
      return regs->sci[which].rdr;
   }

   //assert(0);

   return 0;
}

u8 onchip_read_byte(struct Onchip * regs, u32 addr)
{
   CDTRACE("rbreg: %08X \n", addr);
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return 0;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      if (addr >= 0x5FFFEC0 && addr <= 0x5FFFEC5)
      {
         //channel 0
         return onchip_sci_read_byte(regs, addr - 0x5FFFEC0, 0);
      }
      else if (addr >= 0x5FFFEC6 && addr <= 0x5FFFEC7)
      {
         return 0;//unmapped
      }
      else if (addr >= 0x5FFFEC8 && addr <= 0x5FFFECD)
      {
         //channel 1
         return onchip_sci_read_byte(regs, addr - 0x5FFFEC8, 1);
      }
      else if (addr >= 0x5FFFECE && addr <= 0x5FFFECF)
      {
         return 0;//unmapped
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      switch (addr - 0x5FFFEE0)
      {
      //addra
      case 0:
         return (regs->addra & 0xff00) >> 8;
         break;
      case 1:
         return regs->addra & 0xff;
         break;
      //addrb
      case 2:
         return (regs->addrb & 0xff00) >> 8;
         break;
      case 3:
         return regs->addrb & 0xff;
         break;
      //addrc
      case 4:
         return (regs->addrc & 0xff00) >> 8;
         break;
      case 5:
         return regs->addrc & 0xff;
         break;
      //addrd
      case 6:
         return (regs->addrd & 0xff00) >> 8;
         break;
      case 7:
         return regs->addrd & 0xff;
         break;
      case 8:
         return regs->adcsr;
         break;
      case 9:
         return regs->adcr;
         break;
      }
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return 0;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
   {
      //itu shared
      switch (addr - 0x5FFFF00)
      {
      case 0:
         return regs->itu.tstr;
      case 1:
         return regs->itu.tsnc;
      case 2:
         return regs->itu.tmdr;
      case 3:
         return regs->itu.tfcr;
      }
   }
   //timer
   else if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF04, 0);
      else if (addr >= 0x5FFFF0e && addr <= 0x5FFFF17)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF0E, 1);
      else if (addr >= 0x5FFFF18 && addr <= 0x5FFFF21)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF18, 2);
      else if (addr >= 0x5FFFF22 && addr <= 0x5FFFF2F)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF22, 3);
      else if (addr >= 0x5FFFF32 && addr <= 0x5FFFF3F)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF32, 4);

      if (addr == 0x05ffff30)
         return 0;//unmapped

      if (addr == 0x05ffff31)
         return regs->itu.tocr;
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF4E)
         return regs->dmac.channel[0].chcr >> 8;
      else if (addr == 0x5FFFF4F)
         return regs->dmac.channel[0].chcr & 0xff;

      if (addr == 0x5FFFF48)
         return regs->dmac.dmaor >> 8;
      else if (addr == 0x5FFFF49)
         return regs->dmac.dmaor & 0xff;

      if (addr == 0x5FFFF5E)
         return regs->dmac.channel[1].chcr >> 8;
      else if (addr == 0x5FFFF5F)
         return regs->dmac.channel[1].chcr & 0xff;

      if (addr == 0x5FFFF6E)
         return regs->dmac.channel[2].chcr >> 8;
      else if (addr == 0x5FFFF6F)
         return regs->dmac.channel[2].chcr & 0xff;

      if (addr == 0x5FFFF7E)
         return regs->dmac.channel[3].chcr >> 8;
      else if (addr == 0x5FFFF7F)
         return regs->dmac.channel[3].chcr & 0xff;

      //the rest is inacessible
      return 0;
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return 0;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 1:
      case 2:
      case 3:
         //assert(0);
         break;
         //ipra
      case 4:
         return regs->intc.ipra >> 8;
      case 5:
         return regs->intc.ipra & 0xff; 
         //iprb
      case 6:
         return regs->intc.iprb >> 8;
      case 7:
         return regs->intc.iprb & 0xff;
         //iprc
      case 8:
         return regs->intc.iprc >> 8;
      case 9:
         return regs->intc.iprc & 0xff;
         //iprd
      case 0xa:
         return regs->intc.iprd >> 8;
      case 0xb:
         return regs->intc.iprd & 0xff;
         //ipre
      case 0xc:
         return regs->intc.ipre >> 8;
      case 0xd:
         return regs->intc.ipre & 0xff;
         //icr
      case 0xe:
         return regs->intc.icr >> 8;
      case 0xf:
         return regs->intc.icr & 0xff;
      }
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
         //bar
      case 0:
         return regs->ubc.bar & 0xff000000 >> 24;
      case 1:
         return regs->ubc.bar & 0x00ff0000 >> 16;
      case 2:
         return regs->ubc.bar & 0x0000ff00 >> 8;
      case 3:
         return regs->ubc.bar & 0x000000ff >> 0;
         //bamr
      case 4:
         return regs->ubc.bamr & 0xff000000 >> 24;
      case 5:
         return regs->ubc.bamr & 0x00ff0000 >> 16;
      case 6:
         return regs->ubc.bamr & 0x0000ff00 >> 8;
      case 7:
         return regs->ubc.bamr & 0x000000ff >> 0;
         //bbr
      case 8:
         return regs->ubc.bbr >> 8;
      case 9:
         return regs->ubc.bbr & 0xff;
      }
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         return regs->bsc.bcr >> 8;
      case 1:
         return regs->bsc.bcr & 0xff;
      case 2:
         return regs->bsc.wcr1 >> 8;
      case 3:
         return regs->bsc.wcr1 & 0xff;
      case 4:
         return regs->bsc.wcr2 >> 8;
      case 5:
         return regs->bsc.wcr2 & 0xff;
      case 6:
         return regs->bsc.wcr3 >> 8;
      case 7:
         return regs->bsc.wcr3 & 0xff;
      case 8:
         return regs->bsc.dcr >> 8;
      case 9:
         return regs->bsc.dcr & 0xff;
      case 0xa:
         return regs->bsc.pcr >> 8;
      case 0xb:
         return regs->bsc.pcr & 0xff;
      case 0xc:
         return regs->bsc.rcr >> 8;
      case 0xd:
         return regs->bsc.rcr & 0xff;
      case 0xe:
         return regs->bsc.rtcsr >> 8;
      case 0xf:
         return regs->bsc.rtcsr & 0xff;
         //rtcnt
      case 0x10:
         return regs->bsc.rtcnt >> 8; 
      case 0x11:
         return regs->bsc.rtcnt & 0xff; 
         //rtcor
      case 0x12:
         return regs->bsc.rtcor >> 8;
      case 0x13:
         return regs->bsc.rtcor & 0xff;
      }
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return 0;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      return regs->sbycr;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         return regs->padr >> 8;
      case 1:
         return regs->padr & 0xff;
      case 2:
         return regs->pbdr >> 8;
      case 3:
         return regs->pbdr & 0xff;
      }
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
      //paior
      case 4:
         return regs->pfc.paior >> 8;
      case 5:
         return regs->pfc.paior & 0xff;
         //pbior
      case 6:
         return regs->pfc.pbior >> 8;
      case 7:
         return regs->pfc.pbior & 0xff;
         //pacr1
      case 8:
         return regs->pfc.pacr1 >> 8;
      case 9:
         return regs->pfc.pacr1 & 0xff;
         //pacr2
      case 0xa:
         return regs->pfc.pacr2 >> 8;
      case 0xb:
         return regs->pfc.pacr2 & 0xff;
         //pbcr1
      case 0xc:
         return regs->pfc.pbcr1 >> 8;
      case 0xd:
         return regs->pfc.pbcr1 & 0xff;
         //pbcr2
      case 0xe:
         return regs->pfc.pbcr2 >> 8;
      case 0xf:
         return regs->pfc.pbcr2 & 0xff;
      }
   }
   else if (addr == 0x5FFFFD0)
   {
      return regs->pcdr >> 8;
   }
   else if (addr == 0x5FFFFD1)
   {
      return regs->pcdr & 0xff;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return 0;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      return regs->cascr >> 8;
   }
   else if (addr == 0x5FFFFEF)
   {
      return regs->cascr & 0xff;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         return regs->tpc.tpmr;
      case 1:
         return regs->tpc.tpcr;
      case 2:
         return regs->tpc.nderb;
      case 3:
         return regs->tpc.ndera;
      case 4:
         return regs->tpc.ndrb;
      case 5:
         return regs->tpc.ndra;
      case 6:
         return regs->tpc.ndrb;
      case 7:
         return regs->tpc.ndra;
      }
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return 0;
   }

   //assert(0);

   return 0;
}

void onchip_sci_write_word(struct Onchip * regs, u32 addr, int which, u16 data)
{
   switch (addr)
   {
   case 0:
      regs->sci[which].smr = data >> 8;
      regs->sci[which].brr = data & 0xff;
      return;
   case 2:
      regs->sci[which].scr = data >> 8;
      regs->sci[which].tdr = data & 0xff;
      return;
   case 4:
      regs->sci[which].ssr = data >> 8;
      //rdr is read only
      return;
   }

   //assert(0);
}

void onchip_dmac_write_word(struct Onchip * regs, u32 addr, int which, u16 data)
{
   switch (addr)
   {
   case 0:
      regs->dmac.channel[which].sar = (regs->dmac.channel[which].sar & 0x0000ffff) | data << 16;
      return;
   case 2:
      regs->dmac.channel[which].sar = (regs->dmac.channel[which].sar & 0xffff0000) | data;
      return;
   case 4:
      regs->dmac.channel[which].dar = (regs->dmac.channel[which].dar & 0x0000ffff) | data << 16;
      return;
   case 6:
      regs->dmac.channel[which].dar = (regs->dmac.channel[which].dar & 0xffff0000) | data;
      return;
   case 8:
      //unmapped
      return;
   case 0xa:
      regs->dmac.channel[which].tcr = data;
      return;
   case 0xc:
      //unmapped
      return;
   case 0xe:
      regs->dmac.channel[which].chcr = data & 0xfffd;
      return;
   }

   //assert(0);
}

void onchip_write_word(struct Onchip * regs, u32 addr, u16 data)
{
   CDTRACE("wwreg: %08X %04X\n", addr, data);
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      if (addr >= 0x5FFFEC0 && addr <= 0x5FFFEC5)
      {
         //channel 0
         onchip_sci_write_word(regs, addr - 0x5FFFEC0, 0, data);
         return;
      }
      else if (addr >= 0x5FFFEC6 && addr <= 0x5FFFEC7)
      {
         return;//unmapped
      }
      else if (addr >= 0x5FFFEC8 && addr <= 0x5FFFECD)
      {
         //channel 1
         onchip_sci_write_word(regs, addr - 0x5FFFEC8, 1, data);
         return;
      }
      else if (addr >= 0x5FFFECE && addr <= 0x5FFFECF)
      {
         return;//unmapped
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      return;//read only
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF3F)
   {
      if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
      {
         //not 16-bit accessible
         return;
      }
      if (addr >= 0x5FFFF04 && addr <= 0x5FFFF07)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF0E && addr <= 0x5FFFF11)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF18 && addr <= 0x5FFFF1B)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF22 && addr <= 0x5FFFF25)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF32 && addr <= 0x5FFFF35)
      {
         return;//not 16-bit accessible
      }

      if (addr == 0x5FFFF30)
      {
         //unmapped
         return;
      }
      //channel 0
      if (addr == 0x5FFFF08)
      {
         regs->itu.channel[0].tcnt = data;
         return;
      }
      else if (addr == 0x5FFFF0a)
      {
         regs->itu.channel[0].gra = data;
         return;
      }
      else if (addr == 0x5FFFF0c)
      {
         regs->itu.channel[0].grb = data;
         return;
      }
      //channel 1
      else if (addr == 0x5FFFF12)
      {
         regs->itu.channel[1].tcnt = data;
         return;
      }
      else if (addr == 0x5FFFF14)
      {
         regs->itu.channel[1].gra = data;
         return;
      }
      else if (addr == 0x5FFFF16)
      {
         regs->itu.channel[1].grb = data;
         return;
      }
      //channel 2
      else if (addr == 0x5FFFF1c)
      {
         regs->itu.channel[2].tcnt = data;
         return;
      }
      else if (addr == 0x5FFFF1e)
      {
         regs->itu.channel[2].gra = data;
         return;
      }
      else if (addr == 0x5FFFF20)
      {
         regs->itu.channel[2].grb = data;
         return;
      }
      //channel 3
      else if (addr == 0x5FFFF26)
      {
         regs->itu.channel[3].tcnt = data;
         return;
      }
      else if (addr == 0x5FFFF28)
      {
         regs->itu.channel[3].gra = data;
         return;
      }
      else if (addr == 0x5FFFF2a)
      {
         regs->itu.channel[3].grb = data;
         return;
      }
      else if (addr == 0x5FFFF2c)
      {
         regs->itu.channel[3].bra = data;
         return;
      }
      else if (addr == 0x5FFFF2e)
      {
         regs->itu.channel[3].brb = data;
         return;
      }
      //channel 4
      else if (addr == 0x5FFFF36)
      {
         regs->itu.channel[4].tcnt = data;
         return;
      }
      else if (addr == 0x5FFFF38)
      {
         regs->itu.channel[4].gra = data;
         return;
      }
      else if (addr == 0x5FFFF3a)
      {
         regs->itu.channel[4].grb = data;
         return;
      }
      else if (addr == 0x5FFFF3c)
      {
         regs->itu.channel[4].bra = data;
         return;
      }
      else if (addr == 0x5FFFF3e)
      {
         regs->itu.channel[4].brb = data;
         return;
      }

      //everywhere else is inacessible

      //assert(0);
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF48)
      {
         regs->dmac.dmaor = data & 0xfff9;
         return;
      }
      if (addr >= 0x5FFFF40 && addr <= 0x5FFFF4E)
      {
         onchip_dmac_write_word(regs, addr - 0x5FFFF40, 0, data);
         return;
      }
      else if (addr >= 0x5FFFF50 && addr <= 0x5FFFF5E)
      {
         onchip_dmac_write_word(regs, addr - 0x5FFFF50, 1, data);
         return;
      }
      else if (addr >= 0x5FFFF60 && addr <= 0x5FFFF6E)
      {
         onchip_dmac_write_word(regs, addr - 0x5FFFF60, 2, data);
         return;
      }
      else if (addr >= 0x5FFFF70 && addr <= 0x5FFFF7E)
      {
         onchip_dmac_write_word(regs, addr - 0x5FFFF70, 3, data);
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 2:
         return;//unmapped
      case 4:
         regs->intc.ipra = data;
         return;
      case 6:
         regs->intc.iprb = data;
         return;
      case 8:
         regs->intc.iprc = data;
         return;
      case 0xa:
         regs->intc.iprd = data;
         return;
      case 0xc:
         regs->intc.ipre = data;
         return;
      case 0xe:
         regs->intc.icr = data;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      case 0:
         regs->ubc.bar = (regs->ubc.bar & 0xffff) | data << 16;
         return;
      case 2:
         regs->ubc.bar = (regs->ubc.bar & 0xffff0000) | data;
         return;
      case 4:
         regs->ubc.bamr = (regs->ubc.bamr & 0xffff) | data << 16;
         return;
      case 6:
         regs->ubc.bamr = (regs->ubc.bamr & 0xffff0000) | data;
         return;
      case 8:
         regs->ubc.bbr = data;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         regs->bsc.bcr = data;
         return;
      case 2:
         regs->bsc.wcr1 = data;
         return;
      case 4:
         regs->bsc.wcr2 = data;
         return;
      case 6:
         regs->bsc.wcr3 = data;
         return;
      case 8:
         regs->bsc.dcr = data;
         return;
      case 0xa:
         regs->bsc.pcr = data;
         return;
      case 0xc:
         regs->bsc.rcr = data;
         return;
      case 0xe:
         regs->bsc.rtcsr = data;
         return;
      case 0x10:
         regs->bsc.rtcnt = data;
         return;
      case 0x12:
         regs->bsc.rtcor = data;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      //what happens to the lower 8 bits?
      regs->sbycr = data >> 8;
      return;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         regs->padr = data;
         return;
      case 2:
         regs->pbdr = data;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
         //paior
      case 4:
         regs->pfc.paior = data;
         return;
      case 6:
         regs->pfc.pbior = data;
         return;
      case 8:
         regs->pfc.pacr1 = data;
         return;
      case 0xa:
         regs->pfc.pacr2 = data;
         return;
      case 0xc:
         regs->pfc.pbcr1 = data;
         return;
      case 0xe:
         regs->pfc.pbcr2 = data;
         return;
      }

      //assert(0);
   }
   else if (addr == 0x5FFFFD0)
   {
      regs->pcdr = data;
      return;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      regs->cascr = data;
      return;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         regs->tpc.tpmr = data >> 8;
         regs->tpc.tpcr = data & 0xff;
         return;
      case 2:
         regs->tpc.nderb = data >> 8;
         regs->tpc.ndera = data & 0xff;
         return;
      case 4:
         regs->tpc.ndrb = data >> 8;
         regs->tpc.ndra = data & 0xff;
         return;
      case 6:
         regs->tpc.ndrb = data >> 8;
         regs->tpc.ndra = data & 0xff;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return;
   }
   //assert(0);

   return;
}

u16 onchip_sci_read_word(struct Onchip * regs, u32 addr, int which)
{
   switch (addr)
   {
   case 0:
      return regs->sci[which].smr << 8 | regs->sci[which].brr;
   case 2:
      return regs->sci[which].scr << 8 | regs->sci[which].tdr;
   case 4:
      return regs->sci[which].ssr << 8 | regs->sci[which].rdr;
   }

   //assert(0);

   return 0;
}

u16 onchip_timer_read_word(struct Onchip * regs, u32 addr, int which_timer)
{
   switch (addr)
   {
   case 0:
   case 1:
   case 2:
   case 3:
      //byte access only
      break;
   case 4:
      return regs->itu.channel[which_timer].tcnt;
      break;
   case 6:
      return regs->itu.channel[which_timer].gra;
      break;
   case 8:
      return regs->itu.channel[which_timer].grb;
      break;
   case 0xa:
      return regs->itu.channel[which_timer].bra;
      break;
   case 0xc:
      return regs->itu.channel[which_timer].brb;
      break;
   }

   //assert(0);
   return 0;
}

u16 onchip_dmac_read_word(struct Onchip * regs, u32 addr, int which)
{
   switch (addr)
   {
   case 0:
      return regs->dmac.channel[which].sar >> 16;
   case 2:
      return regs->dmac.channel[which].sar & 0xffff;
   case 4:
      return regs->dmac.channel[which].dar >> 16;
   case 6:
      return regs->dmac.channel[which].dar & 0xffff;
   case 8:
      //unmapped
      return 0;
   case 0xa:
      return regs->dmac.channel[which].tcr;
   case 0xc:
      return 0;
   case 0xe:
      return regs->dmac.channel[which].chcr;
   }

   //assert(0);

   return 0;
}


u16 onchip_read_word(struct Onchip * regs, u32 addr)
{
   CDTRACE("rwreg: %08X %04X\n", addr);
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return 0;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      if (addr >= 0x5FFFEC0 && addr <= 0x5FFFEC5)
      {
         //channel 0
         return onchip_sci_read_word(regs, addr - 0x5FFFEC0, 0);
      }
      else if (addr >= 0x5FFFEC6 && addr <= 0x5FFFEC7)
      {
         return 0;//unmapped
      }
      else if (addr >= 0x5FFFEC8 && addr <= 0x5FFFECD)
      {
         //channel 1
         return onchip_sci_read_word(regs, addr - 0x5FFFEC8, 1);
      }
      else if (addr >= 0x5FFFECE && addr <= 0x5FFFECF)
      {
         return 0;//unmapped
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      switch (addr - 0x5FFFEE0)
      {
      case 0:
         return regs->addra;
      case 2:
         return regs->addrb;
      case 4:
         return regs->addrc;
      case 6:
         return regs->addrd;
      case 8:
         return regs->adcsr << 8 | regs->adcr;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return 0;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF3F)
   {
      if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
      {
         //not 16-bit accessible
         return 0;
      }
      if (addr >= 0x5FFFF04 && addr <= 0x5FFFF07)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF0E && addr <= 0x5FFFF11)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF18 && addr <= 0x5FFFF1B)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF22 && addr <= 0x5FFFF25)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF32 && addr <= 0x5FFFF35)
      {
         return 0;//not 16-bit accessible
      }
      if (addr == 0x5FFFF30)
      {
         //unmapped
         return 0;
      }
      if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
      {
         if (addr <= 0x5FFFF0D)
            return onchip_timer_read_word(regs, addr - 0x5FFFF04, 0);
         else if (addr <= 0x5FFFF17)
            return onchip_timer_read_word(regs, addr - 0x5FFFF0E, 1);
         else if (addr <= 0x5FFFF21)
            return onchip_timer_read_word(regs, addr - 0x5FFFF18, 2);
         else if (addr <= 0x5FFFF2F)
            return onchip_timer_read_word(regs, addr - 0x5FFFF22, 3);
         else
            return onchip_timer_read_word(regs, addr - 0x5FFFF32, 4);
      }
      //everywhere else is inacessible

      //assert(0);

   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF48)
      {
         return regs->dmac.dmaor;
      }
      if (addr >= 0x5FFFF40 && addr <= 0x5FFFF4E)
      {
         return onchip_dmac_read_word(regs, addr - 0x5FFFF40, 0);
      }
      else if (addr >= 0x5FFFF50 && addr <= 0x5FFFF5E)
      {
         return onchip_dmac_read_word(regs, addr - 0x5FFFF50, 1);
      }
      else if (addr >= 0x5FFFF60 && addr <= 0x5FFFF6E)
      {
         return onchip_dmac_read_word(regs, addr - 0x5FFFF60, 2);
      }
      else if (addr >= 0x5FFFF70 && addr <= 0x5FFFF7E)
      {
         return onchip_dmac_read_word(regs, addr - 0x5FFFF70, 3);
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return 0;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 2:
         return 0;//unmapped
      case 4:
         return regs->intc.ipra;
      case 6:
         return regs->intc.iprb;
      case 8:
         return regs->intc.iprc;
      case 0xa:
         return regs->intc.iprd;
      case 0xc:
         return regs->intc.ipre;
      case 0xe:
         return regs->intc.icr;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      case 0:
         return regs->ubc.bar >> 16;
      case 2:
         return regs->ubc.bar & 0xffff;
      case 4:
         return regs->ubc.bamr >> 16;
      case 6:
         return regs->ubc.bamr & 0xffff;
      case 8:
         return regs->ubc.bbr;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         return regs->bsc.bcr;
      case 2:
         return regs->bsc.wcr1;
      case 4:
         return regs->bsc.wcr2;
      case 6:
         return regs->bsc.wcr3;
      case 8:
         return regs->bsc.dcr;
      case 0xa:
         return regs->bsc.pcr;
      case 0xc:
         return regs->bsc.rcr;
      case 0xe:
         return regs->bsc.rtcsr;
      case 0x10:
         return regs->bsc.rtcnt;
      case 0x12:
         return regs->bsc.rtcor;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return 0;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      //what happens to the lower 8 bits?
      return  regs->sbycr << 8 | 0;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         return regs->padr;
      case 2:
         return regs->pbdr;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
         //paior
      case 4:
         return regs->pfc.paior;
      case 6:
         return regs->pfc.pbior;
      case 8:
         return regs->pfc.pacr1;
      case 0xa:
         return regs->pfc.pacr2;
      case 0xc:
         return regs->pfc.pbcr1;
      case 0xe:
         return regs->pfc.pbcr2;
      }

      //assert(0);
   }
   else if (addr == 0x5FFFFD0)
   {
      return regs->pcdr;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return 0;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      return regs->cascr;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         return regs->tpc.tpmr << 8 | regs->tpc.tpcr;
      case 2:
         return regs->tpc.nderb << 8 | regs->tpc.ndera;
      case 4:
         return regs->tpc.ndrb << 8 | regs->tpc.ndra;
      case 6:
         return regs->tpc.ndrb << 8 | regs->tpc.ndra;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return 0;
   }
   //assert(0);

   return 0;
}


void onchip_dmac_write_long(struct Onchip * regs, u32 addr, int which, u32 data)
{
   switch (addr)
   {
   case 0:
      regs->dmac.channel[which].sar = data;
      return;
   case 4:
      regs->dmac.channel[which].dar = data;
      return;
   case 8:
      //unmapped
      return;
   case 0xa:
      regs->dmac.channel[which].tcr = data;
      return;
   case 0xc:
      //unmapped?
      return;
   case 0xe:
      regs->dmac.channel[which].chcr = (data >> 16) & 0xfffd;
      return;
   }

   //assert(0);
}
void onchip_write_long(struct Onchip * regs, u32 addr, u32 data)
{
   //print_serial(0);

   CDTRACE("wlreg: %08X %08X\n", addr, data);
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      return;//inaccessible from 32
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      return;//read only
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF3F)
   {
      if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
      {
         //not 16-bit accessible
         return;
      }
      if (addr >= 0x5FFFF04 && addr <= 0x5FFFF07)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF0E && addr <= 0x5FFFF11)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF18 && addr <= 0x5FFFF1B)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF22 && addr <= 0x5FFFF25)
      {
         return;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF32 && addr <= 0x5FFFF35)
      {
         return;//not 16-bit accessible
      }

      if (addr == 0x5FFFF30)
      {
         //unmapped
         return;
      }
      //channel 0
      if (addr == 0x5FFFF08)
      {
         regs->itu.channel[0].tcnt = data >> 16;
         regs->itu.channel[0].gra = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF0a)
      {
         regs->itu.channel[0].gra = data >> 16;
         regs->itu.channel[0].grb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF0c)
      {
         //not allowed
         return;
      }
      //channel 1
      else if (addr == 0x5FFFF12)
      {
         //not allowed
         return;
      }
      else if (addr == 0x5FFFF14)
      {
         regs->itu.channel[1].gra = data >> 16;
         regs->itu.channel[1].grb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF16)
      {
         regs->itu.channel[1].grb = data >> 16;
         //the rest is ignored?
         return;
      }
      //channel 2
      else if (addr == 0x5FFFF1c)
      {
         regs->itu.channel[2].tcnt = data >> 16;
         regs->itu.channel[2].gra = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF1e)
      {
         regs->itu.channel[2].gra = data >> 16;
         regs->itu.channel[2].grb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF20)
      {
         //not allowed
         return;
      }
      //channel 3
      else if (addr == 0x5FFFF26)
      {
         //not allowed
         return;
      }
      else if (addr == 0x5FFFF28)
      {
         regs->itu.channel[3].gra = data >> 16;
         regs->itu.channel[3].grb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF2a)
      {
         regs->itu.channel[3].grb = data >> 16;
         regs->itu.channel[3].bra = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF2c)
      {
         regs->itu.channel[3].bra = data >> 16;
         regs->itu.channel[3].brb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF2e)
      {
         regs->itu.channel[3].brb = data >> 16;
         //ignored?
         return;
      }
      //channel 4
      else if (addr == 0x5FFFF36)
      {
         //not allowed
         return;
      }
      else if (addr == 0x5FFFF38)
      {
         regs->itu.channel[4].gra = data >> 16;
         regs->itu.channel[4].grb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF3a)
      {
         regs->itu.channel[4].grb = data >> 16;
         regs->itu.channel[4].bra = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF3c)
      {
         regs->itu.channel[4].bra = data >> 16;
         regs->itu.channel[4].brb = (data >> 16) & 0xffff;
         return;
      }
      else if (addr == 0x5FFFF3e)
      {
         regs->itu.channel[4].brb = data >> 16;
         //ignored?
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF48)//dmaor
      {
         regs->dmac.dmaor = data & 0xfff9;
         return;
      }
      if (addr >= 0x5FFFF40 && addr <= 0x5FFFF4E)
      {
         onchip_dmac_write_long(regs, addr - 0x5FFFF40, 0, data);
         return;
      }
      else if (addr >= 0x5FFFF50 && addr <= 0x5FFFF5E)
      {
         onchip_dmac_write_long(regs, addr - 0x5FFFF50, 1, data);
         return;
      }
      else if (addr >= 0x5FFFF60 && addr <= 0x5FFFF6E)
      {
         onchip_dmac_write_long(regs, addr - 0x5FFFF60, 2, data);
         return;
      }
      else if (addr >= 0x5FFFF70 && addr <= 0x5FFFF7E)
      {
         onchip_dmac_write_long(regs, addr - 0x5FFFF70, 3, data);
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 2:
         return;//unmapped
      case 4:
         regs->intc.ipra = data >> 16;
         regs->intc.iprb = data & 0xffff;
         return;
      case 6:
         regs->intc.iprb = data >> 16;
         regs->intc.iprc = data & 0xffff;
         return;
      case 8:
         regs->intc.iprc = data >> 16;
         regs->intc.iprd = data & 0xffff;
         return;
      case 0xa:
         regs->intc.iprd = data >> 16;
         regs->intc.ipre = data & 0xffff;
         return;
      case 0xc:
         regs->intc.ipre = data >> 16;
         regs->intc.icr = data & 0xffff;
         return;
      case 0xe:
         regs->intc.icr = data >> 16;
         //ignored?
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      case 0:
         regs->ubc.bar = data;
         return;
      case 2://apparently possible?
         return;
      case 4:
         regs->ubc.bamr = data;
         return;
      case 6://apparently possible?
         return;
      case 8:
         regs->ubc.bbr = data >> 16;
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         regs->bsc.bcr = data >> 16;
         regs->bsc.wcr1 = data & 0xffff;
         return;
      case 2:
         regs->bsc.wcr1 = data >> 16;
         regs->bsc.wcr2 = data & 0xffff;
         return;
      case 4:
         regs->bsc.wcr2 = data >> 16;
         regs->bsc.wcr3 = data & 0xffff;
         return;
      case 6:
         regs->bsc.wcr3 = data >> 16;
         regs->bsc.dcr = data & 0xffff;
         return;
      case 8:
         regs->bsc.dcr = data >> 16;
         regs->bsc.pcr = data & 0xffff;
         return;
      case 0xa:
         regs->bsc.pcr = data >> 16;
         regs->bsc.rcr = data & 0xffff;
         return;

      //write only with word transfer instructions?
      case 0xc:
         return;
      case 0xe:
         return;
      case 0x10:
         return;
      case 0x12:
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      //extra bits?
      regs->sbycr = data >> 24;
      return;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         //init routine requires pbdr to be set
      {
         u16 val = 0;

         val = ((data >> 16) & 0xffff);
         regs->padr &= ~regs->pfc.paior;
         regs->padr |= val & regs->pfc.paior;

         val = data & 0xffff;
         regs->pbdr &= ~regs->pfc.pbior;
         regs->pbdr |= val & regs->pfc.pbior;
      }
         return;
      case 2:
         regs->pbdr = data >> 16;
         regs->pfc.paior = data & 0xffff;//check this
         return;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
         //paior
      case 4:
         regs->pfc.paior = data >> 16;
         regs->pfc.pbior = data & 0xffff;
         return;
      case 6:
         regs->pfc.pbior = data >> 16;
         regs->pfc.pacr1 = data & 0xffff;
         return;
      case 8:
         regs->pfc.pacr1 = data >> 16;
         regs->pfc.pacr2 = data & 0xffff;
         return;
      case 0xa:
         regs->pfc.pacr2 = data >> 16;
         regs->pfc.pbcr1 = data & 0xffff;
         return;
      case 0xc:
         //not allowed according to tpc section but the rom writes this...
         regs->pfc.pbcr1 = data >> 16;
         regs->pfc.pbcr2 = data & 0xffff;
         return;
      case 0xe:
         regs->pfc.pbcr2 = data >> 16;
         regs->pcdr = data & 0xffff;//check this
         return;
      }

      //assert(0);
   }
   else if (addr == 0x5FFFFD0)
   {
      regs->pcdr = data >> 16;
      return;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      regs->cascr = data >> 16;
      return;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc

      //not accessible from 32 bit
      return;

      //assert(0);
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return;
   }
   //assert(0);

   return;
}

u32 onchip_dmac_read_long(struct Onchip * regs, u32 addr, int which)
{
   switch (addr)
   {
   case 0:
      return regs->dmac.channel[which].sar;
   case 4:
      return regs->dmac.channel[which].dar;
   case 8:
      //unmapped
      return 0;
   case 0xa:
      return regs->dmac.channel[which].tcr;
   case 0xc:
      //unmapped?
      return 0;
   case 0xe:
      return regs->dmac.channel[which].chcr << 16 | 0;
   }

  
   //assert(0);

   return 0;
}

u32 onchip_read_long(struct Onchip * regs, u32 addr)
{
   CDTRACE("rlreg: %08X\n", addr);
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return 0;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      return 0;//inaccessible from 32
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      return 0;//read only
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return 0;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return 0;
   }

   //itu
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF3F)
   {
      if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
      {
         //not 16-bit accessible
         return 0;
      }
      if (addr >= 0x5FFFF04 && addr <= 0x5FFFF07)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF0E && addr <= 0x5FFFF11)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF18 && addr <= 0x5FFFF1B)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF22 && addr <= 0x5FFFF25)
      {
         return 0;//not 16-bit accessible
      }
      if (addr >= 0x5FFFF32 && addr <= 0x5FFFF35)
      {
         return 0;//not 16-bit accessible
      }

      if (addr == 0x5FFFF30)
      {
         //unmapped
         return 0;
      }
      //channel 0
      if (addr == 0x5FFFF08)
      {
         return regs->itu.channel[0].tcnt << 16 | regs->itu.channel[0].gra;
      }
      else if (addr == 0x5FFFF0a)
      {
         return regs->itu.channel[0].gra << 16 | regs->itu.channel[0].grb;
      }
      else if (addr == 0x5FFFF0c)
      {
         //not allowed
         return 0;
      }
      //channel 1
      else if (addr == 0x5FFFF12)
      {
         //not allowed
         return 0;
      }
      else if (addr == 0x5FFFF14)
      {
         return regs->itu.channel[1].gra << 16 | regs->itu.channel[1].grb;
      }
      else if (addr == 0x5FFFF16)
      {
         //lower part?
         return regs->itu.channel[1].grb << 16 | 0;
      }
      //channel 2
      else if (addr == 0x5FFFF1c)
      {
         return regs->itu.channel[2].tcnt << 16 | regs->itu.channel[2].gra;
      }
      else if (addr == 0x5FFFF1e)
      {
         return regs->itu.channel[2].gra << 16 | regs->itu.channel[2].grb;
      }
      else if (addr == 0x5FFFF20)
      {
         //not allowed
         return 0;
      }
      //channel 3
      else if (addr == 0x5FFFF26)
      {
         //not allowed
         return 0;
      }
      else if (addr == 0x5FFFF28)
      {
         return regs->itu.channel[3].gra << 16 | regs->itu.channel[3].grb;
      }
      else if (addr == 0x5FFFF2a)
      {
         return regs->itu.channel[3].grb << 16 | regs->itu.channel[3].bra;
      }
      else if (addr == 0x5FFFF2c)
      {
         return regs->itu.channel[3].bra << 16 | regs->itu.channel[3].brb;
      }
      else if (addr == 0x5FFFF2e)
      {
         //lower part?
         return regs->itu.channel[3].brb << 16 | 0;
      }
      //channel 4
      else if (addr == 0x5FFFF36)
      {
         //not allowed
         return 0 ;
      }
      else if (addr == 0x5FFFF38)
      {
         return regs->itu.channel[4].gra << 16 | regs->itu.channel[4].grb;
      }
      else if (addr == 0x5FFFF3a)
      {
         return regs->itu.channel[4].grb << 16 | regs->itu.channel[4].bra;
      }
      else if (addr == 0x5FFFF3c)
      {
         return regs->itu.channel[4].bra << 16 | regs->itu.channel[4].brb;
      }
      else if (addr == 0x5FFFF3e)
      {
         //lower part?
         return regs->itu.channel[4].brb << 16 | 0;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF48)//dmaor
      {
         return regs->dmac.dmaor << 16;
      }
      if (addr >= 0x5FFFF40 && addr <= 0x5FFFF4E)
      {
         return onchip_dmac_read_long(regs, addr - 0x5FFFF40, 0);
      }
      else if (addr >= 0x5FFFF50 && addr <= 0x5FFFF5E)
      {
         return onchip_dmac_read_long(regs, addr - 0x5FFFF50, 1);
      }
      else if (addr >= 0x5FFFF60 && addr <= 0x5FFFF6E)
      {
         return onchip_dmac_read_long(regs, addr - 0x5FFFF60, 2);
      }
      else if (addr >= 0x5FFFF70 && addr <= 0x5FFFF7E)
      {
         return onchip_dmac_read_long(regs, addr - 0x5FFFF70, 3);
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return 0;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 2:
         return 0;//unmapped
      case 4:
         return regs->intc.ipra << 16 | regs->intc.iprb;
      case 6:
         return regs->intc.iprb << 16 | regs->intc.iprc;
      case 8:
         return regs->intc.iprc << 16 | regs->intc.iprd;
      case 0xa:
         return regs->intc.iprd << 16 | regs->intc.ipre;
      case 0xc:
         return regs->intc.ipre << 16 | regs->intc.icr;
      case 0xe:
         return regs->intc.icr << 16 | 0;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      case 0:
         return regs->ubc.bar;
      case 2://apparently possible?
         return 0;
      case 4:
         return regs->ubc.bamr;
      case 6://apparently possible?
         return 0;
      case 8:
         return regs->ubc.bar << 16 | 0;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         return regs->bsc.bcr << 16 | regs->bsc.wcr1;
      case 2:
         return regs->bsc.wcr1 << 16 | regs->bsc.wcr2;
      case 4:
         return regs->bsc.wcr2 << 16 | regs->bsc.wcr3;
      case 6:
         return regs->bsc.wcr3 << 16 | regs->bsc.dcr;
      case 8:
         return regs->bsc.dcr << 16 | regs->bsc.pcr;
      case 0xa:
         return regs->bsc.pcr << 16 | regs->bsc.rcr;
      case 0xc:
         return regs->bsc.rcr << 16 | regs->bsc.rtcsr;
      case 0xe:
         return regs->bsc.rtcsr << 16 | regs->bsc.rtcnt;
      case 0x10:
         return regs->bsc.rtcnt << 16 | regs->bsc.rtcor;
      case 0x12:
         return regs->bsc.rtcor << 16 | 0;
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return 0;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      //extra bits?
      return regs->sbycr << 24 | 0;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         return regs->padr << 16 | regs->pbdr;
      case 2:
         return regs->pbdr << 16 | regs->pfc.paior;//check this
      }

      //assert(0);
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
         //paior
      case 4:
         return regs->pfc.paior << 16 | regs->pfc.pbior;
      case 6:
         return regs->pfc.pbior << 16 | regs->pfc.pacr1;
      case 8:
         return regs->pfc.pacr1 << 16 | regs->pfc.pacr2;
      case 0xa:
         return regs->pfc.pacr2 << 16 | regs->pfc.pbcr1;
         //not allowed according to tpc section
      case 0xc:
         //regs->pfc.pbcr1 = data >> 16;
         //regs->pfc.pbcr2 = data & 0xffff;
         return 0;
      case 0xe:
         //regs->pfc.pbcr2 = data >> 16;
         //regs->pcdr = data & 0xffff;//check this
         return 0;
      }

      //assert(0);
   }
   else if (addr == 0x5FFFFD0)
   {
      return regs->pcdr << 16 | 0;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return 0;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      return regs->cascr << 16;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc

      //not accessible from 32 bit
      return 0;

      //assert(0);
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return 0;
   }
   //assert(0);

   return 0;
}

void memory_map_write_byte(struct Sh1* sh1, u32 addr, u8 data)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;
   
   SH1MEMLOG("memory_map_write_byte 0x%08x 0x%04x", addr, data);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      /*if (mode_pins == 2)//010
      {
         //   return sh1->rom[addr & 0xffff];
      }
      else*/
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("wbdram: %08X %02X\n", addr, data);
         T2WriteByte(SH1Dram, addr & 0x7FFFF, data);
         return;
      }
      break;
   case 2:
   case 3:
   case 4:
      //external memory space

      if (a27)
      {
         ygr_sh1_write_byte(addr, data);
         return;
      }
      break;
   case 5:
      //onchip area
      if (!a27)
      {
         onchip_write_byte(&sh1->onchip, addr, data);
         return;
      }
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
         //mpeg rom read only
         return;
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
      CDTRACE("wbram: %08X %02X\n", addr, data);
      T2WriteByte(sh1->ram, addr & 0x1fff, data);
//      update_cr_response_values(addr);
//      update_transfer_buffer();
      return;

      if (a27)
      {
         sh1->ram[addr & 0xFFF] = data;
      }
      else
      {
         //onchip peripherals
      }
      break;
   }

   //assert(0);
}

u8 memory_map_read_byte(struct Sh1* sh1, u32 addr)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;

   SH1MEMLOG("memory_map_read_byte 0x%08x", addr);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0
      CDTRACE("rbrom: %08X %02X\n", addr);
      return T2ReadByte(SH1Rom, addr & 0xffff);

  //    if (mode_pins == 2)//010
  //       return sh1->rom[addr & 0xffff];
  //    else
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("rbdram: %08X %02X\n", addr);
         return T2ReadByte(SH1Dram, addr & 0x7FFFF);
      }

      break;
   case 2:
   case 3:
   case 4:
      //external memory space

      if (a27)
      {
         return ygr_sh1_read_byte(addr);
      }
      break;
   case 5:
      //onchip area
      if (!a27)
         return onchip_read_byte(&sh1->onchip, addr);
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
         //mpeg rom
         #ifdef MPEG_CARD
         return T2ReadByte(SH1MpegRom, addr & 0x7FFFF);
         #endif
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      CDTRACE("rbram: %08X %02X\n", addr);
      return T2ReadByte(sh1->ram, addr & 0x1fff);

      //onchip ram
      if (a27)
         return sh1->ram[addr & 0xFFF];
      else
      {
         //onchip peripherals
      }
      break;
   }

   //assert(0);
   return 0;
}


u16 memory_map_read_word(struct Sh1* sh1, u32 addr)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;

   SH1MEMLOG("memory_map_read_word 0x%08x", addr);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      CDTRACE("rwrom: %08X %04X\n", addr);
      return T2ReadWord(SH1Rom, addr & 0xffff);

    //  if (mode_pins == 2)//010
    //     return sh1->rom[addr & 0xffff];
    //  else
   //   {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
 //     }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("rwdram: %08X %08X\n", addr);
         return T2ReadWord(SH1Dram, addr & 0x7FFFF);
      }

      break;
   case 2:
   case 3:
   case 4:
      //external memory space

      if (a27)
      {
         return ygr_sh1_read_word(addr);
      }
      break;
   case 5:
      //onchip area
      if (!a27)
         return onchip_read_word(&sh1->onchip, addr);
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
         #ifdef MPEG_CARD
         //mpeg rom
         return T2ReadWord(SH1MpegRom, addr & 0x7FFFF);
         #endif
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram

      CDTRACE("rwram: %08X %04X\n", addr);
      return T2ReadWord(sh1->ram, addr & 0x1fff);

      if(a27)
         return sh1->ram[addr & 0xFFF];
      else
      {
         //onchip peripherals
      }
      break;
   }

   //assert(0);
   return 0;
}

void memory_map_write_word(struct Sh1* sh1, u32 addr, u16 data)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;

   SH1MEMLOG("memory_map_write_word 0x%08x 0x%04x", addr, data);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      /*if (mode_pins == 2)//010
      {
         //sh1->rom[addr & 0xffff];
      }
      else*/
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("wwdram: %08X %04X\n", addr, data);
         T2WriteWord(SH1Dram, addr & 0x7FFFF, data);
         return;
      }

      break;
   case 2:
   case 3:
   case 4:
      //external memory space

      if (a27)
      {
         ygr_sh1_write_word(addr, data);
         return;
      }
      break;
   case 5:
      //onchip area
      if (!a27)
      {
         onchip_write_word(&sh1->onchip, addr, data);
         return;
      }
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space

         //mpeg rom read only

         return;
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram

      CDTRACE("wwram: %08X %04X\n", addr, data);
      T2WriteWord(sh1->ram, addr & 0x1fff,data);
      //update_cr_response_values(addr);

      return;

      if (a27)
      {
       //  return sh1->ram[addr & 0xFFF];
      }
      else
      {
         //onchip peripherals
      }
      break;
   }

   //assert(0);

   return;
}

u32 memory_map_read_long(struct Sh1* sh1, u32 addr)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;

   SH1MEMLOG("memory_map_read_long 0x%08x", addr);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      return T2ReadLong(SH1Rom, addr & 0xffff);

      //if (mode_pins == 2)//010
     //    return sh1->rom[addr & 0xffff];
     // else
     // {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
    //  }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("rlram: %08X\n", addr);
         return T2ReadLong(SH1Dram, addr & 0x7FFFF);
      }

      break;
   case 2:
   case 3:
   case 4:
      //external memory space

      if (a27)
      {
         return ygr_sh1_read_long(addr);
      }
      break;
   case 5:
      //onchip area
      if (!a27)
         return onchip_read_long(&sh1->onchip, addr);
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
         #ifdef MPEG_CARD
         //mpeg rom area

         return T2ReadLong(SH1MpegRom, addr & 0x7FFFF);
         #endif
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
    //  if (a27)
      //apparently both are ram?
      CDTRACE("rlram: %08X\n", addr);
      return T2ReadLong(sh1->ram, addr & 0x1fff);//sh1->ram[addr & 0x1FFF];
   //   else
    //  {
         //onchip peripherals
    //  }
      break;
   }

   return 0;
}

void memory_map_write_long(struct Sh1* sh1, u32 addr, u32 data)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;

   SH1MEMLOG("memory_map_write_long 0x%08x 0x%04x", addr, data);

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      /*if (mode_pins == 2)//010
      {
         //sh1->rom[addr & 0xffff];
      }
      else*/
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }

      if (a27)
      {
         CDTRACE("wldram: %08X %08X\n", addr, data);
         T2WriteLong(SH1Dram, addr & 0x7FFFF, data);
         return;
      }

      break;
   case 2:
   case 3:
   case 4:
      //external memory space
      
      //ygr area
      if (a27)
      {
         ygr_sh1_write_long(addr, data);
         return;
      }
      break;
   case 5:
      //onchip area
      if (!a27)
      {
         onchip_write_long(&sh1->onchip, addr, data);
         return;
      }
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
         //mpeg rom area read only

         return;
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
      CDTRACE("wlram: %08X %08X\n", addr, data);
      T2WriteLong(sh1->ram, addr & 0x1fff, data);
      //update_cr_response_values(addr);
      return;
      if (a27)
      {
         //  return sh1->ram[addr & 0xFFF];
      }
      else
      {
         //onchip peripherals
      }
      break;
   }
//triggered by yabauseut
 //  //assert(0);

   return;
}

u8 FASTCALL Sh1MemoryReadByte(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr)
{
   return memory_map_read_byte(&sh1_cxt, addr);
}
u16 FASTCALL Sh1MemoryReadWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr)
{
   return memory_map_read_word(&sh1_cxt, addr);
}
u32 FASTCALL Sh1MemoryReadLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr)
{
   return memory_map_read_long(&sh1_cxt, addr);
}
void FASTCALL Sh1MemoryWriteByte(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u8 val)
{
   memory_map_write_byte(&sh1_cxt, addr, val);
}
void FASTCALL Sh1MemoryWriteWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   memory_map_write_word(&sh1_cxt, addr, val);
}
void FASTCALL Sh1MemoryWriteLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   memory_map_write_long(&sh1_cxt, addr, val);
}


void onchip_sci_init(struct Onchip * regs)
{
   int i;
   for (i = 0; i < 2; i++)
   {
      regs->sci[i].smr = 0;
      regs->sci[i].brr = 0xff;
      regs->sci[i].scr = 0;
      regs->sci[i].tdr = 0xff;
      regs->sci[i].ssr = 0x84;
      regs->sci[i].rdr = 0;
   }
}

void onchip_ad_init(struct Onchip * regs)
{
   regs->addra = 0;
   regs->addrb = 0;
   regs->addrc = 0;
   regs->addrd = 0;
   regs->adcsr = 0;
   regs->adcr = 0x7f;
}

void onchip_itu_init(struct Onchip * regs)//init values depend on config
{
   int i;
   
   regs->itu.tstr = 0xe0;
   regs->itu.tsnc = 0xe0;
   regs->itu.tmdr = 0x80;
   regs->itu.tfcr = 0xc0;
   regs->itu.tocr = 0xff;

   for (i = 0; i < 5; i++)
   {
      regs->itu.channel[i].tcr = 0x80;
      regs->itu.channel[i].tior = 0x88;
      regs->itu.channel[i].tier = 0xf8;
      regs->itu.channel[i].tsr = 0xf8;
      regs->itu.channel[i].tcnt = 0;
      regs->itu.channel[i].gra = 0xff;
      regs->itu.channel[i].grb = 0xff;
      //not used for channels 0-2
      regs->itu.channel[i].bra = 0xff;
      regs->itu.channel[i].brb = 0xff;
   }
}

void onchip_dmac_init(struct Onchip * regs)
{
   int i;
   for (i = 0; i < 4; i++)
   {
      regs->dmac.channel[i].sar = 0;//undefined
      regs->dmac.channel[i].dar = 0;//undefined
      regs->dmac.channel[i].tcr = 0;//undefined
      regs->dmac.channel[i].chcr = 0;
   }

   regs->dmac.dmaor = 0;
}

void onchip_intc_init(struct Onchip * regs)
{
   regs->intc.ipra = 0;
   regs->intc.iprb = 0;
   regs->intc.iprc = 0;
   regs->intc.iprd = 0;
   regs->intc.ipre = 0;
}

void onchip_ubc_init(struct Onchip * regs)
{
   regs->ubc.bar = 0;
   regs->ubc.bamr = 0;
   regs->ubc.bbr = 0;
}

void onchip_bsc_init(struct Onchip * regs)
{
   regs->bsc.bcr = 0;
   regs->bsc.wcr1 = 0;
   regs->bsc.wcr2 = 0;
   regs->bsc.wcr3 = 0;
   regs->bsc.dcr = 0;
   regs->bsc.pcr = 0;
   regs->bsc.rcr = 0;
   regs->bsc.rtcsr = 0;
   regs->bsc.rtcnt = 0;
   regs->bsc.rtcor = 0;
}

void onchip_wdt_init(struct Onchip * regs)
{
   regs->wdt.tcsr = 0;
   regs->wdt.tcnt = 0;
   regs->wdt.rstcsr = 0;
}

void onchip_pfc_init(struct Onchip * regs)
{
   regs->pfc.paior = 0;
   regs->pfc.pacr1 = 0x3302;
   regs->pfc.pacr2 = 0xff95;
   regs->pfc.pbior = 0;
   regs->pfc.pbcr1 = 0;
   regs->pfc.pbcr2 = 0;
   regs->cascr = 0x5fff;
}

void onchip_tpc_init(struct Onchip * regs)
{
   regs->tpc.tpmr = 0xf0;
   regs->tpc.tpcr = 0xff;
   regs->tpc.nderb = 0;
   regs->tpc.ndera = 0;
   regs->tpc.ndrb = 0;
   regs->tpc.ndra = 0;
}

void onchip_init(struct Sh1 * sh1)
{
   onchip_sci_init(&sh1->onchip);
   onchip_ad_init(&sh1->onchip);
   onchip_itu_init(&sh1->onchip);
   onchip_dmac_init(&sh1->onchip);
   onchip_intc_init(&sh1->onchip);
   onchip_ubc_init(&sh1->onchip);
   onchip_bsc_init(&sh1->onchip);
   onchip_wdt_init(&sh1->onchip);

   sh1->onchip.sbycr = 0;

   sh1->onchip.padr = 0;
   sh1->onchip.pbdr = 0;

   onchip_pfc_init(&sh1->onchip);

   sh1->onchip.pcdr = 0;

   onchip_tpc_init(&sh1->onchip);
}

void sh1_init(struct Sh1* sh1)
{
   memset(sh1, 0, sizeof(struct Sh1));
   onchip_init(sh1);
}

void sh1_init_func()
{
   sh1_init(&sh1_cxt);

   memset(SH1Dram, 0, 0x80000);

   cdd_reset();
   #ifdef MPEG_CARD
   mpeg_card_init();
   #endif

   sh1_cxt.onchip.pbdr = 0x40c;
}


static int cycles_since = 0;

void tick_timer(int which)
{
   if (sh1_cxt.onchip.itu.tstr & (1 << which))//timer is counting
   {
      u16 old_tcnt = sh1_cxt.onchip.itu.channel[which].tcnt;

      switch (sh1_cxt.onchip.itu.channel[which].tcr & 7)
      {
      case 0:
         sh1_cxt.onchip.itu.channel[which].tcnt++; //internal clock speed
         break;
      case 1:
         if (sh1_cxt.onchip.itu.channel[which].tcnt_fraction == 2)
         {
            sh1_cxt.onchip.itu.channel[which].tcnt++; // phi/2
            sh1_cxt.onchip.itu.channel[which].tcnt_fraction = 0;
         }

         sh1_cxt.onchip.itu.channel[which].tcnt_fraction++;
         break;
      case 2:
         if (sh1_cxt.onchip.itu.channel[which].tcnt_fraction == 4)
         {
            sh1_cxt.onchip.itu.channel[which].tcnt++; // phi/4
            sh1_cxt.onchip.itu.channel[which].tcnt_fraction = 0;
         }

         sh1_cxt.onchip.itu.channel[which].tcnt_fraction++;
         break;
      case 3:
         if (sh1_cxt.onchip.itu.channel[which].tcnt_fraction == 8)
         {
            sh1_cxt.onchip.itu.channel[which].tcnt++; // phi/8
            sh1_cxt.onchip.itu.channel[which].tcnt_fraction = 0;
         }

         sh1_cxt.onchip.itu.channel[which].tcnt_fraction++;
         break;
      default:
         //assert(0);
         break;
      }

      if (sh1_cxt.onchip.itu.channel[which].tier & (1 << 2))
      {
         if (sh1_cxt.onchip.itu.channel[which].tcnt < old_tcnt)
         {
            //overflow interrupt
            TIMERTRACE("*****TCNT4 OVF interrupt*******\n");

            if (which == 4)
               SH2SendInterrupt(SH1, 98, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);

            cycles_since = 0;
         }
      }
   }

   //timer compare a
   if (sh1_cxt.onchip.itu.channel[which].gra == sh1_cxt.onchip.itu.channel[which].tcnt)
   {
      switch (sh1_cxt.onchip.itu.channel[which].tior & 7)
      {
      case 0:
         sh1_cxt.onchip.itu.channel[which].tsr |= 1;

         //cleared by gra compare match
         if(((sh1_cxt.onchip.itu.channel[which].tcr >> 5) & 3) == 1)
            sh1_cxt.onchip.itu.channel[which].tcnt = 0;

         if (sh1_cxt.onchip.itu.channel[which].tier & 1)
            SH2SendInterrupt(SH1, 96, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
         break;
      case 1:
         //output 0
         sh1_cxt.onchip.itu.channel[which].tsr &= ~(1 << 1);
         break;
      case 2:
         //output 1
         sh1_cxt.onchip.itu.channel[which].tsr |= (1 << 1);
         break;
      case 3:
         //toggles
         break;
      case 4:
         break;
      case 5:
         break;
      case 6:
         break;
      case 7:
         break;
      }
   }

   //timer compare b
   if (sh1_cxt.onchip.itu.channel[which].grb == sh1_cxt.onchip.itu.channel[which].tcnt)
   {
      switch ((sh1_cxt.onchip.itu.channel[which].tior >> 4) & 7)
      {
      case 0:
         sh1_cxt.onchip.itu.channel[which].tsr |= 2;

         if (((sh1_cxt.onchip.itu.channel[which].tcr >> 5) & 3) == 2)
            sh1_cxt.onchip.itu.channel[which].tcnt = 0;

         if (sh1_cxt.onchip.itu.channel[which].tier & 2)
            SH2SendInterrupt(SH1, 97, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
         break;
      case 1:
         //output 0
         sh1_cxt.onchip.itu.channel[which].tsr &= ~(1 << 1);
         break;
      case 2:
         //output 1
         sh1_cxt.onchip.itu.channel[which].tsr |= (1 << 1);
         break;
      case 3:
         //toggles
         break;
      case 4:
         break;
      case 5:
         break;
      case 6:
         break;
      case 7:
         break;
      }
   }
}

u16 update_tcnt_fraction(int which, s32 cycles, u8 division)
{
   int remainder = 0;

   sh1_cxt.onchip.itu.channel[which].tcnt_fraction += cycles % division;

   if (sh1_cxt.onchip.itu.channel[which].tcnt_fraction >= division)
   {
      remainder = 1;
      sh1_cxt.onchip.itu.channel[which].tcnt_fraction -= division;
   }
   return sh1_cxt.onchip.itu.channel[which].tcnt + (cycles / division) + remainder;
}

u16 update_tcnt_fast(int which, s32 cycles)
{
   
   switch (sh1_cxt.onchip.itu.channel[which].tcr & 7)
   {
   case 0:
      return sh1_cxt.onchip.itu.channel[which].tcnt + cycles; //internal clock speed
      break;
   case 1:
      return update_tcnt_fraction(which, cycles, 2);
      break;
   case 2:
      return update_tcnt_fraction(which, cycles, 4);
      break;
   case 3:
      return update_tcnt_fraction(which, cycles, 8);
      break;
   default:
      //assert(0);
      break;
   }

   return 0;
}

int check_gr_range(u16 gr, u16 old_tcnt, u16 new_tcnt)
{
   if (new_tcnt < old_tcnt)
   {
      //overflow occured

      if (gr >= old_tcnt || gr <= new_tcnt)
      {
         return 1;
      }
   }
   else
   {
      //linear range check
      if (gr >= old_tcnt && gr <= new_tcnt)
      {
         return 1;
      }
   }

   return 0;
}

void tick_timer_fast(int which, s32 cycles)
{
   u16 old_tcnt = sh1_cxt.onchip.itu.channel[which].tcnt;
   u16 new_tcnt = 0;
   int timer_is_counting = sh1_cxt.onchip.itu.tstr & (1 << which);
   int gra_match = 0, grb_match = 0;

   if (timer_is_counting)
   {
      new_tcnt = update_tcnt_fast(which, cycles);
   }

   gra_match = check_gr_range(sh1_cxt.onchip.itu.channel[which].gra, old_tcnt, new_tcnt);
   grb_match = check_gr_range(sh1_cxt.onchip.itu.channel[which].grb, old_tcnt, new_tcnt);

   if (timer_is_counting && (sh1_cxt.onchip.itu.channel[which].tier & (1 << 2)))
   {
      if (new_tcnt < old_tcnt)
      {
         //overflow interrupt
         TIMERTRACE("*****TCNT4 OVF interrupt*******\n");

         if (which == 4)
            SH2SendInterrupt(SH1, 98, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);

         cycles_since = 0;
      }
   }

   //timer compare a
   if (gra_match)
   {
      switch (sh1_cxt.onchip.itu.channel[which].tior & 7)
      {
      case 0:
         sh1_cxt.onchip.itu.channel[which].tsr |= 1;

         //cleared by gra compare match
         if (((sh1_cxt.onchip.itu.channel[which].tcr >> 5) & 3) == 1)
            new_tcnt = 0;

         if (sh1_cxt.onchip.itu.channel[which].tier & 1)
            SH2SendInterrupt(SH1, 96, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
         break;
      case 1:
         //output 0
         sh1_cxt.onchip.itu.channel[which].tsr &= ~(1 << 1);
         break;
      case 2:
         //output 1
         sh1_cxt.onchip.itu.channel[which].tsr |= (1 << 1);
         break;
      case 3:
         //toggles
         break;
      case 4:
         break;
      case 5:
         break;
      case 6:
         break;
      case 7:
         break;
      }
   }

   //timer compare b
   if (grb_match)
   {
      switch ((sh1_cxt.onchip.itu.channel[which].tior >> 4) & 7)
      {
      case 0:
         sh1_cxt.onchip.itu.channel[which].tsr |= 2;

         if (((sh1_cxt.onchip.itu.channel[which].tcr >> 5) & 3) == 2)
            new_tcnt = 0;

         if (sh1_cxt.onchip.itu.channel[which].tier & 2)
            SH2SendInterrupt(SH1, 97, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
         break;
      case 1:
         //output 0
         sh1_cxt.onchip.itu.channel[which].tsr &= ~(1 << 1);
         break;
      case 2:
         //output 1
         sh1_cxt.onchip.itu.channel[which].tsr |= (1 << 1);
         break;
      case 3:
         //toggles
         break;
      case 4:
         break;
      case 5:
         break;
      case 6:
         break;
      case 7:
         break;
      }
   }

   sh1_cxt.onchip.itu.channel[which].tcnt = new_tcnt;
}


void sh1_serial_recieve_bit(int bit, int channel);
void sh1_serial_transmit_bit(int channel, int* output_bit);

void receive_bit_from_cdd()
{
   u8 receive_from_cdd = cd_drive_get_serial_bit();
   sh1_serial_recieve_bit(receive_from_cdd, 0);
}

void transmit_bit_to_cdd()
{
   int send_to_cdd = 0;
   sh1_serial_transmit_bit(0, &send_to_cdd);
   cd_drive_set_serial_bit(send_to_cdd);
}
void cd_serial_exec();
void tick_serial(int channel)
{
   u8 bit_rate = sh1_cxt.onchip.sci[channel].brr;
   //number of cycles per bit is determined by
   //(brr+1)*4 for settings with an error rate of 0
   //sh1 uses 0x63 == 50,000 bits per second
   int cycles_per_bit = (bit_rate + 1) * 4;
   u8 clock_mode;

   static int was_printed = 0;
   if (sh1_cxt.onchip.sci[0].ssr & SCI_TEND)
   {
      if (!was_printed)
      {
         was_printed = 1;
      }
      sh1_cxt.onchip.sci[channel].serial_clock_counter = 0;
      //tend is set, no transmission
      return;
   }
   was_printed = 0;

   //if (!sh1_cxt.onchip.sci[channel].tdr_written)
   //   return;

   clock_mode = sh1_cxt.onchip.sci[channel].smr & 3;
   if (clock_mode == 3 || clock_mode == 2)//clock pin set as input
      //assert(0);

   sh1_cxt.onchip.sci[channel].serial_clock_counter++;

   if (sh1_cxt.onchip.sci[channel].serial_clock_counter > cycles_per_bit)
   {
      if (sh1_cxt.onchip.sci[channel].scr & SCI_TE &&
         sh1_cxt.onchip.sci[channel].scr & SCI_RE)
      {
         receive_bit_from_cdd();
         transmit_bit_to_cdd();
      }

      sh1_cxt.onchip.sci[channel].serial_clock_counter = 0;
   }
}

#define FAST_TIMERS

void sh1_onchip_run_cycle()
{
#ifndef FAST_TIMERS
   tick_timer(3);
   tick_timer(4);
#endif

   tick_serial(0);

   cycles_since++;
}

void sh1_onchip_run_cycles(s32 cycles)
{
   int i;

#ifdef FAST_TIMERS
   tick_timer_fast(3, cycles);
   tick_timer_fast(4, cycles);
#endif

   for (i = 0; i < cycles; i++)
      sh1_onchip_run_cycle();
}

//u16 sh1_fetch(struct Sh1* sh1)
//{
//   u32 PC = 0;
//   return sh1->rom[PC & 0xffff];
//}

//int sh1_execute_instruction(struct Sh1 * sh1)
//{
//   u16 instruction = sh1_fetch(sh1);
//   int cycles_executed = 1;
//   return cycles_executed;
//}


void test_byte_access(struct Sh1* sh1, u32 addr)
{
   u8 test_val = 0xff/*, result*/;
   memory_map_write_byte(sh1, addr, test_val);
   //result = memory_map_read_byte(sh1, addr);
}

void test_word_access(struct Sh1* sh1, u32 addr)
{
   u16 test_val = 0xffff/*, result*/;
   memory_map_write_word(sh1, addr, test_val);
   //result = memory_map_read_word(sh1, addr);
}

void test_long_access(struct Sh1* sh1, u32 addr)
{
   u32 test_val = 0xffffffff/*, result*/;
   memory_map_write_long(sh1, addr, test_val);
   //result = memory_map_read_long(sh1, addr);
}

void test_mem_map(struct Sh1* sh1)
{
	int i;

   //ygr
   memory_map_write_long(sh1, 0xa000000, 0xdeadbeef);

   //sh1 dram
   for (i = 0; i < 0x7FFFF; i += 4)
   {
      memory_map_write_long(sh1, 0x9000000 + i, 0xdeadbeef);
      memory_map_read_long(sh1, 0x9000000 + i);
   }

   for (i = 0; i < 0x7FFFF; i += 2)
   {
      memory_map_write_word(sh1, 0x9000000 + i, 0xdead);
      memory_map_read_word(sh1, 0x9000000 + i);
   }

   for (i = 0; i < 0x7FFFF; i++)
   {
      memory_map_write_byte(sh1, 0x9000000 + i, 0xde);
      memory_map_read_byte(sh1, 0x9000000 + i);
   }

   //mpeg rom
   #ifdef MPEG_CARD
   for (i = 0; i < 0x7FFFF; i += 4)
   {
      memory_map_write_long(sh1, 0xe000000 + i, 0xdeadbeef);
      memory_map_read_long(sh1, 0xe000000 + i);
   }
   #endif

   for (i = 0; i < 0x7FFFF; i += 2)
   {
      memory_map_write_word(sh1, 0xe000000 + i, 0xdead);
      memory_map_read_word(sh1, 0xe000000 + i);
   }

   for (i = 0; i < 0x7FFFF; i++)
   {
      memory_map_write_byte(sh1, 0xe000000 + i, 0xde);
      memory_map_read_byte(sh1, 0xe000000 + i);
   }
}

void sh1_exec(struct Sh1 * sh1, s32 cycles)
{
#if 0
   u32 i;

   for (i = 0x5FFFEC0; i < 0x5FFFFFF; i++)
   {
      test_byte_access(sh1, i);
   }

   for (i = 0x5FFFEC0; i < 0x5FFFFFF; i+=2)
   {
      test_word_access(sh1, i);
   }

   for (i = 0x5FFFEC0; i < 0x5FFFFFF; i += 4)
   {
      test_long_access(sh1, i);
   }

   test_mem_map(sh1);

   s32 cycles_temp = sh1->cycles_remainder - cycles;
   while (cycles_temp < 0)
   {
      int cycles = sh1_execute_instruction(sh1);
      cycles_temp += cycles;
   }
   sh1->cycles_remainder = cycles_temp;
#endif
}
#if 0
int sh1_load_rom(struct Sh1* sh1, const char* filename)
{
   size_t size = 0;
   FILE * fp = NULL;

   if (!sh1)
      return -1;

   if (!filename)
      return -1;

   fp = fopen(filename, "rb");

   if (!fp)
      return -1;

   if (!fseek(fp, 0, SEEK_END))
      return -1;

   size = ftell(fp);

   if (size != 0x8000)
      return -1;

   if (!fseek(fp, 0, SEEK_SET))
      return -1;

   size = fread(&sh1->rom, sizeof(u8), 0x8000, fp);

   if (size != 0x8000)
      return -1;

   return 1;
}
#endif
int num_output_enables = 0;

void sh1_set_output_enable_rising_edge()
{
}
//signal from the cd drive board microcontroller
//falling edge
void sh1_set_output_enable_falling_edge()
{

   if (sh1_cxt.onchip.itu.channel[3].tsr & (1 << 1))
   {
      //imfb is set, don't overflow again?
      return;
   }

   if (((sh1_cxt.onchip.itu.channel[3].tior >> 4) & 7) != 5)
   {
      //grb is not an input capture reg
      //capturing the falling edge
      return;
   }


   if (!(sh1_cxt.onchip.itu.channel[3].tier & (1 << 1)))
   {
      //imieb intterupt is disbaled
      return;
   }

   num_output_enables++;
   
   //store old grb value in brb
   sh1_cxt.onchip.itu.channel[3].brb = sh1_cxt.onchip.itu.channel[3].grb;

   //put tcnt value in grb
   sh1_cxt.onchip.itu.channel[3].grb = sh1_cxt.onchip.itu.channel[3].tcnt;

   //clear tcnt
   if(((sh1_cxt.onchip.itu.channel[3].tcr >> 5) & 3) == 2)
      sh1_cxt.onchip.itu.channel[3].tcnt = 0;

   sh1_cxt.onchip.itu.channel[3].tsr |= (1 << 1);

   //trigger an interrupt
   SH2SendInterrupt(SH1, 93, (sh1_cxt.onchip.intc.iprd >> 8) & 0xf);
}
//extern int serial_counter;
void sh1_serial_recieve_bit(int bit, int channel)
{
   sh1_cxt.onchip.sci[channel].rsr <<= 1;
   sh1_cxt.onchip.sci[channel].rsr |= bit;
   sh1_cxt.onchip.sci[channel].rsr_counter++;


   //a full byte has been received, transfer data to rdr
   if (sh1_cxt.onchip.sci[channel].rsr_counter == 8)
   {
      sh1_cxt.onchip.sci[channel].rsr_counter = 0;
      sh1_cxt.onchip.sci[channel].rdr = sh1_cxt.onchip.sci[channel].rsr;
      sh1_cxt.onchip.sci[channel].rsr = 0;
      sh1_cxt.onchip.sci[channel].ssr |= SCI_RDRF;

      //trigger interrupt
      if (sh1_cxt.onchip.sci[0].scr & SCI_RIE)//receive data full interrupt is enabled
      {
         SH2SendInterrupt(SH1, 101, sh1_cxt.onchip.intc.iprd & 0xf);
      }
   }
}



void sh1_serial_transmit_bit(int channel, int* output_bit)
{
   *output_bit = sh1_cxt.onchip.sci[channel].tsr & 1;
   if (sh1_cxt.onchip.sci[channel].tsr_counter) {
       sh1_cxt.onchip.sci[channel].tsr >>= 1;
       sh1_cxt.onchip.sci[channel].tsr_counter--;
   }

   //a full byte has been transferred, fill tsr again
   if (sh1_cxt.onchip.sci[channel].tsr_counter == 0)
   {
      if (sh1_cxt.onchip.sci[channel].tdr_written)
      {
         sh1_cxt.onchip.sci[channel].tsr = sh1_cxt.onchip.sci[channel].tdr;
         sh1_cxt.onchip.sci[channel].tsr_counter = 8;
         sh1_cxt.onchip.sci[channel].tdr_written = 0;
         sh1_cxt.onchip.sci[channel].ssr |= SCI_TDRE;
      }
      else
      {
         //end of transmission
         sh1_cxt.onchip.sci[channel].ssr |= SCI_TEND;
      }

      if (sh1_cxt.onchip.sci[0].scr & SCI_TIE)
      {
         //assert(0);
       //  SH2SendInterrupt(SH1, 101, sh1_cxt.onchip.intc.iprd & 0xf);
      }
   }
}

//pb2
void sh1_set_start(int state)
{

   if (state)
      sh1_cxt.onchip.pbdr &= ~0x04;
   else
      sh1_cxt.onchip.pbdr |= 0x04;
}

void tick_dma(int which)
{
   u8 destination_mode, source_mode, is_word_size;
   s8 source_increment, dest_increment;
   u8 mode = (sh1_cxt.onchip.dmac.channel[which].chcr >> 8) & 0xf;
  
   if ((sh1_cxt.onchip.dmac.dmaor & 7) != 1 || //ae, nmif == 0, dme == 1
      (sh1_cxt.onchip.dmac.channel[which].chcr & 3) != 1) //te == 0, de == 1
      return;

   //not dreq based dma
   if (!(mode == 2 || mode == 3))//put / get sector data uses mode 3 
      return;

   if (!ygr_dreq_asserted())
      return;

   destination_mode = sh1_cxt.onchip.dmac.channel[which].chcr >> 14;
   source_mode = (sh1_cxt.onchip.dmac.channel[which].chcr >> 12) & 3;
   is_word_size = (sh1_cxt.onchip.dmac.channel[which].chcr >> 3) & 1;//otherwise byte size

   source_increment = 0;
   dest_increment = 0;

   //update addresses
   //mode 0 means fixed
   if (source_mode == 1)
      source_increment = 2;
   else if (source_mode == 2)
      source_increment = -2;

   if (destination_mode == 1)
      dest_increment = 2;
   else if (destination_mode == 2)
      dest_increment = -2;

   if (is_word_size)
   {
      u16 src_val = memory_map_read_word(&sh1_cxt, sh1_cxt.onchip.dmac.channel[which].sar);
      memory_map_write_word(&sh1_cxt, sh1_cxt.onchip.dmac.channel[which].dar, src_val);
   }
   else
   {
      u8 src_val = memory_map_read_byte(&sh1_cxt, sh1_cxt.onchip.dmac.channel[which].sar);
      memory_map_write_byte(&sh1_cxt, sh1_cxt.onchip.dmac.channel[which].dar, src_val);
   }

   sh1_cxt.onchip.dmac.channel[which].sar += source_increment;
   sh1_cxt.onchip.dmac.channel[which].dar += dest_increment;
   sh1_cxt.onchip.dmac.channel[which].tcr--;

   if (sh1_cxt.onchip.dmac.channel[which].tcr == 0)
   {
      sh1_cxt.onchip.dmac.channel[which].is_active = 0;
      sh1_cxt.onchip.dmac.channel[which].chcr |= 2;//te dma has ended normally

      if (sh1_cxt.onchip.dmac.channel[which].chcr & 4)
      {
         //just do dma1 for now
         SH2SendInterrupt(SH1, 74, (sh1_cxt.onchip.intc.iprc >> 12) & 0xf);
      }
   }
}

#ifdef MPEG_CARD
int print_mpeg_jump = 0;
#endif

void sh1_dma_exec(s32 cycles)
{
	int i;
#if 0 //re-enable when mpeg is fixed
   if (SH1->regs.PC == 0xf914)
      print_mpeg_jump = 1;

   if (SH1->regs.PC == 0xf91c)
      print_mpeg_jump = 0;

   if (print_mpeg_jump && (SH1->regs.PC == 0xf926))
      CDLOG("MPEG Jump to %08x\n", SH1->regs.R[0]);

   //pass mpeg card presence test
   if (SH1->regs.PC == 0x4c6)
   {
      mpeg_card_set_all_irqs();
   }

   //sh1_cxt.onchip.dmac.channel[2].chcr & 2 must not be set at 0xbd1c to pass test

   //must trigger imia1 to pass test?
   if (SH1->regs.PC == 0xbd38)
      sh1_cxt.onchip.dmac.channel[2].chcr |= 2;

   if (SH1->regs.PC == 0xbece)
      sh1_cxt.onchip.dmac.channel[3].chcr |= 2;
#endif
   for (i = 0; i < cycles; i++)
      tick_dma(1);
}

void sh1_dma_init(int which)
{
   //de, dme, mnif, ae, te must all be zero to start a dma
   //but just check de and dme for now
   if (sh1_cxt.onchip.dmac.dmaor & 1 && //dma enabled on all channels
      sh1_cxt.onchip.dmac.channel[which].chcr & 1)//dma enabled on this channel
   {
      sh1_cxt.onchip.dmac.channel[which].is_active = 1;
      sh1_cxt.onchip.dmac.channel[which].chcr &= 0xfffd;//clear te bit to indicate dma is active
   }
}


void sh1_dreq_asserted(int which)
{
   if (!sh1_cxt.onchip.dmac.channel[which].is_active)
      sh1_dma_init(which);

   tick_dma(which);
}

void sh1_assert_tioca(int which)
{
   //capture falling edge of input
   if ((sh1_cxt.onchip.itu.channel[which].tior & 7) == 5)
   {
      //store tcnt in gra
      sh1_cxt.onchip.itu.channel[which].gra = sh1_cxt.onchip.itu.channel[which].tcnt;
      //set imfa
      sh1_cxt.onchip.itu.channel[which].tsr |= 1;

      if (sh1_cxt.onchip.itu.channel[which].tier & 1)
      {
         //imia
         if(which == 0)
            SH2SendInterrupt(SH1, 80, (sh1_cxt.onchip.intc.iprc >> 4) & 0xf);
         else if (which == 1)
            SH2SendInterrupt(SH1, 84, (sh1_cxt.onchip.intc.iprc >> 0) & 0xf);
         else if (which == 2)
            SH2SendInterrupt(SH1, 88, (sh1_cxt.onchip.intc.iprd >> 12) & 0xf);
         else if (which == 3)
            SH2SendInterrupt(SH1, 92, (sh1_cxt.onchip.intc.iprd >> 8) & 0xf);
         else if (which == 4)
            SH2SendInterrupt(SH1, 96, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
      }

   }
}

void sh1_assert_tiocb(int which)
{
   //capture falling edge of input
   if (((sh1_cxt.onchip.itu.channel[which].tior >> 4) & 7) == 5)
   {
      //store tcnt in gra
      sh1_cxt.onchip.itu.channel[which].grb = sh1_cxt.onchip.itu.channel[which].tcnt;
      //set imfa
      sh1_cxt.onchip.itu.channel[which].tsr |= 2;

      if (sh1_cxt.onchip.itu.channel[which].tier & 2)
      {
         //imib
         if (which == 0)
            SH2SendInterrupt(SH1, 81, (sh1_cxt.onchip.intc.iprc >> 4) & 0xf);
         else if (which == 1)
            SH2SendInterrupt(SH1, 85, (sh1_cxt.onchip.intc.iprc >> 0) & 0xf);
         else if (which == 2)
            SH2SendInterrupt(SH1, 89, (sh1_cxt.onchip.intc.iprd >> 12) & 0xf);
         else if (which == 3)
            SH2SendInterrupt(SH1, 93, (sh1_cxt.onchip.intc.iprd >> 8) & 0xf);
         else if (which == 4)
            SH2SendInterrupt(SH1, 97, (sh1_cxt.onchip.intc.iprd >> 4) & 0xf);
      }
   }
}
