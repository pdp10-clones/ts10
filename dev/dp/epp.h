// epp.h - Ethernet Protocol Program for DEQNA series
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


// EPP Commands
enum {
	EPP_INIT = 1, // Initialization (First time)
	EPP_SETUP,    // Setup Frame
	EPP_TRANSMIT, // Transmit Ethernet Frame
	EPP_ILOOP,    // Internal Loopback
	EPP_ELOOP,    // External Loopback
	EPP_EILOOP,   // Extended Internal Loopback
	EPP_XSTATUS,  // Transmit Status Results
	EPP_RECEIVE,  // Receive Ethernet Frame
	EPP_RSTATUS,  // Receive Status Results
};

// OpenTUN function - result code
#define EPP_TUN_IOERROR -1
#define EPP_TUN_INVALID -2

