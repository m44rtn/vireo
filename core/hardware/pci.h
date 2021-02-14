/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __PCI_H__
#define __PCI_H__

#define PCI_BAR0    0x04
#define PCI_BAR1    0x05
#define PCI_BAR2    0x06
#define PCI_BAR3    0x07
#define PCI_BAR4    0x08
#define PCI_BAR5    0x09

void pci_init(void);
unsigned char pciGetInterruptLine(unsigned char bus, unsigned char device, unsigned char func);
unsigned int *pciGetDevices(unsigned char class, unsigned char subclass);
unsigned int *pciGetAllDevices(void);

unsigned int pciGetInfo(unsigned int device);
unsigned int pciGetReg0(unsigned int device);
unsigned int pciGetDeviceByReg0(unsigned int Reg0);

unsigned int pciGetBar(unsigned int device, unsigned char bar);

#endif