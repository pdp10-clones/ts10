// kdf11_sys.c - KDF11 Series CPU System Support Routines
//
// Copyright (c) 2002, Timothy M. Stark
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// TIMOTHY M STARK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Timothy M Stark shall not
// be used in advertising or otherwise to promote the sale, use or other 
// dealings in this Software without prior written authorization from
// Timothy M Stark.

#include "pdp11/defs.h"
#include "pdp11/kdf11.h"

// *************************************************************
// **************** F11 - KW11L Line Time Clock ****************
// *************************************************************

void f11_ResetClock(F11_CPU *f11)
{
	MAP_IO *io = &f11->ioClock;

	// Reset speedometer.
	f11->clkCount    = 0;
	f11->cpu.opMeter = 0;

	// Reset Unibus registers.
	LTC = LTC_DONE;

	io->CancelInterrupt(io, 0);
}

void f11_Tick(void *dptr)
{
	F11_CPU *f11 = (F11_CPU *)dptr;
	MAP_IO  *io  = &f11->ioClock;

	LTC |= LTC_DONE;
	if (LTC & LTC_IE)
		io->SendInterrupt(io, 0);

	// Once each second.
	if (f11->clkCount++ >= 60) {
		f11->clkCount = 0;
#ifdef DEBUG
		if (dbg_Check(DBG_IPS)) {
			dbg_Printf("%s: Speedometer: %d ips\n",
				f11->cpu.Unit.devName, f11->cpu.opMeter);
			f11->cpu.opMeter = 0;
		}
#else /* DEBUG */
		printf("%s: Speedometer: %d ips\n",
			f11->cpu.Unit.devName, f11->cpu.opMeter);
		f11->cpu.opMeter = 0;
#endif /* DEBUG */
	}
}

int f11_ReadLTC(void *dptr, uint32 pAddr, uint16 *data, uint32 acc)
{
	F11_CPU *f11 = (F11_CPU *)dptr;

	*data = LTC;

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: (R) LTC (%o) => %06o\n",
			f11->cpu.Unit.devName, pAddr, *data);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_WriteLTC(void *dptr, uint32 pAddr, uint16 data, uint32 acc)
{
	F11_CPU *f11 = (F11_CPU *)dptr;
	MAP_IO  *io  = &f11->ioClock;

	if (pAddr & 1)
		return UQ_OK;

	// Update IE or DONE bit on Clock CSR register.
	LTC = (data & LTC_RW) | (LTC & ~LTC_RW);
	if (data & LTC_DONE)
		LTC &= ~LTC_DONE;

	// Unless DONE+IE set, clear interrupt request.
	if (((LTC & LTC_DONE) == 0) || ((LTC & LTC_IE) == 0))
		io->CancelInterrupt(io, 0);

	// Test option for XXDP to pass tests because this
	// emulator is too fast to test line time click!
	if (LTC & LTC_IE)
		io->SendInterrupt(io, 0);

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: (W) LTC (%o) <= %06o (Now: %06o)\n",
			f11->cpu.Unit.devName, pAddr, data, LTC);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_InitTimer(P11_CPU *p11)
{
	F11_CPU   *f11 = (F11_CPU *)p11;
	UQ_IO     *uq  = (UQ_IO *)p11->uqba;
	MAP_IO    *io  = &f11->ioClock;
	CLK_QUEUE *clk = &f11->clkTimer;

	// Initialize KW11L Line Time Clock.
	io->devName      = f11->cpu.Unit.devName;
	io->keyName      = LTC_KEY;
	io->emuName      = "KDF11: Line Time Clock";
	io->emuVersion   = NULL;
	io->Device       = f11;
	io->csrAddr      = LTC_CSRADR;
	io->nRegs        = LTC_NREGS;
	io->nVectors     = LTC_NVECS;
	io->intIPL       = LTC_IPL;
	io->intVector[0] = LTC_VEC;
	io->ReadIO       = f11_ReadLTC;
	io->WriteIO      = f11_WriteLTC;
	uq->Callback->SetMap(uq, io);

	// Reset Line Time Clock.
	f11_ResetClock(f11);

	// Initialize KW11L Tick service.
	clk->Next          = NULL;
	clk->Flags         = CLK_REACTIVE;
	clk->outTimer      = LTC_TICK;
	clk->nxtTimer      = LTC_TICK;
	clk->Device        = f11;
	clk->Execute       = f11_Tick;
	ts10_SetRealTimer(clk);

	return P11_OK;
}

void f11_StartTimer(P11_CPU *p11)
{
	F11_CPU *f11 = (F11_CPU *)p11;

	f11->clkCount        = 0;
	f11->cpu.opMeter     = 0;
	f11->clkTimer.Flags |= CLK_ENABLE;
}

void f11_StopTimer(P11_CPU *p11)
{
	F11_CPU *f11 = (F11_CPU *)p11;

	f11->clkTimer.Flags &= ~CLK_ENABLE;
}

// *************************************************************

extern uint32 p11_dsMask[];

// I/O - CPU Registers
//
// Registers  CSR Addresses  # of Registers
// APRs          772300         16 (0040)
// MMR3          772516          1 (0002)
// SR/MMR0-2     777570          4 (0010)
// APRs          777600         16 (0040)
// CPU           777740         16 (0040)

// CSR Addresses
#define APR1_CSRADR  0772300
#define MMR3_CSRADR  0772516
#define SRMM_CSRADR  0777570
#define APR2_CSRADR  0777600
#define CPU_CSRADR   0777740

// Number of Registers
#define APR1_NREGS   (0100 >> 1)  // 32 Registers
#define MMR3_NREGS   (0002 >> 1)  //  1 Register
#define SRMM_NREGS   (0010 >> 1)  //  4 Registers
#define APR2_NREGS   (0100 >> 1)  // 32 Registers
#define CPU_NREGS    (0040 >> 1)  // 16 Registers

// Memory Management Registers

#ifdef DEBUG
static cchar *smNameR[] = {
	"SR",    // Switch Register
	"SR0",   // Memory Management Register #0 - Status Register
	"SR1",   // Memory Management Register #1 - R+/-R Recovery
	"SR2",   // Memory Management Register #2 - Saved PC
};
static cchar *smNameW[] = {
	"DR",    // Display Register
	"SR0",   // Memory Management Register #0 - Status Register
	"SR1",   // Memory Management Register #1 - R+/-R Recovery
	"SR2",   // Memory Management Register #2 - Saved PC
};
#endif /* DEBUG */

int f11_ReadSRMM(void *dptr, uint32 pAddr, uint16 *data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;
	int     reg  = (pAddr >> 1) & 3;

	switch (reg) {
		case 0: // SR - Switch Register
			*data = SR;
			break;

		case 1: // SR0/MMR0 - Status Register
			*data = MMR0;
			break;

		case 2: // SR1/MMR1 - R+/-R Recovery
			*data = MMR1;
			break;

		case 3: // SR2/MMR2 - Saved PC
			*data = MMR2;
			break;
	}

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: %s (%o) => %06o\n",
			p11->Unit.devName, smNameR[reg], pAddr, *data);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_WriteSRMM(void *dptr, uint32 pAddr, uint16 data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;
	int     reg  = (pAddr >> 1) & 3;

	switch (reg) {
		case 0: // DR - Display Register
			DR = data;
			break;

		case 1: // SR0/MMR0: Status Register
			if (acc == ACC_BYTE) // If byte access, merge data with MMR0.
				data = (pAddr & 1) ? ((data << 8)   | (MMR0 & 0377)) :
				                     ((data & 0377) | (MMR0 & ~0377));
			MMR0 = (data & MMR0_RW) | (MMR0 & ~MMR0_RW);
			break;

		default: // Otherwise - Read-only Registers.
			break;
	}

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: %s (%o) <= %06o (Now: %06o)\n",
			p11->Unit.devName, smNameR[reg], pAddr, data, (reg == 0) ? DR :
			(reg == 1) ? MMR0 : (reg == 2) ? MMR1 : MMR2);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_ReadMMR3(void *dptr, uint32 pAddr, uint16 *data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;

	*data = MMR3;

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: (R) MMR3 (%o) => %06o\n",
			p11->Unit.devName, pAddr, *data);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_WriteMMR3(void *dptr, uint32 pAddr, uint16 data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;

	if ((pAddr & 1) == 0) {
		MMR3 = data & MMR3_RW;
	}

	// Update Data Space.
	DSPACE = GetDSpace(PSW_GETCUR(PSW));

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: (W) MMR3 (%o) <= %06o (Now: %06o)\n",
			p11->Unit.devName, pAddr, data, MMR3);
#endif /* DEBUG */

	return UQ_OK;
}

// PAR/PDR Registers
// 
// 772300-772377  - Kernel Block
// 777600-777677  - User Block

int f11_ReadAPR(void *dptr, uint32 pAddr, uint16 *data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;
	uint16  idx, left;

	// F11 does not support D-space.
	if (pAddr & 020)
		return UQ_NXM;

	// Convert I/O address to APR index.
	idx  = (pAddr >> 1) & 017;
	left = (pAddr >> 5) & 1;
	if ((pAddr & 0100) == 0)
		idx |= 020;
	if (pAddr & 0400)
		idx |= 040;

	// Get PAR or PDR from APR table.
	*data = left ? (APR(idx) >> 16) : APR(idx);

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: APR %03o (%o) => %06o (%06o %06o)\n", 
			p11->Unit.devName, idx, pAddr, *data,
			APR(idx) >> 16, APR(idx) & 0177777);
#endif /* DEBUG */

	return UQ_OK;
}

int f11_WriteAPR(void *dptr, uint32 pAddr, uint16 data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;
	uint16  idx, left, apr;

	// F11 does not support D-space.
	if (pAddr & 020)
		return UQ_NXM;

	// Convert I/O address to APR index.
	idx  = (pAddr >> 1) & 017;
	left = (pAddr >> 5) & 1;
	if ((pAddr & 0100) == 0)
		idx |= 020;
	if (pAddr & 0400)
		idx |= 040;

	if (left) {
		// PAR (Processor Address Register)
		if (acc == ACC_BYTE) {
			// If byte access, merge data with APR entry.
			apr  = APR(idx) >> 16;
			data = (pAddr & 1) ? ((data << 8)   | (apr & 0377)) :
		   	                  ((data & 0377) | (apr & ~0377));
		}
		APR(idx) = (data << 16) | (APR(idx) & (0177777 & ~PDR_W));
	} else {
		// PDR (Processor Descriptor Register)
		if (acc == ACC_BYTE) {
			// If byte access, merge data with APR entry.
			apr = APR(idx);
			data = (pAddr & 1) ? ((data << 8)   | (apr & 0377)) :
		   	                  ((data & 0377) | (apr & ~0377));
		}
		APR(idx) = (data & PDR_RW) | (APR(idx) & ~(PDR_RW|PDR_W));
	}

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("%s: APR %03o (%o) <= %06o (%06o %06o)\n", 
			p11->Unit.devName, idx, pAddr, data,
			APR(idx) >> 16, APR(idx) & 0177777);
#endif /* DEBUG */

	return UQ_OK;
}

// CPU Registers - 777740 to 777777
//
// MEMERR  777744  Memory Error Register  Read Only/Write to Reset
// CCR     777746  Cache Control Register Read/Write
// MAINT   777750  Maintenance Register   Read Only
// HITMISS 777752  His/Miss Register      Read Only
// CPUERR  777766  CPU Error Register     Read Only/Write to Reset
// PIRQ    777772  Priority Interrupts    Read/Write with Side Effects
// PSW     777776  Process Status Word    Read/Write with Side Effects

int f11_ReadCPU(void *dptr, uint32 pAddr, uint16 *data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;

	switch ((pAddr >> 1) & 017) {
		case 002: // Memory Error (MEMERR)
			*data = MEMERR;
			break;
		case 003: // Cache Control Register (CCR)
			*data = CCR;
			break;
		case 004: // Maintenance Register (MAINT)
			*data = MAINT;
			break;
		case 005: // Hit/Miss Register (HMR)
			*data = HMR;
			break;
		case 013: // CPU Error Register (CPUERR)
			*data = CPUERR;
			break;
		case 015: // Priority Interrupt Requests (PIRQ)
			*data = PIRQ;
			break;
		case 017: // Processor Status Word (PSW)
			*data = PSW|CC;
			break;
		default:  // Otherwise - Non-existant Memory
			*data = 0;
			return UQ_NXM;
	}

	if (acc & ACC_INST)
		return UQ_ADRERR;
	return UQ_OK;
}

int f11_WriteCPU(void *dptr, uint32 pAddr, uint16 data, uint32 acc)
{
	P11_CPU *p11 = (P11_CPU *)dptr;

	switch ((pAddr >> 1) & 017) {
		case 002: // Memory Error Register
			MEMERR = 0;
			return UQ_OK;

		case 003: // Cache Control Register
			if (acc == ACC_BYTE) // If byte access, merge data with CCR register.
				data = (pAddr & 1) ? ((data << 8)   | (CCR & 0377)) :
		                           ((data & 0377) | (CCR & ~0377));
			CCR = data;
			return UQ_OK;

		case 004: // Maintenance Register
			// Read-only Register - Do nothing
			return UQ_OK;

		case 005: // Hit/Miss Register
			// Read-only Register - Do nothing
			return UQ_OK;

		case 013: // CPU Error Register
			CPUERR = 0;
			return UQ_OK;

		case 015: // Priority Interrupt Requests
			{
				uint16 pl = 0;

				if (acc == ACC_BYTE) {
					if (pAddr & 1) data <<= 8;
					else           return UQ_OK;
				}
				PIRQ = data & PIRQ_RW;

				if (PIRQ & PIRQ_PIR1) pl = 0042;
				if (PIRQ & PIRQ_PIR2) pl = 0104;
				if (PIRQ & PIRQ_PIR3) pl = 0146;
				if (PIRQ & PIRQ_PIR4) pl = 0210;
				if (PIRQ & PIRQ_PIR5) pl = 0252;
				if (PIRQ & PIRQ_PIR6) pl = 0314;
				if (PIRQ & PIRQ_PIR7) pl = 0356;
				PIRQ |= pl;

				// Update interrupt requests.
				uq11_EvalIRQ(p11, GET_IPL(PSW));
			}
			return UQ_OK;

		case 017: // Processor Status Word
			{
				uint16 newPSW,  oldPSW = PSW;
				uint16 newMode, oldMode;
				int    idx;

				if (acc == ACC_BYTE) // If byte access, merge data with APR entry.
					data = (pAddr & 1) ? ((data << 8)   | (PSW & 0377)) :
		         	                  ((data & 0377) | (PSW & ~0377));
				newPSW = (data & PSW_RW) | (PSW & ~PSW_RW);

				// Get modes from old and new PSW.
				oldMode = PSW_GETCUR(oldPSW);
				newMode = PSW_GETCUR(newPSW);

				// Switch SP between kernel, supervisor, or user.
				// Swap two register sets if new RS mode request.
				STKREG(oldMode) = SP;
				if ((oldPSW ^ newPSW) & PSW_RS) {
					uint32 oldSet = (oldPSW & PSW_RS) ? 1 : 0;
					uint32 newSet = (newPSW & PSW_RS) ? 1 : 0;

					// Exchange all registers R0-R5 with R0'-R5'
					for (idx = 0; idx < 6; idx++) {
						GPREG(idx, oldSet) = REGW(idx);
						REGW(idx) = GPREG(idx, newSet);
					}
				}
				SP  = STKREG(newMode);
				PSW = newPSW & ~CC_ALL;
				CC  = newPSW & CC_ALL;

				// Set instruction/data space for memory management.
				ISPACE = GetISpace(newMode);
				DSPACE = GetDSpace(newMode);

				// Update interrupt requests.
				uq11_EvalIRQ(p11, GET_IPL(PSW));
			}
			return UQ_OK;
	}

	return UQ_NXM;
}

// Set up internal I/O registers for MMRx, APR, and CPU registers
int f11_InitRegs(register P11_CPU *p11)
{
	UQ_IO  *uq = (UQ_IO *)p11->uqba;
	MAP_IO *io;

	// Set up CPU registers in I/O map area.
	io             = &uq->ioCPU;
	io->devName    = p11->Unit.devName;
	io->keyName    = p11->Unit.keyName;
	io->emuName    = "CPU Registers";
	io->emuVersion = NULL;
	io->Device     = p11;
	io->csrAddr    = CPU_CSRADR;
	io->nRegs      = CPU_NREGS;
	io->ReadIO     = f11_ReadCPU;
	io->WriteIO    = f11_WriteCPU;
	uq->Callback->SetMap(uq, io);

	// Set up SR/MMR0-2 registers in I/O map area.
	io             = &uq->ioSRMM;
	io->devName    = p11->Unit.devName;
	io->keyName    = p11->Unit.keyName;
	io->emuName    = "SR/MMR0-2 Registers";
	io->emuVersion = NULL;
	io->Device     = p11;
	io->csrAddr    = SRMM_CSRADR;
	io->nRegs      = SRMM_NREGS;
	io->ReadIO     = f11_ReadSRMM;
	io->WriteIO    = f11_WriteSRMM;
	uq->Callback->SetMap(uq, io);

	// Set up MMR3 register in I/O map area.
	io             = &uq->ioMMR3;
	io->devName    = p11->Unit.devName;
	io->keyName    = p11->Unit.keyName;
	io->emuName    = "MMR3 Register";
	io->emuVersion = NULL;
	io->Device     = p11;
	io->csrAddr    = MMR3_CSRADR;
	io->nRegs      = MMR3_NREGS;
	io->ReadIO     = f11_ReadMMR3;
	io->WriteIO    = f11_WriteMMR3;
	uq->Callback->SetMap(uq, io);

	// Set up APR (Kernel/Supervisor) registers in I/O map area.
	io             = &uq->ioAPR1;
	io->devName    = p11->Unit.devName;
	io->keyName    = p11->Unit.keyName;
	io->emuName    = "APR Registers (Kern, Super)";
	io->emuVersion = NULL;
	io->Device     = p11;
	io->csrAddr    = APR1_CSRADR;
	io->nRegs      = APR1_NREGS;
	io->ReadIO     = f11_ReadAPR;
	io->WriteIO    = f11_WriteAPR;
	uq->Callback->SetMap(uq, io);

	// Set up APR (User) registers in I/O map area.
	io             = &uq->ioAPR2;
	io->devName    = p11->Unit.devName;
	io->keyName    = p11->Unit.keyName;
	io->emuName    = "APR Registers (User)";
	io->emuVersion = NULL;
	io->Device     = p11;
	io->csrAddr    = APR2_CSRADR;
	io->nRegs      = APR2_NREGS;
	io->ReadIO     = f11_ReadAPR;
	io->WriteIO    = f11_WriteAPR;
	uq->Callback->SetMap(uq, io);

	f11_InitTimer(p11);
}

// *************************************************************
// ************** F11 Series System Configuration **************
// *************************************************************

static uint16 intVectors[] = {
	VEC_RED,   // Red Stack
	VEC_ODD,   // Odd Address
	VEC_MME,   // Memory Management Error
	VEC_NXM,   // Non-existant Memory
	VEC_PAR,   // Parity Error
	VEC_PRV,   // Privileged Instruction
	VEC_PRV,   // Illegal Instruction
	VEC_BPT,   // Breakpoint Instruction
	VEC_IOT,   // Input/Output Trap Instruction
	VEC_EMT,   // Emulator Trap Instruction
	VEC_TRAP,  // Trap Instruction
	VEC_TRC,   // Trace (T-bit)
	VEC_YEL,   // Yellow Stack
	VEC_PWRFL, // Power Failure
	VEC_FPE    // Floating Point Error
};

void f11_ResetCPU(F11_CPU *f11)
{
	register P11_CPU *p11 = (P11_CPU *)f11;
	int idx;

	PIRQ = 0;
	MMR0 = 0;
	MMR1 = 0;
	MMR2 = 0;
	MMR3 = 0;

	for (idx = 0; idx < TRAP_MAX; idx++)
		p11->intVec[idx] = intVectors[idx];

	// Clear all memory management settings
	for (idx = 0; idx < APR_NREGS; idx++)
		APR(idx) = 0;
}

extern P11_INST pdp11_Inst[];

void f11_BuildCPU(F11_CPU *f11)
{
	register P11_CPU *cpu = (P11_CPU *)f11;
	uint32 idxInst, idxOpnd;

	// Clear all instruction tables
	for (idxInst = 0; idxInst < NUM_INST; idxInst++)
		cpu->tblOpcode[idxInst] = INSNAM(p11, ILL);

	// Build instruction table
	for (idxInst = 0; pdp11_Inst[idxInst].Name; idxInst++) {
		uint16 opCode = pdp11_Inst[idxInst].opCode;
		uint16 opReg  = pdp11_Inst[idxInst].opReg;

		if (pdp11_Inst[idxInst].Execute == NULL)
			continue;
		for (idxOpnd = opCode; idxOpnd < (opCode + opReg + 1); idxOpnd++)
			cpu->tblOpcode[idxOpnd] = pdp11_Inst[idxInst].Execute;
	}
}

void *f11_Create(MAP_DEVICE *newMap, int argc, char **argv)
{
	F11_CPU    *f11 = NULL;
	P11_SYSTEM *p11sys;
	CLK_QUEUE  *Timer;

	if (f11 = (F11_CPU *)calloc(1, sizeof(F11_CPU))) {
		f11->cpu.Unit.devName    = newMap->devName;
		f11->cpu.Unit.keyName    = newMap->keyName;
		f11->cpu.Unit.emuName    = newMap->emuName;
		f11->cpu.Unit.emuVersion = newMap->emuVersion;

		// First, link it to VAX system device.
		p11sys            = (P11_SYSTEM *)newMap->sysDevice;
		f11->cpu.System   = p11sys;
//		f11->cpu.Console  = p11_ConsoleInit((P11_CPU *)f11);
		p11sys->cpu       = (P11_CPU *)f11;

		f11->cpu.Flags   = 0;
		f11->cpu.cpuType = PID_F11;

		// Build instruction table
		f11_BuildCPU(f11);
		f11_ResetCPU(f11);

		// Set main memory (will be moved to 'set memory' command later
		p11_InitMemory((P11_CPU *)f11, 1024 * 1024);

		// Set function calls for KDF11 Series Processor.
		f11->cpu.InitRegs   = f11_InitRegs; // Unibus Registers
		f11->cpu.InitTimer  = f11_InitTimer;
		f11->cpu.StartTimer = f11_StartTimer;
		f11->cpu.StopTimer  = f11_StopTimer;

		// Finally, link them to its mapping device.
		newMap->Device        = f11;
#ifdef DEBUG
//		newMap->Breaks        = &f11->cpu.Breaks;
#endif /* DEBUG */
	}

	return f11;
}

int f11_Reset(void *dptr)
{
	f11_ResetCPU(dptr);
}

int f11_Boot(MAP_DEVICE *map, int argc, char **argv)
{
	F11_CPU          *f11 = (F11_CPU *)map->Device;
	register P11_CPU *p11 = (P11_CPU *)f11;

	// Tell operator that processor is being booted.
	printf("Booting %s: (%s)... ", p11->Unit.devName, p11->Unit.keyName);

	// Initialize F11 Processor
	p11->State = P11_HALT;
	f11_ResetCPU(f11);

	// Initial Power On
	PC = 0173000;

	// Enable F11 Processor to run.
	// Finally, transfer control to
	// PDP-11 processor.
	p11->State = P11_RUN;
	emu_State  = P11_RUN;

	// Tell operator that it was done.
	printf("done.\n");

	return TS10_OK;
}

extern COMMAND p11_Commands[];
extern COMMAND p11_SetCommands[];
extern COMMAND p11_ShowCommands[];
extern DEVICE  uq11_Device;

DEVICE *f11_Devices[] =
{
	&uq11_Device,  // Unibus/QBus I/O Devices
	NULL           // Null terminator
};

// CVAX Series System Definition
DEVICE f11_System =
{
	// Description Information
	F11_KEY,             // Device Type Name
	F11_NAME,            // Emulator Name
	F11_VERSION,         // Emulator Version

	f11_Devices,         // Unibus/Qbus Device Listings
	DF_USE|DF_SYSMAP,    // Device Flags
	DT_PROCESSOR,        // Device Type

	p11_Commands,        // Commands
	p11_SetCommands,     // Set Commands
	p11_ShowCommands,    // Show Commands

	f11_Create,          // Create Routine
	NULL,                // Configure Routine
	NULL,                // Delete Routine
	f11_Reset,           // Reset Routine
	NULL,                // Attach Routine
	NULL,                // Detach Routine
	NULL,                // Info Routine
	f11_Boot,            // Boot Routine
	NULL,                // Execute Routine
#ifdef DEBUG
	NULL,                // Debug Routine
#endif /* DEBUG */
};
