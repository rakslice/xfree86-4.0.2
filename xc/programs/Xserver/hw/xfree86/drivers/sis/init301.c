/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/init301.c,v 1.3 2000/12/02 01:16:16 dawes Exp $ */

#include "xf86.h"
#include "xf86PciInfo.h"

#include "sis.h"
#include "sis_regs.h"

#include "init301.h"

BOOLEAN SetCRT2Group(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo, ScrnInfoPtr pScrn)
{
   USHORT temp;

   SetFlag=SetFlag|ProgrammingCRT2;
   SearchModeID(ROMAddr,ModeNo);
   temp=GetRatePtrCRT2(ROMAddr,ModeNo);
   if(((temp&0x02)==0) && ((VBInfo&CRT2DisplayFlag)==0))
     return(FALSE);
   SaveCRT2Info(ModeNo);
   DisableBridge(BaseAddr);   
   UnLockCRT2(BaseAddr);
   SetDefCRT2ExtRegs(BaseAddr);    
   SetCRT2ModeRegs(BaseAddr,ModeNo);   
   if(IF_DEF_LVDS==0) {
    if(VBInfo&CRT2DisplayFlag){
      LockCRT2(BaseAddr);
      return 0;
    }
   }
   GetCRT2Data(ROMAddr,ModeNo); 
   if(IF_DEF_LVDS==1) {
     GetLVDSDesData(ROMAddr,ModeNo);  
  }
   SetGroup1(BaseAddr,ROMAddr,ModeNo,pScrn);    
   if(IF_DEF_LVDS==0) {
     SetGroup2(BaseAddr,ROMAddr,ModeNo);  
     SetGroup3(BaseAddr,ROMAddr);  
     SetGroup4(BaseAddr,ROMAddr,ModeNo);
     SetGroup5(BaseAddr,ROMAddr);   
   }
   else {
     if(IF_DEF_CH7005==1) SetCHTVReg(ROMAddr,ModeNo);   
     ModCRT1CRTC(ROMAddr,ModeNo);   
     SetCRT2ECLK(ROMAddr,ModeNo);
   }
   EnableCRT2();
   EnableBridge(BaseAddr);
   if(IF_DEF_LVDS==0) {
  /*   SetLockRegs();  */
   }
   LockCRT2(BaseAddr);
   return 1;
}

VOID SetDefCRT2ExtRegs(USHORT BaseAddr)
{
  USHORT  Part1Port,Part2Port,Part4Port;
  USHORT  temp;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  Part2Port=BaseAddr+IND_SIS_CRT2_PORT_10;
  Part4Port=BaseAddr+IND_SIS_CRT2_PORT_14;
  if(IF_DEF_LVDS==0) {
    SetReg1(Part1Port,0x02,0x40);
    SetReg1(Part4Port,0x10,0x80);
    temp=(UCHAR)GetReg1(P3c4,0x16);
    temp=temp&0xC3;
    SetReg1(P3d4,0x35,temp);
  }
  else {
    SetReg1(P3d4,0x32,0x02);
    SetReg1(Part1Port,0x02,0x00);
  }
}

USHORT GetRatePtrCRT2(ULONG ROMAddr, USHORT ModeNo)
{                           /* return bit0=>0:standard mode 1:extended mode */
  SHORT  index;             /*        bit1=>0:crt2 no support this mode     */
  USHORT temp;             /*              1:crt2 support this mode        */
  USHORT ulRefIndexLength;
  USHORT temp1,modeflag1,Flag;
  SHORT  LCDRefreshIndex[2]={0x03,0x01};

  if(IF_DEF_CH7005==1) {
    if(VBInfo&SetCRT2ToTV) {
      modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
      if(modeflag1&HalfDCLK) return(0);
    }
  }
  if(ModeNo<0x14) return(0);       /* Mode No <= 13h then return              */

  index=GetReg1(P3d4,0x33);        /* Get 3d4 CRTC33                          */
  index=index>>SelectCRT2Rate;     /* For CRT2,cl=SelectCRT2Rate=4, shr ah,cl */
  index=index&0x0F;                /* Frame rate index                        */
  if(index!=0) index--;

  if(SetFlag&ProgrammingCRT2){
    Flag=1;
    if(IF_DEF_CH7005==1) {
      if(VBInfo&SetCRT2ToTV) {
        index=0;
        Flag=0;
      }
    }
    if((Flag)&&(VBInfo&SetCRT2ToLCD)){
      if(IF_DEF_LVDS==0) {
        temp=LCDResInfo;
        temp1=LCDRefreshIndex[temp];
        if(index>temp1){
          index=temp1;
        }
      }
      else {
        index=0;
      }
    }
  }

  REFIndex=*((USHORT *)(ROMAddr+ModeIDOffset+0x04));   /* si+Ext_point */

  ulRefIndexLength =Ext2StructSize;
  do {
    temp=*((USHORT *)(ROMAddr+REFIndex));              /* di => REFIndex */
    if(temp==0xFFFF) break;
    temp=temp&ModeInfoFlag;
    if(temp<ModeType) break;

    REFIndex=REFIndex+ulRefIndexLength;                /* rate size */
    index--;
    if(index<0){
      if(!(VBInfo&SetCRT2ToRAMDAC)){
        if(VBInfo&SetInSlaveMode){
          temp1=*((USHORT *)(ROMAddr+REFIndex+0-Ext2StructSize));
          if(temp1&InterlaceMode){
            index=0;
          }
        }
      }
    }
  } while(index>=0);
  REFIndex=REFIndex-ulRefIndexLength;                    /* rate size */

  if((SetFlag&ProgrammingCRT2)){
    temp1=AjustCRT2Rate(ROMAddr);
  }else{
    temp1=0;
  }

  return(0x01|(temp1<<1));
}

BOOLEAN AjustCRT2Rate(ULONG ROMAddr)
{
  USHORT tempax,tempbx=0,temp,resinfo;
  USHORT tempextinfoflag,Flag;
  tempax=0;
  if(IF_DEF_LVDS==0) {
    if(VBInfo&SetCRT2ToRAMDAC){
      tempax=tempax|SupportRAMDAC2;
    }
    if(VBInfo&SetCRT2ToLCD){
      tempax=tempax|SupportLCD;
      if(LCDResInfo!=Panel1280x1024){
        temp=*((UCHAR *)(ROMAddr+ModeIDOffset+0x09)); /* si+Ext_ResInfo */
        if(temp>=9) tempax=0;
      }
    }
/* ynlai begin */
    if(IF_DEF_HiVision==1) {
      tempax=tempax|SupportHiVisionTV;
      if(VBInfo&SetInSlaveMode){
        resinfo=*((UCHAR *)(ROMAddr+ModeIDOffset+0x09)); /*si+Ext_ResInfo  */
        if(resinfo==4) return(0);
        if(resinfo==3) {
          if(SetFlag&TVSimuMode) return(0);
        }
        if(resinfo>7) return(0);
      }
    }
    else {
      if(VBInfo&(SetCRT2ToAVIDEO|SetCRT2ToSVIDEO|SetCRT2ToSCART)){
        tempax=tempax|SupportTV;
        if(!(VBInfo&SetPALTV)){
          tempextinfoflag=*((USHORT *)(ROMAddr+REFIndex+0x0)); /* di+Ext_InfoFlag */
          if(tempextinfoflag&NoSupportSimuTV){
            if(VBInfo&SetInSlaveMode){
              if(!(VBInfo&SetNotSimuMode)){
                return 0;
              }
            }
          }
        }
      }
    }
/* ynlai end   */
    tempbx=*((USHORT *)(ROMAddr+ModeIDOffset+0x04));   /* si+Ext_point */
  }
  else {  /* for LVDS */
    Flag=1;
    if(IF_DEF_CH7005==1) {
      if(VBInfo&SetCRT2ToTV) {
        tempax=tempax|SupportCHTV;
        Flag=0;
      }
    }                    
    tempbx=*((USHORT *)(ROMAddr+ModeIDOffset+0x04));   
    if((Flag)&&(VBInfo&SetCRT2ToLCD)){
      tempax=tempax|SupportLCD;
      temp=*((UCHAR *)(ROMAddr+ModeIDOffset+0x09)); /*si+Ext_ResInfo  */
      if(temp>0x08)   return(0);       /*1024x768  */
      if(LCDResInfo<Panel1024x768){
        if(temp>0x07) return(0);       /*800x600  */
        if(temp==0x04) return(0);      /*512x384  */
      }
    }
  }
  for(;REFIndex>tempbx;REFIndex-=Ext2StructSize){
     tempextinfoflag=*((USHORT *)(ROMAddr+REFIndex+0x0)); /* di+Ext_InfoFlag */
     if(tempextinfoflag&tempax){
       return 1;
     }
  }
  for(REFIndex=tempbx;;REFIndex+=Ext2StructSize){
     tempextinfoflag=*((USHORT *)(ROMAddr+REFIndex+0x0)); /* di+Ext_InfoFlag */
     if(tempextinfoflag==0x0FFFF){
       return 0;
     }
     if(tempextinfoflag&tempax){
       return 1;
     }
  }
  return(FALSE);
}

VOID SaveCRT2Info(USHORT ModeNo)
{
  USHORT temp1,temp2,temp3;
  temp1=(VBInfo&SetInSlaveMode)>>8;
  temp2=~(SetInSlaveMode>>8);
  temp3=(UCHAR)GetReg1(P3d4,0x31);
  temp3=((temp3&temp2)|temp1);
  SetReg1(P3d4,0x31,(USHORT)temp3);
  temp3=(UCHAR)GetReg1(P3d4,0x35);
  temp3=temp3&0xF3;
  SetReg1(P3d4,0x35,(USHORT)temp3);
}

VOID DisableLockRegs(){
  UCHAR temp3;
  temp3=(UCHAR)GetReg1(P3c4,0x32);
  temp3=temp3&0xDF;
  SetReg1(P3c4,0x32,(USHORT)temp3);
}

VOID DisableCRT2(){
  UCHAR temp3;
  temp3=(UCHAR)GetReg1(P3c4,0x1E);
  temp3=temp3&0xDF;
  SetReg1(P3c4,0x1E,(USHORT)temp3);
}

VOID DisableBridge(USHORT  BaseAddr)
{
  UCHAR   temp3,part2_02,part2_05;
  USHORT  Part2Port,Part1Port=0;
  Part2Port=BaseAddr+IND_SIS_CRT2_PORT_10;

  if(IF_DEF_LVDS==0) {
    part2_02=(UCHAR)GetReg1(Part2Port,0x02);
    part2_05=(UCHAR)GetReg1(Part2Port,0x05);
/*    if(!WaitVBRetrace(BaseAddr))  */        /* return 0:no enable read dram */
    {
      LongWait();
      DisableLockRegs();
    }
    SetReg1(Part2Port,0x02,0x38);
    SetReg1(Part2Port,0x05,0xFF);
    temp3=(UCHAR)GetReg1(Part2Port,0x00);
    temp3=temp3&0xDF;
    SetReg1(Part2Port,0x00,(USHORT)temp3);
    SetReg1(Part2Port,0x02,part2_02);
    SetReg1(Part2Port,0x05,part2_05);
    DisableCRT2();
  }
  else {
    DisableLockRegs();
    DisableCRT2();
    UnLockCRT2(BaseAddr);
    SetRegANDOR(Part1Port,0x02,0xFF,0x40); /*et Part1Port ,index 2, D6=1,  */
  }
}

VOID GetCRT2Data(ULONG ROMAddr,USHORT ModeNo)
{
  if(IF_DEF_LVDS==0){ /*301  */
    GetCRT2Data301(ROMAddr,ModeNo);
    return;
  }else{ /*LVDS */
    GetCRT2DataLVDS(ROMAddr,ModeNo);
    return;
  }
}

VOID GetCRT2DataLVDS(ULONG ROMAddr,USHORT ModeNo)
{
   USHORT tempax,tempbx,OldREFIndex;  

   OldREFIndex=(USHORT)REFIndex;         /*push di  */
   GetResInfo(ROMAddr,ModeNo);
   GetCRT2Ptr(ROMAddr,ModeNo);

   tempax=*((USHORT *)(ROMAddr+REFIndex));
   tempax=tempax&0x0FFF;
   VGAHT=tempax;

   tempax=*((USHORT *)(ROMAddr+REFIndex+1));
   tempax=tempax>>4;
   tempax=tempax&0x07FF;
   VGAVT=tempax;
/*   VGAVT=518;   */

   tempax=*((USHORT *)(ROMAddr+REFIndex+3));
   tempax=tempax&0x0FFF;
   tempbx=*((USHORT *)(ROMAddr+REFIndex+4));
   tempbx=tempbx>>4;
   tempbx=tempbx&0x07FF;

   HT=tempax;
   VT=tempbx;

   if(IF_DEF_TRUMPION==0){
     if(VBInfo&SetCRT2ToLCD){
       if(!(LCDInfo&LCDNonExpanding)){
         if(LCDResInfo==Panel800x600){
           tempax=800;
           tempbx=600;
         }else if(LCDResInfo==Panel1024x768){ 
           tempax=1024;
           tempbx=768;
         }else{
           tempax=1280;
           tempbx=1024;
         } 
         HDE=tempax;
         VDE=tempbx;
       }
     }
   }
   REFIndex=OldREFIndex;         /*pop di  */
   return;
}

VOID GetCRT2Data301(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempax,tempbx,modeflag1,OldREFIndex;
  USHORT tempal,tempah,tempbl,resinfo;

  OldREFIndex=REFIndex;         /* push di */
  RVBHRS=50;NewFlickerMode=0;RY1COE=0;
  RY2COE=0;RY3COE=0;RY4COE=0;

  GetResInfo(ROMAddr,ModeNo);
  if(VBInfo&SetCRT2ToRAMDAC){
    GetRAMDAC2DATA(ROMAddr,ModeNo);
    REFIndex=OldREFIndex;         /* pop di */
    return;
  }
  GetCRT2Ptr(ROMAddr,ModeNo);

  tempal=*((UCHAR *)(ROMAddr+REFIndex));
  tempah=*((UCHAR *)(ROMAddr+REFIndex+4));
  tempax=tempal|(((tempah<<8)>>7)&0xFF00);
  RVBHCMAX=tempax;

  tempal=*((UCHAR *)(ROMAddr+REFIndex+1));
  RVBHCFACT=tempal;

  tempax=*((USHORT *)(ROMAddr+REFIndex+2));
  VGAHT=(tempax&0x0FFF);

  tempax=*((USHORT *)(ROMAddr+REFIndex+3));
  VGAVT=((tempax>>4)&0x07FF);

  tempax=*((USHORT *)(ROMAddr+REFIndex+5));
  tempax=(tempax&0x0FFF);
  tempbx=*((USHORT *)(ROMAddr+REFIndex+6));
  tempbx=((tempbx>>4)&0x07FF);
  tempbl=tempbx&0x00FF;

  if(VBInfo&SetCRT2ToTV){
    tempax=*((USHORT *)(ROMAddr+REFIndex+5));
    tempax=(tempax&0x0FFF);
    HDE=tempax;
    tempax=*((USHORT *)(ROMAddr+REFIndex+6));
    tempax=((tempax>>4)&0x07FF);
    VDE=tempax;
    tempax=*((USHORT *)(ROMAddr+REFIndex+8));
    tempbl=(tempax>>8);
    tempax=tempax&0x0FFF;
    modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
    if(modeflag1&HalfDCLK){
      tempax=*((USHORT *)(ROMAddr+REFIndex+10));
    }
    RVBHRS=tempax;
/*  ynlai  begin */
    tempbl=tempbl&0x80;
    if(IF_DEF_HiVision==1) {
       resinfo=*((UCHAR *)(ROMAddr+ModeIDOffset+0x09)); /* si+Ext_ResInfo */
       if(resinfo==8) tempbl=0x40;
       else if(resinfo==9) tempbl=0x40;
       else if(resinfo==10) tempbl=0x40;
    }
/*  ynlai  end */
    NewFlickerMode=tempbl;

/* ynlai begin */
    if(IF_DEF_HiVision==1) {
      if(VGAVDE==350) SetFlag=SetFlag|TVSimuMode;
      tempax=ExtHiTVHT;
      tempbx=ExtHiTVVT;
      if(VBInfo&SetInSlaveMode) {
        if(SetFlag&TVSimuMode) {
          tempax=StHiTVHT;
          tempbx=StHiTVVT;
          modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
          if(!(modeflag1&Charx8Dot)){
             tempax=StHiTextTVHT;
             tempbx=StHiTextTVVT;
          }
        }
      }
    }
    else {
      tempax=*((USHORT *)(ROMAddr+REFIndex+12));
      RY1COE=(tempax&0x00FF);
      RY2COE=((tempax&0xFF00)>>8);
      tempax=*((USHORT *)(ROMAddr+REFIndex+14));
      RY3COE=(tempax&0x00FF);
      RY4COE=((tempax&0xFF00)>>8);
      if(!(VBInfo&SetPALTV)){
        tempax=NTSCHT;
        tempbx=NTSCVT;
      }else{
        tempax=PALHT;
        tempbx=PALVT;
      }
    }
/* ynlai end */
  }
  HT=tempax;
  VT=tempbx;
  if(!(VBInfo&SetCRT2ToLCD)){
    REFIndex=OldREFIndex;         /* pop di */
    return;
  }

  tempax=1024;
  if(VGAVDE==350){      /* cx->VGAVDE */
    tempbx=560;
  }else if(VGAVDE==400){
    tempbx=640;
  }else{
    tempbx=768;
  }

  if(LCDResInfo==Panel1280x1024){
    tempax=1280;
    if(VGAVDE==360){
      tempbx=768;
    }else if(VGAVDE==375){
      tempbx=800;
    }else if(VGAVDE==405){
      tempbx=864;
    }else{
      tempbx=1024;
    }
  }

  HDE=tempax;
  VDE=tempbx;
  REFIndex=OldREFIndex;         /* pop di */
  return;
}

VOID GetResInfo(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT temp,xres,yres,modeflag1;
  if(ModeNo<=0x13){
    temp=(USHORT)*((UCHAR *)(ROMAddr+ModeIDOffset+0x05));   /* si+St_ResInfo */
    xres=StResInfo[temp][0];
    yres=StResInfo[temp][1];
  }else{
    temp=(USHORT)*((UCHAR *)(ROMAddr+ModeIDOffset+0x09));   /* si+Ext_ResInfo */
    xres=ModeResInfo[temp][0];  /* xres->ax */
    yres=ModeResInfo[temp][1];  /* yres->bx */
    modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));    /* si+St_ModeFlag */
    if(modeflag1&HalfDCLK){ xres=xres*2;}
    if(modeflag1&DoubleScanMode){yres=yres*2;}
  }
  if(!(LCDResInfo==Panel1024x768)){
    if(yres==400) yres=405;
    if(yres==350) yres=360;
    if(SetFlag&LCDVESATiming){
      if(yres==360) yres=375;
    }
  }
  if(IF_DEF_LVDS==1) {
    if(xres==720) xres=640;
  }
  VGAHDE=xres;
  HDE=xres;
  VGAVDE=yres;
  VDE=yres;
}

VOID GetLVDSDesData(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT old_REFIndex,tempax;

  old_REFIndex=(USHORT)REFIndex; /*push di  */
  REFIndex=GetLVDSDesPtr(ROMAddr,ModeNo);

  tempax=*((USHORT *)(ROMAddr+REFIndex));
  tempax=tempax&0x0FFF;
  LCDHDES=tempax;

  if(LCDInfo&LCDNonExpanding){ /*hw walk-a-round  */
    if(LCDResInfo>=Panel1024x768){
      if(ModeNo<=0x13){
        LCDHDES=320;
      }
    }
  }

  tempax=*((USHORT *)(ROMAddr+REFIndex+1));
  tempax=tempax>>4;
  tempax=tempax&0x07FF;
  LCDVDES=tempax;

  REFIndex=old_REFIndex; /*pop di  */
  return;  
}


VOID GetRAMDAC2DATA(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempax,tempbx,tempbh,modeflag1,t1=0,t2;
  RVBHCMAX=1;RVBHCFACT=1;
  if(ModeNo<=0x13){
    tempax=*((UCHAR *)(ROMAddr+REFIndex+10));
    tempbx=*((USHORT *)(ROMAddr+REFIndex+16));
  }else{
    t1=*((UCHAR *)(ROMAddr+REFIndex+0x2));      /* Ext_CRT1CRTC=2 */
    t1=t1*CRT1Len;
    REFIndex=*((USHORT *)(ROMAddr+0x204));      /* Get CRT1Table */
    REFIndex=REFIndex+t1;
    t1=*((UCHAR *)(ROMAddr+REFIndex+0));
    t2=*((UCHAR *)(ROMAddr+REFIndex+14));
    tempax=(t1&0xFF)|((t2&0x03)<<8);
    tempbx=*((USHORT *)(ROMAddr+REFIndex+6));
    t1=*((UCHAR *)(ROMAddr+REFIndex+13));
    t1=(t1&0x01)<<2;
  }

  tempbh=tempbx>>8;
  tempbh=((tempbh&0x20)>>4)|(tempbh&0x01);
  tempbh=tempbh|t1;
  tempbx=(tempbx&0xFF)|(tempbh<<8);
  tempax=tempax+5;
  modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
  if(modeflag1&Charx8Dot){
    tempax=tempax*8;
  }else{
    tempax=tempax*9;
  }

  VGAHT=tempax;
  HT=tempax;
  tempbx++;
  VGAVT=tempbx;
  VT=tempbx;

}

VOID GetCRT2Ptr(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempcl,tempbx,tempal,tempax,CRT2PtrData=0;
  USHORT Flag;

  if(IF_DEF_LVDS==0) {
    if(VBInfo&SetCRT2ToLCD){              /* LCD */
      tempbx=LCDResInfo;
      tempcl=LCDDataLen;
      tempbx=tempbx-Panel1024x768;
      if(!(SetFlag&LCDVESATiming)) tempbx+=5;  
    }
/* ynlai begin */
    else {
      if(IF_DEF_HiVision==1) {
        if(VGAVDE>480) SetFlag=SetFlag&(!TVSimuMode);
        tempcl=HiTVDataLen;
        tempbx=2;
        if(VBInfo&SetInSlaveMode) {
           if(!(SetFlag&TVSimuMode)) tempbx=10;
        }
      }
      else {
        if(VBInfo&SetPALTV){
          tempcl=TVDataLen;
          tempbx=3;
        }
        else{
          tempbx=4;
          tempcl=TVDataLen;
        }
      }
    }
/* ynlai end */
    if(SetFlag&TVSimuMode){
      tempbx=tempbx+4;
    }
    if(ModeNo<=0x13){
      tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));   /* si+St_CRT2CRTC */
    }else{
      tempal=*((UCHAR *)(ROMAddr+REFIndex+4));    /* di+Ext_CRT2CRTC */
    }
    tempal=tempal&0x1F;

    tempax=tempal*tempcl;
    REFIndex=*((USHORT *)(ROMAddr + 0x20E + tempbx*2));
    REFIndex+=tempax;
  }
  else {   /* LVDS */
    Flag=1;
    tempbx=0;
    if(IF_DEF_CH7005==1) {
      if(!(VBInfo&SetCRT2ToLCD)) {
        Flag=0;
        tempbx=7;
	if(VBInfo&SetPALTV) tempbx=tempbx+2;
        if(VBInfo&SetCHTVOverScan) tempbx=tempbx+1;
      } 
    }
    tempcl=LVDSDataLen;
    if(Flag==1) {
      tempbx=LCDResInfo-Panel800x600;
      if(LCDInfo&LCDNonExpanding){
        tempbx=tempbx+3;
      }
    } 
    if(ModeNo<=0x13) tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));      /* si+St_CRT2CRTC */
    else tempal=*((UCHAR *)(ROMAddr+REFIndex+0x04));      /* di+Ext_CRT2CRTC */
    tempal=tempal&0x1F;
    tempax=tempal*tempcl;
    CRT2PtrData=*((USHORT *)(ROMAddr+ADR_CRT2PtrData)); /*ADR_CRT2PtrData is defined in init.def  */
    REFIndex=*((USHORT *)(ROMAddr+CRT2PtrData+tempbx*2));
    REFIndex+=tempax;
  }
}

VOID UnLockCRT2(USHORT BaseAddr)
{
  UCHAR temp3;
  USHORT  Part1Port;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  temp3=(UCHAR)GetReg1(Part1Port,0x24);
  temp3=temp3|0x01;
  SetReg1(Part1Port,0x24,(USHORT)temp3);
}

VOID SetCRT2ModeRegs(USHORT BaseAddr,USHORT ModeNo)
{
  USHORT i,j;
  USHORT tempcl,tempah,temp3;
  USHORT  Part4Port;
  USHORT  Part1Port;
  Part4Port=BaseAddr+IND_SIS_CRT2_PORT_14;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  for(i=0,j=4;i<3;i++,j++){
     SetReg1(Part1Port,j,0);
  }

  tempcl=ModeType;
  if(ModeNo>0x13){
    tempcl=tempcl-ModeVGA;
    if((tempcl>0)||(tempcl==0)){
      tempah=((0x010>>tempcl)|0x080);
    }
  }else{
    tempah=0x080;
  }

  if(VBInfo&SetInSlaveMode){
    tempah=(tempah^0x0A0);
  }
  if(VBInfo&CRT2DisplayFlag){
    tempah=0;
  }
  SetReg1(Part1Port,0,tempah);


  if(IF_DEF_LVDS==0) {
    tempah=0x01;
    if(!(VBInfo&SetInSlaveMode)){
      tempah=(tempah|0x02);
    }
    if(!(VBInfo&SetCRT2ToRAMDAC)){
      tempah=(tempah^0x05);
      if(!(VBInfo&SetCRT2ToLCD)){
        tempah=(tempah^0x01);
      }
    }
    tempah=(tempah<<5)&0xFF;
    if(VBInfo&CRT2DisplayFlag){
      tempah=0;
    }
    SetReg1(Part1Port,0x01,tempah);

    tempah=tempah>>5;
    if((ModeType==ModeVGA)&&(!(VBInfo&SetInSlaveMode))){
      tempah=tempah|0x010;
    }
    if(LCDResInfo!=Panel1024x768){
      tempah=tempah|0x080;
    }
    if(VBInfo&SetCRT2ToTV){
      if(VBInfo&SetInSlaveMode){
        tempah=tempah|0x020;
      }
    }

    temp3=(UCHAR)GetReg1(Part4Port,0x0D);
    temp3=temp3&(~0x0BF);
    temp3=temp3|tempah;
    SetReg1(Part4Port,0x0D,(USHORT)temp3);

/* ynlai begin */
    tempah=0;
    if(VBInfo&SetCRT2ToTV) {
      if(VBInfo&SetInSlaveMode) {
        if(!(SetFlag&TVSimuMode)) {
          if(IF_DEF_HiVision==0) {
            SetFlag=SetFlag|RPLLDIV2XO;
            tempah=tempah|0x40;
          }
        }
      }
      else {
        SetFlag=SetFlag|RPLLDIV2XO;
        tempah=tempah|0x40;
      }
    }
    if(LCDResInfo==Panel1280x1024) tempah=tempah|0x80;
    if(LCDResInfo==Panel1280x960) tempah=tempah|0x80;
    SetReg1(Part4Port,0x0C,(USHORT)temp3);
/* ynlai end */
  }
  else {
    tempah=0;
    if(!(VBInfo&SetInSlaveMode)){
      tempah=tempah|0x02;
    }
    tempah=(tempah<<5)&0x0FF;
    if(VBInfo&CRT2DisplayFlag){
      tempah=0;
    }
    SetReg1(Part1Port,0x01,tempah);
  }
}

VOID SetGroup1(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo,
               ScrnInfoPtr pScrn)
{
  if(IF_DEF_LVDS==0){  /*301  */
    SetGroup1_301(BaseAddr,ROMAddr,ModeNo,pScrn);
  }else{  /*LVDS  */
    SetGroup1_LVDS(BaseAddr,ROMAddr,ModeNo,pScrn);
  }
}

VOID SetGroup1_LVDS(USHORT  BaseAddr,ULONG ROMAddr,USHORT ModeNo,
                    ScrnInfoPtr pScrn)
{
  USHORT temp1,temp2,tempcl,tempch,tempbh,tempal,tempah,tempax,tempbx;
  USHORT tempcx,OldREFIndex,lcdhdee;
  USHORT Part1Port;
  USHORT temppush1,temppush2;
  unsigned long int tempeax,tempebx,tempecx,templong;

  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  OldREFIndex=(USHORT)REFIndex;         /*push di  */

  SetCRT2Offset(Part1Port,ROMAddr);
  SetCRT2FIFO(Part1Port,ROMAddr,ModeNo,pScrn);
  SetCRT2Sync(BaseAddr,ROMAddr,ModeNo);

  temp1=(VGAHT-1)&0x0FF;            /*BTVGA2HT 0x08,0x09  */
  SetReg1(Part1Port,0x08,temp1);
  temp1=(((VGAHT-1)&0xFF00)>>8)<<4;
  SetRegANDOR(Part1Port,0x09,~0x0F0,temp1);
 
  
  temp1=(VGAHDE+12)&0x0FF;              /*BTVGA2HDEE 0x0A,0x0C  */
  SetReg1(Part1Port,0x0A,temp1);
  /*temp1=((VGAHDE+12)&0xFF00)>>8;    Wrong              */
  /*SetReg1(Part1Port,0x0C,temp1);  */
  
  temp1=VGAHDE+12;      /*bx  BTVGA@HRS 0x0B,0x0C  */
  temp2=(VGAHT-VGAHDE)>>2;      /* */
  temp1=temp1+temp2;
  temp2=(temp2<<1)+temp1;
  tempcl=temp2&0x0FF;

  SetReg1(Part1Port,0x0B,(USHORT)(temp1&0x0FF));
  tempah=(temp1&0xFF00)>>8;
  tempbh=((((VGAHDE+12)&0xFF00)>>8)<<4)&0x0FF;
  tempah=tempah|tempbh;
  SetReg1(Part1Port,0x0C,tempah);
  SetReg1(Part1Port,0x0D,tempcl);       /*BTVGA2HRE 0x0D  */
  tempcx=(VGAVT-1);
  tempah=tempcx&0x0FF;
  if(IF_DEF_CH7005==1) {
    if(VBInfo&0x0C)  tempah=tempah-1;
  } 
  SetReg1(Part1Port,0x0E,tempah);        /*BTVGA2TV 0x0E,0x12 */
  tempbx=VGAVDE-1;
  tempah=tempbx&0x0FF;
  if(IF_DEF_CH7005==1) {
    if(VBInfo&0x0C)  tempah=tempah-1;
  } 
  SetReg1(Part1Port,0x0F,tempah);       /*BTVGA2VDEE 0x0F,0x12  */
  tempah=((tempbx&0xFF00)<<3)>>8;
  tempah=tempah|((tempcx&0xFF00)>>8);
  SetReg1(Part1Port,0x12,tempah);
  
  tempbx=(VGAVT+VGAVDE)>>1;             /*BTVGA2VRS     0x10,0x11 */
  tempcx=((VGAVT-VGAVDE)>>4)+tempbx+1;  /*BTVGA2VRE     0x11  */

  tempah=tempbx&0x0FF;
  SetReg1(Part1Port,0x10,tempah);
  tempbh=(tempbx&0xFF00)>>8;
  tempah=((tempbh<<4)&0x0FF)|(tempcx&0x0F);
  SetReg1(Part1Port,0x11,tempah);

  SetRegANDOR(Part1Port,0x13,~0x03C,tempah);

  /*lines below are newly added for LVDS  */
  tempax=LCDHDES;
  tempbx=HDE;
  tempcx=HT;
  tempcx=tempcx-tempbx; /*HT-HDE  */
  /*push ax       lcdhdes  */
  tempax=tempax+tempbx; /*lcdhdee  */
  tempbx=HT;
  if(tempax>=tempbx){
    tempax=tempax-tempbx;
  }
  /*push ax   lcdhdee  */
  lcdhdee=tempax;
  tempcx=tempcx>>2;     /*temp  */
  tempcx=tempcx+tempax; /*lcdhrs  */
  if(tempcx>=tempbx){
    tempcx=tempcx-tempbx;
  }
  /* v ah,cl  */
  tempax=tempcx;
  tempax=tempax>>3; /*BPLHRS */
  tempah=tempax&0x0FF;
  SetReg1(Part1Port,0x14,tempah); /*Part1_14h  */
  tempah=tempah+2;
  tempah=tempah+0x01F;
  tempcl=tempcx&0x0FF;
  tempcl=tempcl&0x07;
  tempcl=(tempcl<<5)&0xFF; /* PHLHSKEW */
  tempah=tempah|tempcl;
  SetReg1(Part1Port,0x15,tempah); /*Part1_15h  */
  tempbx=lcdhdee;       /*lcdhdee  */
  tempcx=LCDHDES;       /*lcdhdes  */
  tempah=(tempcx&0xFF);
  tempah=tempah&0x07;   /*BPLHDESKEW  */
  SetReg1(Part1Port,0x1A,tempah); /*Part1_1Ah  */
  tempcx=tempcx>>3;     /*BPLHDES */
  tempah=(tempcx&0xFF);
  SetReg1(Part1Port,0x16,tempah); /*Part1_16h  */
  tempbx=tempbx>>3;     /*BPLHDEE  */
  tempah=tempbx&0xFF;
  SetReg1(Part1Port,0x17,tempah); /*Part1_17h  */

  tempcx=VGAVT;
  tempbx=VGAVDE;
  tempcx=tempcx-tempbx;         /* GAVT-VGAVDE  */
  tempbx=LCDVDES;               /*VGAVDES  */
  temppush1=tempbx;             /*push bx temppush1 */
  if(IF_DEF_TRUMPION==0){
    if(IF_DEF_CH7005==1) tempax=VGAVDE;
    if(VBInfo&SetCRT2ToLCD) {
      if(LCDResInfo==Panel800x600) tempax=600;
      else tempax=768;  
    }
  }
  else tempax=VGAVDE;
  tempbx=tempbx+tempax;
  tempax=VT;            /*VT  */
  if(tempbx>=VT){
    tempbx=tempbx-tempax;
  }
  temppush2=tempbx;     /*push bx  temppush2  */
  tempcx=tempcx>>1;
  tempbx=tempbx+tempcx;
  tempbx++;     /*BPLVRS  */
  if(tempbx>=tempax){
    tempbx=tempbx-tempax;
  }
  tempah=tempbx&0xFF;
  SetReg1(Part1Port,0x18,tempah); /*Part1_18h  */
  tempcx=tempcx>>3;
  tempcx=tempcx+tempbx;
  tempcx++;             /*BPLVRE  */
  tempah=tempcx&0xFF;
  tempah=tempah&0x0F;
  tempah=tempah|0x030;
  SetRegANDOR(Part1Port,0x19,~0x03F,tempah); /*Part1_19h  */
  tempbh=(tempbx&0xFF00)>>8;
  tempbh=tempbh&0x07;
  tempah=tempbh;
  tempah=(tempah<<3)&0xFF;      /*BPLDESKEW =0 */
  /*movzx  */
  tempbx=VGAVDE;
  if(tempbx!=VDE){
    tempah=tempah|0x40;
  }
  SetRegANDOR(Part1Port,0x1A,0x07,tempah); /*Part1_1Ah */
  tempecx=VGAVT;
  tempebx=VDE;
  tempeax=VGAVDE;
  tempecx=tempecx-tempeax;      /*VGAVT-VGAVDE  */
  tempeax=tempeax*64;
  templong=tempeax/tempebx;
  if(templong*tempebx<tempeax){
    templong++;
  }
  tempebx=templong;     /*BPLVCFACT  */
  if(SetFlag&EnableLVDSDDA){
    tempebx=tempebx&0x03F;
  }
  tempah=(USHORT)(tempebx&0x0FF);
  SetReg1(Part1Port,0x1E,tempah); /*Part1_1Eh */
  tempbx=temppush2;     /* p bx temppush2 BPLVDEE  */
  tempcx=temppush1;     /*pop cx temppush1 NPLVDES */
  tempbh=(tempbx&0xFF00)>>8;
  tempah=tempah&0x07;
  tempah=tempbh;
  tempah=tempah<<3;
  tempch=(tempcx&0xFF00)>>8;
  tempch=tempch&0x07;
  tempah=tempah|tempch;
  SetReg1(Part1Port,0x1D,tempah); /*Part1_1Dh */
  tempah=tempbx&0xFF;
  SetReg1(Part1Port,0x1C,tempah); /*Part1_1Ch  */
  tempah=tempcx&0xFF;
  SetReg1(Part1Port,0x1B,tempah); /*Part1_1Bh  */
  
  tempecx=VGAHDE;
  tempebx=HDE;
  tempeax=tempecx;
  tempeax=tempeax<<6;
  tempeax=tempeax<<10;
  tempeax=tempeax/tempebx;
  if(tempebx==tempecx){
    tempeax=65535;
  }
  tempecx=tempeax;
  tempeax=VGAHT;
  tempeax=tempeax<<6;
  tempeax=tempeax<<10;
  tempeax=tempeax/tempecx;
  tempecx=tempecx<<16;
  tempeax=tempeax-1;
  tempax=(USHORT)(tempeax&0x00FFFF);
  tempcx=tempax;
  tempah=tempcx&0x0FF;
  SetReg1(Part1Port,0x1F,tempah); /*Part1_1Fh  */
  tempbx=VDE;
  tempbx--;             /*BENPLACCEND */
  if(SetFlag&EnableLVDSDDA){
    tempbx=1;
  }
  tempah=(tempbx&0xFF00)>>8;
  tempah=(tempah<<3)&0xFF;
  tempch=(tempcx&0xFF00)>>8;
  tempch=tempch&0x07;
  tempah=tempah|tempch;
  SetReg1(Part1Port,0x20,tempah); /*Part1_20h */
  tempah=tempbx&0xFF;
  SetReg1(Part1Port,0x21,tempah); /*Part1_21h */
  tempecx=tempecx>>16;          /*BPLHCFACT  */
  temp1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag  */
  if(temp1&HalfDCLK){
    tempecx=tempecx>>1;
  }
  tempcx=(USHORT)(tempecx&0x0FFFF);
  tempah=(tempcx&0xFF00)>>8;
  SetReg1(Part1Port,0x22,tempah); /*Part1_22h */
  tempah=tempcx&0x0FF;
  SetReg1(Part1Port,0x23,tempah); /*Part1_23h */
  if(IF_DEF_TRUMPION==1){
    tempal=(USHORT)*((UCHAR *)(ROMAddr+ModeIDOffset+0x05));   /* si+St_ResInfo */
    if(ModeNo>0x13){
      SetFlag=SetFlag|ProgrammingCRT2;
      GetRatePtrCRT2(ROMAddr,ModeNo);
      tempal=*((UCHAR *)(ROMAddr+REFIndex+0x04));      /* di+Ext_CRT2CRTC */
      tempal=tempal&0x1F;
    }
    tempah=0x80;
    tempal=tempal*tempah;
    REFIndex= offset_Zurac; /*offset Zurac need added in rompost.asm  */
    REFIndex=REFIndex+tempal;
    SetTPData();   /*this function not implemented yet  */
    SetTPData();
    SetTPData();
    SetTPData();
    SetTPData();
    SetTPData();
    SetTPData();
    SetTPData();
    SetTPData();
  }
 
  REFIndex=OldREFIndex;         /*pop di  */

  return;
}

VOID SetTPData()
{
  return;
}


VOID SetGroup1_301(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo,ScrnInfoPtr pScrn)
{
  SISPtr  pSiS = SISPTR(pScrn);
  USHORT  temp1,temp2,tempcl,tempch,tempbl,tempbh,tempal,tempah,tempax,tempbx;
  USHORT  tempcx,OldREFIndex;
  USHORT  Part1Port,resinfo,modeflag;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  OldREFIndex=REFIndex;         /* push di */

  SetCRT2Offset(Part1Port,ROMAddr);
  SetCRT2FIFO(Part1Port,ROMAddr,ModeNo,pScrn);
  SetCRT2Sync(BaseAddr,ROMAddr,ModeNo);

  GetCRT1Ptr(ROMAddr);

  temp1=(VGAHT-1)&0x0FF;            /* BTVGA2HT 0x08,0x09 */
  SetReg1(Part1Port,0x08,temp1);
  temp1=(((VGAHT-1)&0xFF00)>>8)<<4;
  SetRegANDOR(Part1Port,0x09,~0x0F0,temp1);

  temp1=(VGAHDE+12)&0x0FF;              /* BTVGA2HDEE 0x0A,0x0C */
  SetReg1(Part1Port,0x0A,temp1);

  temp1=VGAHDE+12;      /* bx  BTVGA@HRS 0x0B,0x0C */
  temp2=(VGAHT-VGAHDE)>>2;      /* cx */
  temp1=temp1+temp2;
  temp2=(temp2<<1)+temp1;
  tempcl=temp2&0x0FF;
  if(VBInfo&SetCRT2ToRAMDAC){
    tempbl=*((UCHAR *)(ROMAddr+REFIndex+4));    /* di+4      */
    tempbh=*((UCHAR *)(ROMAddr+REFIndex+14));   /* di+14     */
    temp1=((tempbh>>6)<<8)|tempbl;              /* temp1->bx */
    temp1=(temp1-1)<<3;
    tempcl=*((UCHAR *)(ROMAddr+REFIndex+5));    /* di+5  */
    tempch=*((UCHAR *)(ROMAddr+REFIndex+15));   /* di+15 */
    tempcl=tempcl&0x01F;
    tempch=(tempch&0x04)<<(6-2);
    tempcl=((tempcl|tempch)-1)<<3;
  }
  SetReg1(Part1Port,0x0B,(USHORT)(temp1&0x0FF));
  tempah=(temp1&0xFF00)>>8;
  tempbh=((((VGAHDE+12)&0xFF00)>>8)<<4)&0x0FF;
  tempah=tempah|tempbh;
  SetReg1(Part1Port,0x0C,tempah);
  SetReg1(Part1Port,0x0D,tempcl);       /* BTVGA2HRE 0x0D */
  tempcx=(VGAVT-1);
  tempah=tempcx&0x0FF;
  SetReg1(Part1Port,0x0E,tempah);       /* BTVGA2TV 0x0E,0x12 */
  tempbx=VGAVDE-1;
  tempah=tempbx&0x0FF;
  SetReg1(Part1Port,0x0F,tempah);       /* BTVGA2VDEE 0x0F,0x12 */
  tempah=((tempbx&0xFF00)<<3)>>8;
  tempah=tempah|((tempcx&0xFF00)>>8);
  SetReg1(Part1Port,0x12,tempah);

  tempbx=(VGAVT+VGAVDE)>>1;             /* BTVGA2VRS     0x10,0x11 */
  tempcx=((VGAVT-VGAVDE)>>4)+tempbx+1;  /* BTVGA2VRE     0x11      */
  if(VBInfo&SetCRT2ToRAMDAC){
    tempbx=*((UCHAR *)(ROMAddr+REFIndex+8));   /* di+8 */
    temp1=*((UCHAR *)(ROMAddr+REFIndex+7));    /* di+7 */
    if(temp1&0x04){
      tempbx=tempbx|0x0100;
    }
    if(temp1&0x080){
      tempbx=tempbx|0x0200;
    }
    temp1=*((UCHAR *)(ROMAddr+REFIndex+13));    /* di+13 */
    if(temp1&0x08){
      tempbx=tempbx|0x0400;
    }
    tempcl= *((UCHAR *)(ROMAddr+REFIndex+9));   /* di+9 */
    tempcx=(tempcx&0xFF00)|(tempcl&0x00FF);
  }
  tempah=tempbx&0x0FF;
  SetReg1(Part1Port,0x10,tempah);
  tempbh=(tempbx&0xFF00)>>8;
  tempah=((tempbh<<4)&0x0FF)|(tempcx&0x0F);
  SetReg1(Part1Port,0x11,tempah);

  if( pSiS->Chipset == PCI_CHIP_SIS300 ){
    tempah=0x10;
    if((LCDResInfo!=Panel1024x768)&&(LCDResInfo==Panel1280x1024)) tempah=0x20;
  }
  else tempah=0x20;
  if(VBInfo&SetCRT2ToTV) tempah=0x08;
/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    if(VBInfo&SetInSlaveMode) tempah=0x2c;
    else tempah=0x20;
  }
/* ynlai end */
  SetRegANDOR(Part1Port,0x13,~0x03C,tempah);

  if(!(VBInfo&SetInSlaveMode)){
    REFIndex=OldREFIndex;
    return;
  }
  if(VBInfo&SetCRT2ToTV){
    tempax=0xFFFF;
  }else{
    tempax=GetVGAHT2();
  }
  tempcl=0x08;                  /* Reg 0x03 Horozontal Total */
  temp1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
  if(!(temp1&Charx8Dot)){                           /* temp1->St_ModeFlag */
    tempcl=0x09;
  }
  if(tempax>=VGAHT){
    tempax=VGAHT;
  }
  if(temp1&HalfDCLK){
    tempax=tempax>>1;
  }
  tempax=(tempax/tempcl)-5;
  tempbl=tempax;
  tempah=0xFF;          /* set MAX HT */
  SetReg1(Part1Port,0x03,tempah);

  tempax=VGAHDE;                /* 0x04 Horizontal Display End */
  if(temp1&HalfDCLK){
    tempax=tempax>>1;
  }
  tempax=(tempax/tempcl)-1;
  tempbh=tempax;
  SetReg1(Part1Port,0x04,tempax);

  tempah=tempbh;
  if(VBInfo&SetCRT2ToTV){
    tempah=tempah+2;
  }
/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    resinfo=*(USHORT *)(ROMAddr+ModeIDOffset+0x09); /* si+Ext_ResInfo */
    if(resinfo==7) tempah=tempah-2;
  }
/* ynlai end */
  SetReg1(Part1Port,0x05,tempah); /* 0x05 Horizontal Display Start */
  SetReg1(Part1Port,0x06,0x03);   /* 0x06 Horizontal Blank end     */
                                  /* 0x07 horizontal Retrace Start */
/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    tempah=tempbl-1;
    modeflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
    if(!(modeflag&HalfDCLK)) {
      tempah=tempah-6;
      if(SetFlag&TVSimuMode) {
        tempah=tempah-4;
        if(ModeNo>0x13) tempah=tempah-10;
      }
    }
  }
/* ynlai end */
  else {
    tempcx=(tempbl+tempbh)>>1;
    tempah=(tempcx&0xFF)+2;

    if(VBInfo&SetCRT2ToTV){
       tempah=tempah-1;
       if(!(temp1&HalfDCLK)){
          if((temp1&Charx8Dot)){
            tempah=tempah+4;
            if(VGAHDE>=800){
              tempah=tempah-6;
            }
          }
        }
    }else{
       if(!(temp1&HalfDCLK)){
         tempah=tempah-4;
         if(VGAHDE>=800){
           tempah=tempah-7;
           if(ModeType==ModeEGA){
             if(VGAVDE==1024){
               tempah=tempah+15;
               if(LCDResInfo!=Panel1280x1024){
                  tempah=tempah+7;
               }
             }
           }
           if(VGAHDE>=1280){
             tempah=tempah+28;
           }
         }
       }
    }
  }
  SetReg1(Part1Port,0x07,tempah); /* 0x07 Horizontal Retrace Start */

  SetReg1(Part1Port,0x08,0);      /* 0x08 Horizontal Retrace End   */
  SetReg1(Part1Port,0x18,0x03);   /* 0x18 SR08                     */
  SetReg1(Part1Port,0x19,0);      /* 0x19 SR0C                     */
  SetReg1(Part1Port,0x09,0xFF);   /* 0x09 Set Max VT               */

  tempcx=0x121;
  tempcl=0x21;
  tempch=0x01;
  tempbx=VGAVDE;                /* 0x0E Virtical Display End */
  if(tempbx==360) tempbx=350;
  if(tempbx==375) tempbx=350;
  if(tempbx==405) tempbx=400;
  tempbx--;
  tempah=tempbx&0x0FF;
  SetReg1(Part1Port,0x0E,tempah);
  SetReg1(Part1Port,0x10,tempah); /* 0x10 vertical Blank Start */
  tempbh=(tempbx&0xFF00)>>8;
  if(tempbh&0x01){
    tempcl=tempcl|0x0A;
  }
  tempah=0;tempal=0x0B;
  if(temp1&DoubleScanMode){
    tempah=tempah|0x080;
  }
  if(tempbh&0x02){
    tempcl=tempcl|0x040;
    tempah=tempah|0x020;
  }
  SetReg1(Part1Port,0x0B,tempah);
  if(tempbh&0x04){
    tempch=tempch|0x06;
  }

  SetReg1(Part1Port,0x11,0);  /* 0x11 Vertival Blank End */

  tempax=VGAVT-tempbx;          /* 0x0C Vertical Retrace Start */
  tempax=tempax>>2;
  temp2=tempax;         /* push ax */
  tempax=tempax<<1;
  tempbx=tempax+tempbx;
/*  ynlai begin */
/*  ynlai end   */
  if((SetFlag&TVSimuMode)&&(VBInfo&SetPALTV)&&(VGAHDE==800)){
    tempbx=tempbx+40;
  }
  tempah=(tempbx&0x0FF);
  SetReg1(Part1Port,0x0C,tempah);
  tempbh=(tempbx&0xFF00)>>8;
  if(tempbh&0x01){
    tempcl=tempcl|0x04;
  }
  if(tempbh&0x02){
    tempcl=tempcl|0x080;
  }
  if(tempbh&0x04){
    tempch=tempch|0x08;
  }

  tempax=temp2;         /* pop ax */
  tempax=(tempax>>2)+1;
  tempbx=tempbx+tempax;
  tempah=(tempbx&0x0FF)&0x0F;
  SetReg1(Part1Port,0x0D,tempah);  /* 0x0D vertical Retrace End */
  tempbl=tempbx&0x0FF;
  if(tempbl&0x10){
    tempch=tempch|0x020;
  }

  tempah=tempcl;
  SetReg1(Part1Port,0x0A,tempah); /* 0x0A CR07 */
  tempah=tempch;
  SetReg1(Part1Port,0x17,tempah); /* 0x17 SR0A */
  tempax=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
  tempah=(tempax&0xFF00)>>8;
  tempah=(tempah>>1)&0x09;
  SetReg1(Part1Port,0x16,tempah); /* 0x16 SR01 */
  SetReg1(Part1Port,0x0F,0);      /* 0x0F CR14 */
  SetReg1(Part1Port,0x12,0);      /* 0x12 CR17 */
  SetReg1(Part1Port,0x1A,0);      /* 0x1A SR0E */

  REFIndex=OldREFIndex;           /* pop di */
}

VOID SetCRT2Offset(USHORT Part1Port,ULONG ROMAddr)
{
  USHORT offset;
  if(VBInfo&SetInSlaveMode){
    return;
  }
  offset=GetOffset(ROMAddr);
  SetReg1(Part1Port,0x07,(USHORT)(offset&0xFF));
  SetReg1(Part1Port,0x09,(USHORT)((offset&0xFF00)>>8));
  SetReg1(Part1Port,0x03,(USHORT)(((offset>>3)&0xFF)+1));
}

USHORT GetOffset(ULONG ROMAddr)
{
  USHORT tempal,temp1,colordepth;
  tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x03));    /* si+Ext_ModeInfo */
  tempal=(tempal>>4)&0xFF;
  ScreenOffset=*((USHORT *)(ROMAddr+0x206));         /* Get ScreeOffset table */
  tempal=*((UCHAR *)(ROMAddr+ScreenOffset+tempal));  /* get ScreenOffset */
  tempal=tempal&0xFF;
  temp1=*((UCHAR *)(ROMAddr+REFIndex));              /* di+Ext_InfoFlag */
  if(temp1&InterlaceMode){
    tempal=tempal<<1;
  }
  colordepth=GetColorDepth(ROMAddr);
  return(tempal*colordepth);
}

USHORT GetColorDepth(ULONG ROMAddr)
{
  USHORT ColorDepth[6]={1,2,4,4,6,8};
  USHORT temp;
  int temp1;
  temp=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
  temp1=(temp&ModeInfoFlag)-ModeEGA;
  if(temp1<0) temp1=0;
  return(ColorDepth[temp1]);
}

VOID SetCRT2FIFO(USHORT  Part1Port,ULONG ROMAddr,USHORT ModeNo,ScrnInfoPtr pScrn)
{
  SISPtr  pSiS = SISPTR(pScrn);
  USHORT temp,temp1,temp2,temp3,flag;
  USHORT vclk2ptr,latencyindex;
  USHORT oldREFIndex,CRT1ModeNo,oldModeIDOffset;
  long int longtemp;

  USHORT LatencyFactor[48]={ 88, 80, 78, 72, 70, 00,        /* 64  bit    BQ=2 */
                           00, 79, 77, 71, 69, 49,          /* 64  bit    BQ=1 */
                           88, 80, 78, 72, 70, 00,          /* 128 bit    BQ=2 */
                           00, 72, 70, 64, 62, 44,          /* 128 bit    BQ=1 */
                           73, 65, 63, 57, 55, 00,          /* 64  bit    BQ=2 */
                           00, 64, 62, 56, 54, 34,          /* 64  bit    BQ=1 */
                           78, 70, 68, 62, 60, 00,          /* 128 bit    BQ=2 */
                           00, 62, 60, 54, 52, 34};         /* 128 bit    BQ=1 */

  oldREFIndex=REFIndex;         /* push REFIndex(CRT2 now) */
  oldModeIDOffset=ModeIDOffset; /* push ModeIDOffset       */

  CRT1ModeNo=(UCHAR)GetReg1(P3d4,0x34); /* get CRT1 ModeNo  */
  SearchModeID(ROMAddr,CRT1ModeNo);     /* Get ModeID Table */

  GetRatePtr(ROMAddr,CRT1ModeNo); /* Set REFIndex-> for crt1 refreshrate */
  temp1=GetVCLK(ROMAddr,CRT1ModeNo);
  temp2=GetColorTh(ROMAddr);
  temp3=GetMCLK(ROMAddr);
  temp=((USHORT)(temp1*temp2)/temp3);   /* temp->bx */
  temp1=(UCHAR)GetReg1(P3c4,0x14);      /* SR_14    */
  temp1=temp1>>6;
  temp1=temp1<<1;
  if(temp1==0) temp1=1;
  temp1=temp1<<2;               /* temp1->ax */

  longtemp=temp1-temp;

  temp2=(USHORT)((28*16)/(int)longtemp);   /* temp2->cx */
  if(!((temp2*(int)longtemp)==(28*16))) temp2++;

  if( pSiS->Chipset == PCI_CHIP_SIS300 ){
    temp1=CalcDelayVB();
  }else{ /* for Trojan and Spartan */
    flag=(UCHAR)GetReg1(P3c4,0x14);   /* SR_14 */
    if(flag&0x80){
      latencyindex=12;  /* 128 bit */
    }else{
      latencyindex=0;   /* 64 bit */
    }
    flag=GetQueueConfig();
    if(!(flag&0x01)){
      latencyindex+=24; /* GUI timing =0 */
    }
    if(flag&0x10){
      latencyindex+=6;  /* BQ =2 */
    }
    latencyindex=latencyindex + (flag>>5);
    temp1= LatencyFactor[latencyindex];
    temp1=temp1+15;
    flag=(UCHAR)GetReg1(P3c4,0x14);   /* SR_14 */
    if(!(flag&0x80)){
      temp1=temp1+5;    /* 64 bit */
    }
  }

  temp2=temp2+temp1;
  REFIndex=oldREFIndex;         /* pop REFIndex(CRT2) */
  ModeIDOffset=oldModeIDOffset; /* pop ModeIDOffset */

  vclk2ptr=GetVCLK2Ptr(ROMAddr,ModeNo);
  temp1=*((USHORT *)(ROMAddr+vclk2ptr+(VCLKLen-2)));
  temp3=GetColorTh(ROMAddr);
  longtemp=temp1*temp2*temp3;
  temp3=GetMCLK(ROMAddr);
  temp3=temp3<<4;
  temp2=(int)(longtemp/temp3);
  if((long int)temp2*(long int)temp3<(long int)longtemp)
      temp2++;                /* temp2->cx */

  temp1=(UCHAR)GetReg1(Part1Port,0x01);         /* part1port index 01 */
  temp1=(temp1&(~0x1F))|0x16;
  SetReg1(Part1Port,0x01,temp1);

/* ynlai begin */
  if(IF_DEF_HiVision==1) { if(temp2<=10) temp2=10; }
  else { if(temp2<=6) temp2=6; }
/* ynlai end */
  if(temp2>0x14) temp2=0x14;
  temp1=(UCHAR)GetReg1(Part1Port,0x02);         /* part1port index 02 */
  temp1=(temp1&(~0x1F))|temp2;
  SetReg1(Part1Port,0x02,temp1);
}

USHORT GetVCLK(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempptr;
  USHORT temp1;
  tempptr=GetVCLKPtr(ROMAddr,ModeNo);
  temp1=*((USHORT *)(ROMAddr+tempptr+(VCLKLen-2)));
  return temp1;
}

USHORT GetQueueConfig()
{
  USHORT tempal,tempbl;
  ULONG tempeax;

  SetReg4(0xcf8,0x80000050);
  tempeax=GetReg3(0xcfc);
  tempeax=(tempeax>>24)&0x0f;
  tempbl=(USHORT)tempeax;
  tempbl=tempbl<<4;

  SetReg4(0xcf8,0x800000A0);
  tempeax=GetReg3(0xcfc);
  tempeax=(tempeax>>24)&0x0f;
  tempal=(USHORT)tempeax;
  tempbl=tempbl|tempal;

  return(tempbl);
}

USHORT GetVCLKPtr(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempal=0;
  if(IF_DEF_LVDS==0) {
    tempal=(UCHAR)GetReg2((USHORT)(P3ca+0x02)); /*  Port 3cch */
    tempal=((tempal>>2)&0x03);
    if(ModeNo>0x13){
      tempal=*((UCHAR *)(ROMAddr+REFIndex+0x03));    /* di+Ext_CRTVCLK */
    }
    VCLKLen=GetVCLKLen(ROMAddr);
    tempal=tempal*VCLKLen;
    tempal=tempal+(*((USHORT *)(ROMAddr+0x208)));  /* VCLKData */
    return ((USHORT)tempal);
  } else {
    if(LCDResInfo==Panel800x600) {
        tempal=VCLK40;
    } else if(LCDResInfo==Panel1024x768) {
                tempal=VCLK65;
           }
   VCLKLen=GetVCLKLen(ROMAddr);
   tempal=tempal*VCLKLen;
   tempal=tempal+(*((USHORT *)(ROMAddr+0x208)));
   return((USHORT)tempal);
  }
}

USHORT GetColorTh(ULONG ROMAddr)
{
  USHORT temp;
  temp=GetColorDepth(ROMAddr);
  temp=temp>>1;
  if(temp==0) temp++;
  return temp;
}

USHORT GetMCLK(ULONG ROMAddr)
{
  USHORT tempmclkptr;
  USHORT tempmclk;
  tempmclkptr=GetMCLKPtr(ROMAddr);
  tempmclk=*((USHORT *)(ROMAddr+tempmclkptr+0x03));        /* di+3 */
  return tempmclk;
}

USHORT GetMCLKPtr(ULONG ROMAddr)
{
  USHORT tempdi;
  USHORT tempdramtype,tempax;

  tempdi=*((USHORT *)(ROMAddr+0x20C));        /* MCLKData */
  tempdramtype=GetDRAMType(ROMAddr);
  tempax=5*tempdramtype;
  tempdi=tempdi+tempax;
  return (tempdi);
}

USHORT GetDRAMType(ULONG ROMAddr)
{
 USHORT tsoftsetting,temp3;
 tsoftsetting=*((UCHAR *)(ROMAddr+0x52));
 if(!(tsoftsetting&SoftDramType)){
   temp3=(UCHAR)GetReg1(P3c4,0x3A);
   tsoftsetting=temp3;
 }
 tsoftsetting=tsoftsetting&0x07;
 return(tsoftsetting);
}

USHORT CalcDelayVB()
{
  USHORT tempal,tempah,temp1,tempbx;
  USHORT ThTiming[8]={1,2,2,3,0,1,1,2};
  USHORT ThLowB[24]={81,4,72,6,88,8,120,12,
                  55,4,54,6,66,8,90,12,
                  42,4,45,6,55,8,75,12};

  tempah=(UCHAR)GetReg1(P3c4,0x18);     /* SR_18 */
  tempah=tempah&0x62;
  tempah=tempah>>1;
  tempal=tempah;
  tempah=tempah>>3;
  tempal=tempal|tempah;
  tempal=tempal&0x07;

  temp1=ThTiming[tempal];       /* temp1->cl */

  tempbx=(UCHAR)GetReg1(P3c4,0x16);     /* SR_16 */
  tempbx=tempbx>>6;
  tempah=(UCHAR)GetReg1(P3c4,0x14);     /* SR_14 */
  tempah=((tempah>>4)&0x0C);
  tempbx=((tempbx|tempah)<<1);

  tempal=ThLowB[tempbx+1]*temp1;
  tempbx=ThLowB[tempbx];
  tempbx=tempal+tempbx;

  return(tempbx);
}

USHORT GetVCLK2Ptr(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempal,tempbx,temp;
  USHORT LCDXlat1VCLK[4]={VCLK65,VCLK65,VCLK65,VCLK65};
  USHORT LCDXlat2VCLK[4]={VCLK108_2,VCLK108_2,VCLK108_2,VCLK108_2};
  USHORT LVDSXlat1VCLK[4]={VCLK40,VCLK40,VCLK40,VCLK40};
  USHORT LVDSXlat2VCLK[4]={VCLK65,VCLK65,VCLK65,VCLK65};
  USHORT LVDSXlat3VCLK[4]={VCLK65,VCLK65,VCLK65,VCLK65};

  if(IF_DEF_LVDS==0) {
    if(ModeNo<=0x13){
      tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));  /* si+St_CRT2CRTC */
    }else{
      tempal=*((UCHAR *)(ROMAddr+REFIndex+0x04));      /* di+Ext_CRT2CRTC */
    }
    tempal=tempal>>6;
    if(LCDResInfo!=Panel1024x768){
      tempal=LCDXlat2VCLK[tempal];
    }else{
      tempal=LCDXlat1VCLK[tempal];
    }

    if(VBInfo&SetCRT2ToLCD){
      tempal=tempal;
    }
/* ynlai begin */
    else {        /* for TV */
      if(VBInfo&SetCRT2ToTV) {
        if(IF_DEF_HiVision==1) {
          if(SetFlag&RPLLDIV2XO) tempal=HiTVVCLKDIV2;
          else tempal=HiTVVCLK;
          if(SetFlag&TVSimuMode){
            temp=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
            if(temp&Charx8Dot) tempal=HiTVSimuVCLK;
            else tempal=HiTVTextVCLK;
          }
        }
        else {
          if(VBInfo&SetCRT2ToTV){
            if(SetFlag&RPLLDIV2XO) tempal=TVVCLKDIV2;
            else tempal=TVVCLK;
          }
          else {
            tempal=(UCHAR)GetReg2((USHORT)(P3ca+0x02));      /*  Port 3cch */
            tempal=((tempal>>2)&0x03);
            if(ModeNo>0x13) tempal=*((UCHAR *)(ROMAddr+REFIndex+0x03));    /* di+Ext_CRTVCLK */
          }
        }
      }
    }
  } 
/* ynlai end */
  else {       /*   LVDS  */
    if(ModeNo<=0x13) tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));
    else tempal=*((UCHAR *)(ROMAddr+REFIndex+0x04));
    if(IF_DEF_CH7005==1) {
      if(!(VBInfo&SetCRT2ToLCD)) {
        tempal=tempal&0x1f;
        tempbx=0; 
        if(VBInfo&SetPALTV) tempbx=tempbx+2;
        if(VBInfo&SetCHTVOverScan) tempbx=tempbx+1;
        tempbx=tempbx<<1;
        temp=(*((USHORT *)(ROMAddr+ADR_CHTVVCLKPtr)));
        tempbx=(*((USHORT *)(ROMAddr+temp+tempbx)));
        tempal=(*((USHORT *)(ROMAddr+tempbx+tempal)));
        tempal=tempal&0x00FF;
      }
    }
    else {
      tempal=tempal>>6;
      if(LCDResInfo==Panel800x600) tempal=LVDSXlat1VCLK[tempal];
      else if(LCDResInfo==Panel1024x768) tempal=LVDSXlat2VCLK[tempal];
      else tempal=LVDSXlat3VCLK[tempal];
    }
  }
  VCLKLen=GetVCLKLen(ROMAddr);
  tempal=tempal*VCLKLen;
  tempal=tempal+(*((USHORT *)(ROMAddr+0x208)));      /* VCLKData */
  return ((USHORT)tempal);
}

USHORT GetVCLKLen(ULONG ROMAddr)
{
  USHORT VCLKDataStart,vclklabel,temp;
  VCLKDataStart=*((USHORT *)(ROMAddr+0x208));
  for(temp=0;;temp++){
     vclklabel=*((USHORT *)(ROMAddr+VCLKDataStart+temp));
     if(vclklabel==VCLKStartFreq){
       temp=temp+2;
       return(temp);
     }
  }
  return(0);
}


VOID SetCRT2Sync(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo)
{
  USHORT temp1,tempah=0;
  USHORT temp;
  USHORT  Part1Port;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  if(IF_DEF_LVDS==1){
    if(VBInfo&SetCRT2ToLCD){
      tempah=LCDInfo;
      if(!(tempah&LCDSync)){
        temp=*((USHORT *)(ROMAddr+REFIndex));    /*di+Ext_InfoFlag */
        tempah=(temp>>8)&0x0C0;
      }else{
        tempah=tempah&0x0C0;
      }
    }
  }
  else {
  temp=*((USHORT *)(ROMAddr+REFIndex));    /* di+Ext_InfoFlag */
  tempah=(temp>>8)&0x0C0;
  }
  temp1=(UCHAR)GetReg1(Part1Port,0x19);    /* part1port index 02 */
  temp1=(temp1&(~0x0C0))|tempah;
  SetReg1(Part1Port,0x19,temp1);
}

VOID GetCRT1Ptr(ULONG ROMAddr)
{
  USHORT temprefcrt1;
  USHORT temp;
  temp=*((UCHAR *)(ROMAddr+REFIndex+0x02));    /* di+Ext_CRT1CRTC */
  temp=temp*CRT1Len;
  temprefcrt1=*((USHORT *)(ROMAddr+0x204));    /* Get CRT1Table */
  REFIndex=temprefcrt1+temp;         /* di->CRT1Table+Ext_CRT1CRTC*CRT1Len */
}

VOID SetRegANDOR(USHORT Port,USHORT Index,USHORT DataAND,USHORT DataOR)
{
  USHORT temp1;
  temp1=GetReg1(Port,Index);     /* part1port index 02 */
  temp1=(temp1&(DataAND))|DataOR;
  SetReg1(Port,Index,temp1);
}

USHORT GetVGAHT2()
{
  long int temp1,temp2;
  temp1=(VGAVT-VGAVDE)*RVBHCMAX;
  temp1=temp1&0x0FFFF;
  temp2=(VT-VDE)*RVBHCFACT;
  temp2=temp2&0x0FFFF;
  temp2=temp2*HT;
  temp2=temp2/temp1;
  return((USHORT)temp2);
}

VOID SetGroup2(USHORT  BaseAddr,ULONG ROMAddr, USHORT ModeNo)
{
  USHORT tempah,tempbl,tempbh,tempcl,i,j,tempcx,pushcx,tempbx,tempax;
  USHORT tempmodeflag,tempflowflag;
  UCHAR *temp1;
  USHORT *temp2;
  USHORT pushbx;
  USHORT  Part2Port;
  USHORT  modeflag;
  long int longtemp;

  Part2Port=BaseAddr+IND_SIS_CRT2_PORT_10;
  tempcx=VBInfo;
  tempah=VBInfo&0x0FF;
  tempbl=VBInfo&0x0FF;
  tempbh=VBInfo&0x0FF;
  tempbx=(tempbl&0xFF)|(tempbh<<8);
  tempbl=tempbl&0x10;
  tempbh=(tempbh&0x04)<<1;
  tempah=(tempah&0x08)>>1;
  tempah=tempah|tempbh;
  tempbl=tempbl>>3;
  tempah=tempah|tempbl;
  tempah=tempah^0x0C;

  if(IF_DEF_HiVision==1) {
    temp1=(UCHAR *)(ROMAddr+0x0F1);     /* PALPhase */
    tempah=tempah^0x01;
    if(VBInfo&SetInSlaveMode) {
      temp2=HiTVSt2Timing;
      if(SetFlag&TVSimuMode) {
        modeflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   
        if(modeflag&Charx8Dot) temp2=HiTVSt1Timing;
        else temp2=HiTVTextTiming;
      }	
    }
    else  temp2=HiTVExtTiming;
  }
  else {
    if(VBInfo&SetPALTV){
      temp1=(UCHAR *)(ROMAddr+0x0F1);     /* PALPhase */
      temp2=PALTiming;
    }else{
      tempah=tempah|0x10;
      temp1=(UCHAR *)(ROMAddr+0x0ED);     /* NTSCPhase */
      temp2=NTSCTiming;
    }
  }
  SetReg1(Part2Port,0x0,tempah);
  for(i=0x31;i<=0x34;i++,temp1++){
     SetReg1(Part2Port,i,*(UCHAR *)temp1);
  }
  for(i=0x01,j=0;i<=0x2D;i++,j++){
     SetReg1(Part2Port,i,temp2[j]);
  }
  for(i=0x39;i<=0x45;i++,j++){
     SetReg1(Part2Port,i,temp2[j]);   /* di->temp2[j] */
  }

  tempah=GetReg1(Part2Port,0x0A);
  tempah=tempah|NewFlickerMode;
  SetReg1(Part2Port,0x0A,tempah);

  SetReg1(Part2Port,0x35,RY1COE);
  SetReg1(Part2Port,0x36,RY2COE);
  SetReg1(Part2Port,0x37,RY3COE);
  SetReg1(Part2Port,0x38,RY4COE);

/* ynlai begin */
  if(IF_DEF_HiVision==1) tempax=950;
  else {
    if(VBInfo&SetPALTV) tempax=520;
    else tempax=440;
  }
  if(VDE<=tempax) {
    tempax=tempax-VDE;
    tempax=tempax>>2;
    tempah=(tempax&0xFF00)>>8;
    tempah=tempah+temp2[0];
    SetReg1(Part2Port,0x01,tempah);
    tempah=tempax&0x00FF;
    tempah=tempah+temp2[1];
    SetReg1(Part2Port,0x02,tempah);
  }
/* begin end */

  tempcx=HT-1;
  tempah=tempcx&0xFF;
  SetReg1(Part2Port,0x1B,tempah);
  tempah=(tempcx&0xFF00)>>8;
  SetRegANDOR(Part2Port,0x1D,~0x0F,(UCHAR)tempah);

  tempcx=HT>>1;
  pushcx=tempcx; /* push cx */

  tempcx=tempcx+7;
/* ynlai begin */
  if(IF_DEF_HiVision==1) tempcx=tempcx-4;
/* ynlai end   */
  tempah=(tempcx&0xFF);
  tempah=(tempah<<4)&0xFF;
  SetRegANDOR(Part2Port,0x22,~0x0F0,tempah);


  tempbx=temp2[j];
  tempbx=tempbx+tempcx;
  tempah=tempbx&0xFF;
  SetReg1(Part2Port,0x24,tempah);
  tempah=(tempbx&0xFF00)>>8;
  tempah=(tempah<<4)&0xFF;
  SetRegANDOR(Part2Port,0x25,~0x0F0,tempah);

  tempbx=tempbx+8;
/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    tempbx=tempbx-4;
    tempcx=tempbx;
  }
/* ynlai end */
  tempah=((tempbx&0xFF)<<4)&0xFF;
  SetRegANDOR(Part2Port,0x29,~0x0F0,tempah);

  tempcx=tempcx+temp2[++j];
  tempah=tempcx&0xFF;
  SetReg1(Part2Port,0x27,tempah);
  tempah=(((tempcx&0xFF00)>>8)<<4)&0xFF;
  SetRegANDOR(Part2Port,0x28,~0x0F0,tempah);

  tempcx=tempcx+8;
/* ynlai begin */
  if(IF_DEF_HiVision==1) tempcx=tempcx-4;
/* ynlai end   */
  tempah=tempcx&0xFF;
  tempah=(tempah<<4)&0xFF;
  SetRegANDOR(Part2Port,0x2A,~0x0F0,tempah);

  tempcx=pushcx;  /* pop cx */
  tempcx=tempcx-temp2[++j];
  tempah=tempcx&0xFF;
  tempah=(tempah<<4)&0xFF;
  SetRegANDOR(Part2Port,0x2D,~0x0F0,tempah);

  tempcx=tempcx-11;
  if(!(VBInfo&SetCRT2ToTV)){
    tempax=GetVGAHT2();
    tempcx=tempax-1;
  }
  tempah=tempcx&0xFF;
  SetReg1(Part2Port,0x2E,tempah);

  tempbx=VDE;
  if(VGAVDE==360){
    tempbx=746;
  }
  if(VGAVDE==375){
    tempbx=746;
  }
  if(VGAVDE==405){
    tempbx=853;
  }
  /* assuming <<ifndef>> HivisionTV */
  if((VBInfo&SetCRT2ToTV)){
    tempbx=tempbx>>1;
  }

  tempbx=tempbx-2;
  tempah=tempbx&0xFF;
/* ynlai begin */
  if(IF_DEF_HiVision==1)
    if(VBInfo&SetInSlaveMode)
      if(ModeNo==0x2f) tempah=tempah+1;
/* ynlai end   */
  SetReg1(Part2Port,0x2F,tempah);

  tempah=(tempcx&0xFF00)>>8;
  tempbh=(tempbx&0xFF00)>>8;
  tempbh=(tempbh<<6)&0xFF;
  tempah=tempah|tempbh;
  /* assuming <<ifndef>> hivisiontv */
/* ynlai begin */
  if(IF_DEF_HiVision==0) {
    tempah=tempah|0x10;
    if(!(VBInfo&SetCRT2ToSVIDEO)){
      tempah=tempah|0x20;
    }
  }
/* ynlai end   */
  SetReg1(Part2Port,0x30,tempah);

  tempbh=0;
  tempbx=tempbx&0xFF;

  tempmodeflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
  tempflowflag=0;
  if(!(tempmodeflag&HalfDCLK)){
    tempcx=VGAHDE;
    if(tempcx>=HDE){
      tempbh=tempbh|0x20;
      tempbx=(tempbh<<8)|(tempbx&0xFF);
      tempah=0;
    }
  }
  tempcx=0x0101;
/* ynlai begin */
  if(IF_DEF_HiVision==1) {
     if(VGAHDE>=1024) {
       tempcx=0x1920;
       if(VGAHDE>=1280) tempcx=0x1420;
     }
  }
/* ynlai end   */
  if(!(tempbh&0x20)){
    if(tempmodeflag&HalfDCLK){
      tempcl=((tempcx&0xFF)<<1)&0xFF;
      tempcx=(tempcx&0xFF00)|tempcl;
    }
    pushbx=tempbx;
    tempax=VGAHDE;
    tempbx=(tempcx&0xFF00)>>8;
    longtemp=tempax*tempbx;
    tempcx=tempcx&0xFF;
    longtemp=longtemp/tempcx;
    longtemp=longtemp*8*1024;
    tempax=(longtemp)/HDE;
    if(tempax*HDE<longtemp){
      tempax=tempax+1;
    }else{
      tempax=tempax;
    }
    tempbx=pushbx;
    tempah=((tempax&0xFF00)>>8)&0x01F;
    tempbh=tempbh|tempah;
    tempah=tempax&0xFF;
  }

  SetReg1(Part2Port,0x44,tempah);
  tempah=tempbh;
  SetRegANDOR(Part2Port,0x45,~0x03F,tempah);

  if(IF_DEF_HiVision==1) {
    if(!(VBInfo&SetInSlaveMode)) {
      SetReg1(Part2Port,0x0B,0x00);
    }
  }

  if(VBInfo&SetCRT2ToTV){
    return;
  }
  tempah=0x01;
  if(LCDResInfo==Panel1280x1024){
    if(ModeType==ModeEGA){
      if(VGAHDE>=1024){
        tempah=0x02;
      }
    }
  }
  SetReg1(Part2Port,0x0B,tempah);

  tempbx=HDE-1;         /* RHACTE=HDE-1 */
  tempah=tempbx&0xFF;
  SetReg1(Part2Port,0x2C,tempah);
  tempah=(tempbx&0xFF00)>>8;
  tempah=(tempah<<4)&0xFF;
  SetRegANDOR(Part2Port,0x2B,~0x0F0,tempah);

  tempbx=VDE-1;         /* RTVACTEO=(VDE-1)&0xFF */
  tempah=tempbx&0xFF;
  SetReg1(Part2Port,0x03,tempah);
  tempah=((tempbx&0xFF00)>>8)&0x07;
  SetRegANDOR(Part2Port,0x0C,~0x07,tempah);

  tempcx=VT-1;
  tempah=tempcx&0xFF;   /* RVTVT=VT-1 */
  SetReg1(Part2Port,0x19,tempah);
  tempah=(tempcx&0xFF00)>>8;
  tempah=(tempah<<5)&0xFF;
  if(LCDInfo&LCDRGB18Bit){
    tempah=tempah|0x10;
  }
  SetReg1(Part2Port,0x1A,tempah);

  tempcx++;
  if(LCDResInfo==Panel1024x768){
    tempbx=768;
  }else{
    tempbx=1024;
  }

  if(tempbx==VDE){
    tempax=1;
  }else{
    tempax=tempbx;
    tempax=(tempax-VDE)>>1;
  }
  tempcx=tempcx-tempax; /* lcdvdes */
  tempbx=tempbx-tempax; /* lcdvdee */

  tempah=tempcx&0xFF;   /* RVEQ1EQ=lcdvdes */
  SetReg1(Part2Port,0x05,tempah);
  tempah=tempbx&0xFF;   /* RVEQ2EQ=lcdvdee */
  SetReg1(Part2Port,0x06,tempah);

  tempah=(tempbx&0xFF00)>>8;
  tempah=(tempah<<3)&0xFF;
  tempah=tempah|((tempcx&0xFF00)>>8);
  SetReg1(Part2Port,0x02,tempah);

  tempcx=(VT-VDE)>>4;   /* (VT-VDE)>>4 */
  tempbx=(VT+VDE)>>1;
  tempah=tempbx&0xFF;   /* RTVACTEE=lcdvrs */
  SetReg1(Part2Port,0x04,tempah);

 tempah=(tempbx&0xFF00)>>8;
 tempah=(tempah<<4)&0xFF;
 tempbx=tempbx+tempcx+1;
 tempbl=(tempbx&0x0F);
 tempah=tempah|tempbl;  /* RTVACTSO=lcdvrs&0x700>>4+lcdvre */
 SetReg1(Part2Port,0x01,tempah);

 tempah=GetReg1(Part2Port,0x09);
 tempah=tempah&0xF0;
 SetReg1(Part2Port,0x09,tempah);

 tempah=GetReg1(Part2Port,0x0A);
 tempah=tempah&0xF0;
 SetReg1(Part2Port,0x0A,tempah);

 tempcx=(HT-HDE)>>2;    /* (HT-HDE)>>2     */
 tempbx=(HDE+7);        /* lcdhdee         */
 tempah=tempbx&0xFF;    /* RHEQPLE=lcdhdee */
 SetReg1(Part2Port,0x23,tempah);
 tempah=(tempbx&0xFF00)>>8;
 SetRegANDOR(Part2Port,0x25,~0x0F,tempah);

 SetReg1(Part2Port,0x1F,0x07);  /* RHBLKE=lcdhdes */
 tempah=GetReg1(Part2Port,0x20);
 tempah=tempah&0x0F;
 SetReg1(Part2Port,0x20,tempah);

 tempbx=tempbx+tempcx;
 tempah=tempbx&0xFF;            /* RHBURSTS=lcdhrs */
 SetReg1(Part2Port,0x1C,tempah);
 tempah=(tempbx&0xFF00)>>8;
 tempah=(tempah<<4)&0xFF;
 SetRegANDOR(Part2Port,0x1D,~0x0F0,tempah);

 tempbx=tempbx+tempcx;
 tempah=tempbx&0xFF;            /* RHSYEXP2S=lcdhre */
 SetReg1(Part2Port,0x21,tempah);

 tempah=GetReg1(Part2Port,0x17);
 tempah=tempah&0xFB;
 SetReg1(Part2Port,0x17,tempah);

 tempah=GetReg1(Part2Port,0x18);
 tempah=tempah&0xDF;
 SetReg1(Part2Port,0x18,tempah);
 return;
}

VOID SetGroup3(USHORT  BaseAddr,ULONG ROMAddr)
{
  USHORT i;
  USHORT *tempdi;
  USHORT  Part3Port;
  USHORT  modeflag;
  Part3Port=BaseAddr+IND_SIS_CRT2_PORT_12;
/* ynlai begin */
  SetReg1(Part3Port,0x00,0x00);
  if(VBInfo&SetPALTV){
    SetReg1(Part3Port,0x13,0xFA);
    SetReg1(Part3Port,0x14,0xC8);
  }
  else {
    SetReg1(Part3Port,0x13,0xF6);
    SetReg1(Part3Port,0x14,0xBF);
  }
  if(IF_DEF_HiVision==1) {
    tempdi=HiTVGroup3Data;
    if(SetFlag&TVSimuMode) {
      tempdi=HiTVGroup3Simu;
      modeflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   
      if(!(modeflag&Charx8Dot)) {
        tempdi=HiTVGroup3Text;
      }
    }
    for(i=0;i<=0x3E;i++){
       SetReg1(Part3Port,i,tempdi[i]);
    }
  }
/* ynlai end   */
  return;
}

VOID SetGroup4(USHORT  BaseAddr,ULONG ROMAddr,USHORT ModeNo)
{
  USHORT  Part4Port;
  USHORT tempax,tempah,tempcx,tempbx,tempbh,tempch,tempmodeflag;
  long int tempebx,tempeax,templong;
  Part4Port=BaseAddr+IND_SIS_CRT2_PORT_14;

  tempax=0x0c;
  if(VBInfo&SetCRT2ToTV){
    if(VBInfo&SetInSlaveMode){
      if(!(SetFlag&TVSimuMode)){
        SetFlag=SetFlag|RPLLDIV2XO;
        tempax=tempax|0x04000;
      }
    }else{
      SetFlag=SetFlag|RPLLDIV2XO;
      tempax=tempax|0x04000;
    }
  }

  if(LCDResInfo!=Panel1024x768){
    tempax=tempax|0x08000;
  }
  tempah=(tempax&0xFF00)>>8;
  SetReg1(Part4Port,0x0C,tempah);

  tempah=RVBHCFACT;
  SetReg1(Part4Port,0x13,tempah);

  tempbx=RVBHCMAX;
  tempah=tempbx&0xFF;
  SetReg1(Part4Port,0x14,tempah);
  tempbh=(((tempbx&0xFF00)>>8)<<7)&0xFF;

  tempcx=VGAHT-1;
  tempah=tempcx&0xFF;
  SetReg1(Part4Port,0x16,tempah);
  tempch=(((tempcx&0xFF00)>>8)<<3)&0xFF;
  tempbh=tempbh|tempch;

  tempcx=VGAVT-1;
  if(!(VBInfo&SetCRT2ToTV)){
    tempcx=tempcx-5;
  }
  tempah=tempcx&0xFF;
  SetReg1(Part4Port,0x17,tempah);
  tempbh=tempbh|((tempcx&0xFF00)>>8);
  tempah=tempbh;
  SetReg1(Part4Port,0x15,tempah);

  tempcx=VBInfo;
  tempbx=VGAHDE;
  tempmodeflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
  if(tempmodeflag&HalfDCLK){
    tempbx=tempbx>>1;
  }

/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    tempah=0xA0;
    if(tempbx!=1024) {
       tempah=0xC0;
       if(tempbx!=1280) tempah=0;
    }
  }
  else {
    if(VBInfo&SetCRT2ToLCD){
      tempah=0;
      if(tempbx>800){
        tempah=0x60;
      }
    }else{
      tempah=0x080;
    }
  }
/* ynlai end   */
  if(LCDResInfo!=Panel1280x1024){
    tempah=tempah|0x0A;
  }

  SetRegANDOR(Part4Port,0x0E,~0xEF,tempah);

  tempebx=VDE;

/* ynlai begin */
  if(IF_DEF_HiVision==1) {
    if(!(tempah&0xE0)) tempbx=tempbx>>1;
  }
/* ynlai end   */

  tempcx=RVBHRS;
  tempah=tempcx&0xFF;
  SetReg1(Part4Port,0x18,tempah);

  tempeax=VGAVDE;
  tempcx=tempcx|0x04000;
  tempeax=tempeax-tempebx;
  if(tempeax<0){
    tempcx=tempcx^(0x04000);
    tempeax=VGAVDE;
  }

  templong=(tempeax*256*1024)/tempebx;
  if(tempeax*256*1024-templong*tempebx>0){
    tempebx=templong+1;
  }else{
    tempebx=templong;
  }


  tempah=tempebx&0xFF;
  SetReg1(Part4Port,0x1B,tempah);
  tempah=(tempebx&0xFF00)>>8;
  SetReg1(Part4Port,0x1A,tempah);
  tempebx=tempebx>>16;
  tempah=tempebx&0xFF;
  tempah=(tempah<<4)&0xFF;
  tempah=tempah|((tempcx&0xFF00)>>8);
  SetReg1(Part4Port,0x19,tempah);

  SetCRT2VCLK(BaseAddr,ROMAddr,ModeNo); 
}

VOID SetCRT2VCLK(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo)
{
  USHORT vclk2ptr;
  USHORT tempah,temp1;
  USHORT  Part4Port;

  Part4Port=BaseAddr+IND_SIS_CRT2_PORT_14;
  vclk2ptr=GetVCLK2Ptr(ROMAddr,ModeNo);
  SetReg1(Part4Port,0x0A,0x01);
  tempah=*((UCHAR *)(ROMAddr+vclk2ptr+0x01));   /* di+1 */
  SetReg1(Part4Port,0x0B,tempah);
  tempah=*((UCHAR *)(ROMAddr+vclk2ptr+0x00));   /* di */
  SetReg1(Part4Port,0x0A,tempah);
  SetReg1(Part4Port,0x12,0x00);
  tempah=0x08;
  if(VBInfo&SetCRT2ToRAMDAC){
    tempah=tempah|0x020;
  }
  temp1=GetReg1(Part4Port,0x12);
  tempah=tempah|temp1;
  SetReg1(Part4Port,0x12,tempah);
}

VOID SetGroup5(USHORT  BaseAddr,ULONG ROMAddr)
{
  USHORT  Part5Port;
  USHORT Pindex,Pdata;
  Part5Port=BaseAddr+IND_SIS_CRT2_PORT_14+2;
  Pindex=Part5Port;
  Pdata=Part5Port+1;
  if(ModeType==ModeVGA){
    if(!(VBInfo&(SetInSlaveMode|LoadDACFlag|CRT2DisplayFlag))){
      EnableCRT2();
      LoadDAC2(ROMAddr,Part5Port);
    }
  }
  return;
}

VOID EnableCRT2()
{
  USHORT temp1;
  temp1=GetReg1(P3c4,0x1E);
  temp1=temp1|0x20;
  SetReg1(P3c4,0x1E,temp1);     /* SR 1E */
}

VOID LoadDAC2(ULONG ROMAddr,USHORT Part5Port)
{
   USHORT data,data2;
   USHORT time,i,j,k;
   USHORT m,n,o;
   USHORT si,di,bx,dl;
   USHORT al,ah,dh;
   USHORT *table=0;
   USHORT Pindex,Pdata;
   Pindex=Part5Port;
   Pdata=Part5Port+1;
   data=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));
   data=data&DACInfoFlag;
   time=64;
   if(data==0x00) table=MDA_DAC;
   if(data==0x08) table=CGA_DAC;
   if(data==0x10) table=EGA_DAC;
   if(data==0x18) {
     time=256;
     table=VGA_DAC;
   }
   if(time==256) j=16;
   else j=time;

   SetReg3(Pindex,0x00);

   for(i=0;i<j;i++) {
      data=table[i];
      for(k=0;k<3;k++) {
        data2=0;
        if(data&0x01) data2=0x2A;
        if(data&0x02) data2=data2+0x15;
        SetReg3(Pdata,data2);
        data=data>>2;
      }
   }

   if(time==256) {
      for(i=16;i<32;i++) {
         data=table[i];
         for(k=0;k<3;k++) SetReg3(Pdata,data);
      }
      si=32;
      for(m=0;m<9;m++) {
         di=si;
         bx=si+0x04;
         dl=0;
         for(n=0;n<3;n++) {
            for(o=0;o<5;o++) {
              dh=table[si];
              ah=table[di];
              al=table[bx];
              si++;
              WriteDAC2(Pdata,dl,ah,al,dh);
            }         /* for 5 */
            si=si-2;
            for(o=0;o<3;o++) {
              dh=table[bx];
              ah=table[di];
              al=table[si];
              si--;
              WriteDAC2(Pdata,dl,ah,al,dh);
            }         /* for 3 */
            dl++;
         }            /* for 3 */
         si=si+5;
      }               /* for 9 */
   }
}

VOID WriteDAC2(USHORT Pdata,USHORT dl, USHORT ah, USHORT al, USHORT dh)
{
  USHORT temp;
  USHORT bh,bl;

  bh=ah;
  bl=al;
  if(dl!=0) {
    temp=bh;
    bh=dh;
    dh=temp;
    if(dl==1) {
       temp=bl;
       bl=dh;
       dh=temp;
    }
    else {
       temp=bl;
       bl=bh;
       bh=temp;
    }
  }
  SetReg3(Pdata,(USHORT)dh);
  SetReg3(Pdata,(USHORT)bh);
  SetReg3(Pdata,(USHORT)bl);
}

VOID LockCRT2(USHORT BaseAddr)
{
  USHORT  Part1Port;
  USHORT  Part4Port;
  USHORT temp1;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  Part4Port=BaseAddr+IND_SIS_CRT2_PORT_14;
  temp1=GetReg1(Part1Port,0x24);
  temp1=temp1&0xFE;
  SetReg1(Part1Port,0x24,temp1);
}

VOID SetLockRegs()
{
  USHORT temp1;

  if((VBInfo&SetInSlaveMode)&&(!(VBInfo&SetCRT2ToRAMDAC))){
    VBLongWait(); 
    temp1=GetReg1(P3c4,0x32);
    temp1=temp1|0x20;
    SetReg1(P3c4,0x32,temp1);
    VBLongWait(); 
  }
}

VOID EnableBridge(USHORT BaseAddr)
{
  USHORT part2_02,part2_05;
  USHORT Part2Port;
  Part2Port=BaseAddr+IND_SIS_CRT2_PORT_10;

  if(IF_DEF_LVDS==0) {
    part2_02=(UCHAR)GetReg1(Part2Port,0x02);
    part2_05=(UCHAR)GetReg1(Part2Port,0x05);
    SetReg1(Part2Port,0x02,0x38);
    SetReg1(Part2Port,0x05,0xFF);
    LongWait();
    SetRegANDOR(Part2Port,0x00,~0x0E0,0x020);
 /*   WaitVBRetrace(BaseAddr);  */
    SetReg1(Part2Port,0x02,part2_02);
    SetReg1(Part2Port,0x05,part2_05);
  }
  else {
    EnableCRT2();
    UnLockCRT2(BaseAddr);
/*       SetRegANDOR(Part1Port,0x02,~0x040,0x0);   */    
  }
}

VOID GetVBInfo(USHORT BaseAddr,ULONG ROMAddr)
{
  USHORT flag1,tempbx,tempbl,tempbh,tempah,temp;

  SetFlag=0;
  tempbx=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
  tempbl=tempbx&ModeInfoFlag;
  ModeType=tempbl;
  tempbx=0;
  flag1=GetReg1(P3c4,0x38);     /* call BridgeisOn */
  if(IF_DEF_LVDS==0) {          /* for 301 */
   if(!(flag1&0x20)){
     VBInfo=CRT2DisplayFlag;
     return;
   }
  }
  tempbl=GetReg1(P3d4,0x30);
  tempbh=GetReg1(P3d4,0x31);

  tempah=((SetCHTVOverScan>>8)|(SetInSlaveMode>>8)|(DisableCRT2Display>>8));
  tempah=tempah^0xFF; 
  tempbh=tempbh&tempah;   
/*  ynlai begin */
  if(IF_DEF_LVDS==1){  /* for LVDS */
    if(IF_DEF_CH7005==1) temp=SetCRT2ToLCD|SetCRT2ToTV;
    else temp=SetCRT2ToLCD;
  }
  else {
    if(IF_DEF_HiVision==1) temp=0xFC;
    else temp=0x7C;
  }
  if(!(tempbl&temp)) {
    VBInfo=DisableCRT2Display;
    return;
  }
/*  ynlai end */
  if(IF_DEF_LVDS==0) {
    if(tempbl&SetCRT2ToRAMDAC){
      tempbl=tempbl&(SetCRT2ToRAMDAC|SwitchToCRT2|SetSimuScanMode);
    }else if(tempbl&SetCRT2ToLCD){
      tempbl=tempbl&(SetCRT2ToLCD|SwitchToCRT2|SetSimuScanMode);
    }else if(tempbl&SetCRT2ToSCART){
      tempbl=tempbl&(SetCRT2ToSCART|SwitchToCRT2|SetSimuScanMode);
      tempbh=tempbh|(SetPALTV>>8);
    }else if(tempbl&SetCRT2ToHiVisionTV){
      tempbl=tempbl&(SetCRT2ToHiVisionTV|SwitchToCRT2|SetSimuScanMode);
/* ynlai begin */
      tempbh=tempbh|(SetPALTV>>8);
/* ynlai end */
    }
  }
  else {
    if(IF_DEF_CH7005==1) {
      if(tempbl&SetCRT2ToTV) 
        tempbl=tempbl&(SetCRT2ToTV|SwitchToCRT2|SetSimuScanMode);
    } 
    if(tempbl&SetCRT2ToLCD) 
      tempbl=tempbl&(SetCRT2ToLCD|SwitchToCRT2|SetSimuScanMode);
  }
  tempah=GetReg1(P3d4,0x31);
  if(tempah&(CRT2DisplayFlag>>8)){
    if(!(tempbl&(SwitchToCRT2|SetSimuScanMode))){
      tempbx=SetSimuScanMode|CRT2DisplayFlag;
      tempbh=((tempbx&0xFF00)>>8);
      tempbl=tempbx&0xFF;
    }
  }
  if(!(tempbh&(DriverMode>>8))){
    tempbl=tempbl|SetSimuScanMode;
  }
  VBInfo=tempbl|(tempbh<<8);
  if(!(VBInfo&SetSimuScanMode)){
    if(!(VBInfo&SwitchToCRT2)){
      if(BridgeIsEnable(BaseAddr)){
        if(BridgeInSlave()){
          VBInfo=VBInfo|SetSimuScanMode;
        }
      }
    }
    else {
      flag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));      /* si+St_ModeFlag */
      if(!(flag1&CRT2Mode)) {
        VBInfo=VBInfo|SetSimuScanMode;
      }
    }
  }
  if(!(VBInfo&DisableCRT2Display)) {
    if(VBInfo&DriverMode) {
      if(VBInfo&SetSimuScanMode) {
        flag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
        if(!(flag1&CRT2Mode)) {
          VBInfo=VBInfo|SetInSlaveMode;
        }
      } 
    }
    else {
      VBInfo=VBInfo|SetSimuScanMode;
      if(IF_DEF_LVDS==0) {
        if(VBInfo&SetCRT2ToTV) {
          if(!(VBInfo&SetNotSimuMode))  SetFlag=SetFlag|TVSimuMode;
        }
      }
    }
  } 
  if(IF_DEF_CH7005==1) {
    tempah=GetReg1(P3d4,0x35);
    if(tempah&TVOverScan) VBInfo=VBInfo|SetCHTVOverScan; 
  }
}

BOOLEAN BridgeIsEnable(USHORT BaseAddr)
{
  USHORT flag1;
  USHORT  Part1Port;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;

  if(IF_DEF_LVDS==1){
    return 1;
  }
  flag1=GetReg1(P3c4,0x38);                   /* call BridgeisOn */
  if(!(flag1&0x20)){ return 0;}
  flag1=GetReg1(Part1Port,0x0);
  if(flag1&0x0a0){
    return 1;
  }else{
    return 0;
  }
}

BOOLEAN BridgeInSlave()
{
  USHORT flag1;
  flag1=GetReg1(P3d4,0x31);
  if(flag1&(SetInSlaveMode>>8)){
    return 1;
  }else{
    return 0;
  }
}

BOOLEAN GetLCDResInfo(ULONG ROMAddr,USHORT P3d4)
{
  USHORT tempah,tempbh,tempflag;        

  tempah=(UCHAR)GetReg1(P3d4,0x36);
  tempbh=tempah;
  tempah=tempah&0x0F;
/*  if(tempah!=0) tempah--; */
  if(tempah>Panel1280x1024) tempah=0;
  LCDResInfo=tempah;
  tempbh=tempbh>>4;
  LCDTypeInfo=tempbh;

  tempah=(UCHAR)GetReg1(P3d4,0x37);
  LCDInfo=tempah;

  if(IF_DEF_LVDS==1){
    tempflag=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));    /* si+St_ModeFlag  */
    if(tempflag&HalfDCLK){
      if(IF_DEF_TRUMPION==0){
        if(!(LCDInfo&LCDNonExpanding)){
          if(LCDResInfo==Panel1024x768){
            tempflag=*((UCHAR *)(ROMAddr+ModeIDOffset+0x09));/*si+Ext_ResInfo*/
            if(tempflag==4){ /*512x384  */
              SetFlag=SetFlag|EnableLVDSDDA;
            }
          }else{
            if(LCDResInfo==Panel800x600){
              tempflag=*((UCHAR*)(ROMAddr+ModeIDOffset+0x09));/*si+Ext_ResInfo*/
              if(tempflag==3){ /*400x300  */
                SetFlag=SetFlag|EnableLVDSDDA;
              } 
            }
          }
        }else{
          SetFlag=SetFlag|EnableLVDSDDA;
        }
      }else{
        SetFlag=SetFlag|EnableLVDSDDA;
      }
    }
  }

  if(!(VBInfo&SetCRT2ToLCD)){
    return 1;
  }
  if(!(VBInfo&(SetSimuScanMode|SwitchToCRT2))){
    return 1;
  }
  if(VBInfo&SetInSlaveMode){
    if(VBInfo&SetNotSimuMode){
      SetFlag=SetFlag|LCDVESATiming;
    }
  }else{
    SetFlag=SetFlag|LCDVESATiming;
  }
  return 1;
}

VOID PresetScratchregister(USHORT P3d4)
{
  SetReg1(P3d4,0x37,0x00);
}

BOOLEAN GetLCDDDCInfo(ScrnInfoPtr pScrn)
{
  USHORT tempah;
/*tempah=(HwDeviceExtension->usLCDType);// set in sisv.c                                                 */
  tempah=1;
  SetReg1(P3d4,0x36,tempah);  /* cr 36 0:no LCD 1:1024x768 2:1280x1024 */
  if(tempah>0) return 1;
  else return 0;
}

VOID SetTVSystem()
{
  USHORT tempah;
  tempah=GetReg1(P3c4,0x38);           /* SR 38                          */
  tempah=tempah&0x01;                  /* get SR 38 D0 TV Type Selection */
                                       /* 0:NTSC 1:PAL                   */
  SetRegANDOR(P3d4,0x31,~0x01,tempah); /* set CR 31 D0= SR 38 D0         */
  return;
}

VOID LongWait()
{
  USHORT i;
  for(i=0; i<0xFFFF; i++) {
     if(!(inSISREG(P3da) & 0x08))
       break;
  }
  for(i=0; i<0xFFFF; i++) {
     if((inSISREG(P3da) & 0x09) == 9)
       break;
  } 
}

VOID VBLongWait()
{
  USHORT regsr1f,tempah,temp;

  regsr1f=GetReg1(P3c4,0x1F);
  tempah=regsr1f&(~0xC0);
  SetReg1(P3c4,0x1F,tempah);

  for(temp=1;temp>0;){
     temp=GetReg2(P3da);
     temp=temp&0x08;
  }
  for(;temp==0;){
     temp=GetReg2(P3da);
     temp=temp&0x08;
  }

  SetReg1(P3c4,0x1F,regsr1f);
  return;
}

BOOLEAN WaitVBRetrace(USHORT  BaseAddr)
{
  USHORT temp;
  USHORT Part1Port;
  Part1Port=BaseAddr+IND_SIS_CRT2_PORT_04;
  temp=GetReg1(Part1Port,0x00);
  if(!(temp&0x80)){
    return 0;
  }

  for(temp=0;temp==0;){
     temp=GetReg1(Part1Port,0x25);
     temp=temp&0x01;
  }
  for(;temp>0;){
     temp=GetReg1(Part1Port,0x25);
     temp=temp&0x01;
  }
  return 1;
}


VOID ModCRT1CRTC(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT OldREFIndex,temp,tempah,i,modeflag1;

  OldREFIndex=(USHORT)REFIndex;
  temp=GetLVDSCRT1Ptr(ROMAddr,ModeNo);
  if(temp==0){
    REFIndex=OldREFIndex;
    return;
  }
  tempah=(UCHAR)GetReg1(P3d4,0x11);/*unlock cr0-7  */
  tempah=tempah&0x7F;
  SetReg1(P3d4,0x11,tempah);
  tempah=*((UCHAR *)(ROMAddr+REFIndex));
  SetReg1(P3d4,0x0,tempah);
  REFIndex++;
  for(i=0x02;i<=0x05;REFIndex++,i++){
    tempah=*((UCHAR *)(ROMAddr+REFIndex));
    SetReg1(P3d4,i,tempah); 
  }
  for(i=0x06;i<=0x07;REFIndex++,i++){
    tempah=*((UCHAR *)(ROMAddr+REFIndex));
    SetReg1(P3d4,i,tempah); 
  }
  for(i=0x10;i<=0x11;REFIndex++,i++){
    tempah=*((UCHAR *)(ROMAddr+REFIndex));
    SetReg1(P3d4,i,tempah); 
  }
  for(i=0x15;i<=0x16;REFIndex++,i++){
    tempah=*((UCHAR *)(ROMAddr+REFIndex));
    SetReg1(P3d4,i,tempah); 
  }

  for(i=0x0A;i<=0x0C;REFIndex++,i++){
    tempah=*((UCHAR *)(ROMAddr+REFIndex));
    SetReg1(P3c4,i,tempah); 
  }
  tempah=*((UCHAR *)(ROMAddr+REFIndex));
  tempah=tempah&0x0E0;
  SetReg1(P3c4,0x0E,tempah);

  tempah=*((UCHAR *)(ROMAddr+REFIndex));
  tempah=tempah&0x01;
  tempah=tempah<<5;
  modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag */
  if(modeflag1&DoubleScanMode){
    tempah=tempah|0x080;
  }
  SetRegANDOR(P3d4,0x09,~0x020,tempah);
  REFIndex=OldREFIndex;
  return; 
}

VOID SetCRT2ECLK(ULONG ROMAddr, USHORT ModeNo)
{
  USHORT OldREFIndex,tempah,tempal;
  USHORT P3cc=P3c9+3;

  OldREFIndex=(USHORT)REFIndex;
  if(IF_DEF_TRUMPION==0){  /*no trumpion  */
    tempal=GetReg2(P3cc);
    tempal=tempal&0x0C;
    REFIndex=GetVCLK2Ptr(ROMAddr,ModeNo);
  }else{  /*trumpion  */
    SetFlag=SetFlag&(~ProgrammingCRT2);
    tempal=*((UCHAR *)(ROMAddr+REFIndex+0x03));   /*&di+Ext_CRTVCLK  */
    tempal=tempal&0x03F;
    if(tempal==0x02){ /*31.5MHz  */
      REFIndex=REFIndex-Ext2StructSize;
    }
    REFIndex=GetVCLKPtr(ROMAddr,ModeNo);
    SetFlag=SetFlag|ProgrammingCRT2;
  }
  tempal=0x02B;
  if(!(VBInfo&SetInSlaveMode)){
    tempal=tempal+3; 
  }
  SetReg1(P3c4,0x05,0x86);
  tempah=*((UCHAR *)(ROMAddr+REFIndex));  
  SetReg1(P3c4,tempal,tempah);
  tempah=*((UCHAR *)(ROMAddr+REFIndex+1));
  tempal++; 
  SetReg1(P3c4,tempal,tempah);
  tempal++;
  SetReg1(P3c4,tempal,0x80);
  REFIndex=OldREFIndex;
  return;
}

USHORT GetLVDSDesPtr(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempcl,tempbx,tempal,tempptr,LVDSDesPtrData;
  USHORT Flag;

  Flag=1;
  tempbx=0;
  if(IF_DEF_CH7005==1) {
    if(!(VBInfo&SetCRT2ToLCD)) {
      Flag=0;
      tempbx=32;
      if(VBInfo&SetPALTV) tempbx=tempbx+2;
      if(VBInfo&SetCHTVOverScan) tempbx=tempbx+1;
    }
  } 
  tempcl=LVDSDesDataLen;
  if(Flag) {
    tempbx=LCDTypeInfo;
    if(LCDInfo&LCDNonExpanding){
      tempbx=tempbx+16;
    }
  } 
  if(ModeNo<=0x13) tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));   /* si+St_CRT2CRTC  */
  else  tempal=*((UCHAR *)(ROMAddr+REFIndex+4));    /*di+Ext_CRT2CRTC  */
  tempal=tempal&0x1F;
  tempal=tempal*tempcl;
  tempbx=tempbx<<1;
  LVDSDesPtrData=*((USHORT *)(ROMAddr+ADR_LVDSDesPtrData));
  tempptr=*((USHORT *)(ROMAddr+LVDSDesPtrData+tempbx));
  tempptr=tempptr+tempal;
  return(tempptr);
  
}

BOOLEAN GetLVDSCRT1Ptr(ULONG ROMAddr,USHORT ModeNo)
{
  USHORT tempal,tempbx,modeflag1; 
  USHORT LVDSCRT1DataPtr,Flag; 

  if(!(VBInfo&SetInSlaveMode)){
/*       return 0;  */
  }
  Flag=1;
  tempbx=0;
  if(IF_DEF_CH7005==1) {
    if(!(VBInfo&SetCRT2ToLCD)) {
      Flag=0;
      tempbx=12;
      if(VBInfo&SetPALTV) tempbx=tempbx+2;
      if(VBInfo&SetCHTVOverScan) tempbx=tempbx+1;	
    }
  }
  if(Flag) {
    tempbx=LCDResInfo;
    tempbx=tempbx-Panel800x600;
    if(LCDInfo&LCDNonExpanding) tempbx=tempbx+6;
    modeflag1=*((USHORT *)(ROMAddr+ModeIDOffset+0x01));   /* si+St_ModeFlag  */
    if(modeflag1&HalfDCLK) tempbx=tempbx+3;
  }
  if(ModeNo<=0x13) tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));   /* si+St_CRT2CRTC  */
  else tempal=*((UCHAR *)(ROMAddr+REFIndex+4));    /*di+Ext_CRT2CRTC  */
  tempal=tempal&0x3F;

  tempbx=tempbx<<1;
  LVDSCRT1DataPtr=*((USHORT *)(ROMAddr+ADR_LVDSCRT1DataPtr));
  REFIndex=*((USHORT *)(ROMAddr+LVDSCRT1DataPtr+tempbx));
  tempal=tempal*LVDSCRT1Len;
  REFIndex=REFIndex+tempal;
  return 1;

}

VOID SetCHTVReg(ULONG ROMAddr,USHORT ModeNo) 
{
  USHORT old_REFIndex,temp,tempbx,tempcl;

  old_REFIndex=(USHORT)REFIndex;                /*push di  */
  GetCHTVRegPtr(ROMAddr,ModeNo);

  if(VBInfo&SetPALTV) {
    SetCH7005(0x4304); 
    SetCH7005(0x6909); 
  } 
  else {
    SetCH7005(0x0304); 
    SetCH7005(0x7109); 
  }

  temp=*((USHORT *)(ROMAddr+REFIndex+0x00));
  tempbx=((temp&0x00FF)<<8)|0x00;
  SetCH7005(tempbx); 
  temp=*((USHORT *)(ROMAddr+REFIndex+0x01));
  tempbx=((temp&0x00FF)<<8)|0x07;
  SetCH7005(tempbx);	
  temp=*((USHORT *)(ROMAddr+REFIndex+0x02));
  tempbx=((temp&0x00FF)<<8)|0x08;
  SetCH7005(tempbx);	
  temp=*((USHORT *)(ROMAddr+REFIndex+0x03));
  tempbx=((temp&0x00FF)<<8)|0x0A;
  SetCH7005(tempbx);	
  temp=*((USHORT *)(ROMAddr+REFIndex+0x04));
  tempbx=((temp&0x00FF)<<8)|0x0B;
  SetCH7005(tempbx);	

  SetCH7005(0x2801);	
  SetCH7005(0x3103);	
  SetCH7005(0x003D);	
  SetCHTVRegANDOR(0x0010,0x1F);	
  SetCHTVRegANDOR(0x0211,0xF8);
  SetCHTVRegANDOR(0x001C,0xEF);
  
  if(!(VBInfo&SetPALTV)) {
    if(ModeNo<=0x13) tempcl=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));   /* si+St_CRT2CRTC */
    else tempcl=*((UCHAR *)(ROMAddr+REFIndex+4));    /* di+Ext_CRT2CRTC */
    tempcl=tempcl&0x3F;
    if(VBInfo&SetCHTVOverScan) {
      if(tempcl==0x04) {   /* 640x480   underscan */
        SetCHTVRegANDOR(0x0020,0xEF);	
        SetCHTVRegANDOR(0x0121,0xFE);	
      }
      else {
        if(tempcl==0x05) {    /* 800x600  underscan */    
          SetCHTVRegANDOR(0x0118,0xF0);	
          SetCHTVRegANDOR(0x0C19,0xF0);	
          SetCHTVRegANDOR(0x001A,0xF0);	
          SetCHTVRegANDOR(0x001B,0xF0);	
          SetCHTVRegANDOR(0x001C,0xF0);	
          SetCHTVRegANDOR(0x001D,0xF0);	
          SetCHTVRegANDOR(0x001E,0xF0);	
          SetCHTVRegANDOR(0x001F,0xF0);	
          SetCHTVRegANDOR(0x0120,0xEF);	
          SetCHTVRegANDOR(0x0021,0xFE);	
        }
      }
    }
    else {
      if(tempcl==0x04) {     /* 640x480   overscan  */
        SetCHTVRegANDOR(0x0020,0xEF);	
        SetCHTVRegANDOR(0x0121,0xFE);	
      }
      else {
        if(tempcl==0x05) {   /* 800x600   overscan */
          SetCHTVRegANDOR(0x0118,0xF0);	
          SetCHTVRegANDOR(0x0F19,0xF0);	
          SetCHTVRegANDOR(0x011A,0xF0);	
          SetCHTVRegANDOR(0x0C1B,0xF0);	
          SetCHTVRegANDOR(0x071C,0xF0);	
          SetCHTVRegANDOR(0x011D,0xF0);	
          SetCHTVRegANDOR(0x0C1E,0xF0);	
          SetCHTVRegANDOR(0x071F,0xF0);	
          SetCHTVRegANDOR(0x0120,0xEF);	
          SetCHTVRegANDOR(0x0021,0xFE);	
        }
      }
    }
  }

  REFIndex=old_REFIndex;   		
}

VOID SetCHTVRegANDOR(USHORT tempax,USHORT tempbh)
{
  USHORT tempal,tempah,tempbl;

  tempal=tempax&0x00FF;
  tempah=(tempax>>8)&0x00FF;
  tempbl=GetCH7005(tempal); 
  tempbl=(((tempbl&tempbh)|tempah)<<8|tempal);  
  SetCH7005(tempbl);  
}

VOID GetCHTVRegPtr(ULONG ROMAddr,USHORT ModeNo) 
{
  USHORT tempbx,tempal,tempcl,CHTVRegDataPtr;

  if(VBInfo&SetCRT2ToTV) {
    tempbx=0;
    if(VBInfo&SetPALTV) tempbx=tempbx+2;
    if(VBInfo&SetCHTVOverScan) tempbx=tempbx+1;

    if(ModeNo<=0x13) tempal=*((UCHAR *)(ROMAddr+ModeIDOffset+0x04));   /* si+St_CRT2CRTC */
    else tempal=*((UCHAR *)(ROMAddr+REFIndex+4));    /* di+Ext_CRT2CRTC */
    tempal=tempal&0x3F;
    
    tempcl=CHTVRegDataLen;
    tempal=tempal*tempcl;
    tempbx=tempbx<<1;

    CHTVRegDataPtr=*((USHORT *)(ROMAddr+ADR_CHTVRegDataPtr));
    REFIndex=*((USHORT *)(ROMAddr+CHTVRegDataPtr+tempbx));
    REFIndex=REFIndex+tempal;
  }	 
}

VOID SetCH7005(USHORT tempbx)
{
  USHORT tempah,temp;

  DDC_Port=0x3c4;
  DDC_Index=0x11;
  DDC_DataShift=0x00;
  DDC_DeviceAddr=0xEA;

  SetSwitchDDC2();  
  SetStart();
  tempah=DDC_DeviceAddr;
  temp=WriteDDC2Data(tempah); 
  tempah=tempbx&0x00FF; 
  temp=WriteDDC2Data(tempah);  
  tempah=(tempbx&0xFF00)>>8; 
  temp=WriteDDC2Data(tempah);  
  SetStop();   
}

USHORT GetCH7005(USHORT tempbx)
{
  USHORT tempah;

  DDC_Port=0x3c4;
  DDC_Index=0x11;
  DDC_DataShift=0x00;
  DDC_DeviceAddr=0xEA;
  DDC_ReadAddr=tempbx;

  SetSwitchDDC2();  
  SetStart();
  tempah=DDC_DeviceAddr;
  WriteDDC2Data(tempah);
  tempah=DDC_ReadAddr;
  WriteDDC2Data(tempah);

  SetStart();
  tempah=DDC_DeviceAddr;
  tempah=tempah|0x01;
  if(WriteDDC2Data(tempah)) {
  }
  tempah=ReadDDC2Data(tempah);  
  SetStop();   
  return(tempah);
}

VOID SetSwitchDDC2(VOID)
{
  USHORT i;

  SetSCLKHigh();
  for(i=0;i<1000;i++) {
    GetReg1(DDC_Port,0x05);
  }
  SetSCLKLow();
  for(i=0;i<1000;i++) {
    GetReg1(DDC_Port,0x05);
  }
}


VOID SetStart(VOID)
{
  USHORT temp;

  SetSCLKLow();  
  SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x02);     /*  SetSDA(0x01); */
  SetSCLKHigh();    
  SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x00);      /* SetSDA(0x00); */
  SetSCLKHigh();    
}

VOID SetStop(VOID)
{
  SetSCLKLow();  
  SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x00);    /*  SetSDA(0x00); */
  SetSCLKHigh();  
  SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x02);      /* SetSDA(0x01); */
  SetSCLKHigh();    
}

USHORT WriteDDC2Data(USHORT tempax)
{ 
  USHORT i,flag,temp;

  flag=0x80;
  for(i=0;i<8;i++) {  
    SetSCLKLow();  
    if(tempax&flag) {
      SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x02);     
    }
    else {
      SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x00);     
    }
    SetSCLKHigh();    
    flag=flag>>1;
  }
  return(CheckACK());
}

USHORT ReadDDC2Data(USHORT tempax)
{ 
  USHORT i,temp,getdata;

  getdata=0;
  for(i=0;i<8;i++) {  
    getdata=getdata<<1;
    SetSCLKLow();
    SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x02);      
    SetSCLKHigh();
    temp=GetReg1(DDC_Port,DDC_Index);      	
    if(temp&0x02) getdata=getdata|0x01;
  }  
  return(getdata);
}  

VOID SetSCLKLow(VOID)
{
    SetRegANDOR(DDC_Port,DDC_Index,0xFE,0x00);      /* SetSCLKLow()  */
    DDC2Delay(); 
}


VOID SetSCLKHigh(VOID)
{
  USHORT temp;

  SetRegANDOR(DDC_Port,DDC_Index,0xFE,0x01);      /* SetSCLKLow()  */
  do {
    temp=GetReg1(DDC_Port,DDC_Index);
  } while(!(temp&0x01));
  DDC2Delay(); 
}  

VOID DDC2Delay(VOID)
{
  USHORT i;

   for(i=0;i<DDC2DelayTime;i++) {   
    GetReg1(P3c4,0x05); 	
  }
}

USHORT CheckACK(VOID)
{
  USHORT tempah;

  SetSCLKLow();
  SetRegANDOR(DDC_Port,DDC_Index,0xFD,0x02);  
  SetSCLKHigh();
  tempah=GetReg1(DDC_Port,DDC_Index);
  SetSCLKLow();
  if(tempah&0x01) return(1);
  else return(0);
}
