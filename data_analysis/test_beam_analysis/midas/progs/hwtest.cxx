/********************************************************************\

  Name:         hwtest.c
  Created by:   Stefan Ritt

  Contents:     Program which tests the architecture of the HW/OS

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <string.h>

struct {
   char c;
   double d;
} test_align;

#ifdef vxw
int hwtest()
#else
int main()
#endif
{
   unsigned long dummy;
   unsigned char *p;
   float f;
   double d;

   printf("Data sizes:\n");
   printf("sizeof(int)    = %d\n", (int)sizeof(int));
   printf("sizeof(float)  = %d\n", (int)sizeof(float));
   printf("sizeof(double) = %d\n", (int)sizeof(double));
   printf("sizeof(long int) = %d\n", (int)sizeof(long int));
   printf("sizeof(long double) = %d\n", (int)sizeof(long double));
   printf("sizeof(char *) = %d\n", (int)sizeof(p));
   if (sizeof(p) == 2)
      printf("...this looks like a 16-bit OS\n\n");
   else if (sizeof(p) == 4)
      printf("...this looks like a 32-bit OS\n\n");
   else if (sizeof(p) == 8)
      printf("...this looks like a 64-bit OS\n\n");

   printf("Byte order:\n");
   dummy = 0x12345678;
   p = (unsigned char *) &dummy;
   printf("0x12345678     = %02X %02X %02X %02X\n", p[0], p[1], p[2], p[3]);
   if (*((unsigned char *) &dummy) == 0x78)
      printf("...this looks like a LITTLE ENDIAN machine\n\n");
   else
      printf("...this looks like a BIG ENDIAN machine\n\n");

   printf("Floating point coding:\n");
   f = (float) 1.2345;
   p = (unsigned char *) &f;
   printf("1.2345f        = ");
   for (unsigned i = 0; i < sizeof(float); i++)
      printf("%02X ", p[i]);
   printf("\n");
   d = 1.2345;
   p = (unsigned char *) &d;
   printf("1.2345d        = ");
   for (unsigned i = 0; i < sizeof(double); i++)
      printf("%02X ", p[i]);
   printf("\n");

   dummy = 0;
   memcpy(&dummy, &f, sizeof(f));
   if ((dummy & 0xFF) == 0x19 &&
       ((dummy >> 8) & 0xFF) == 0x04 &&
       ((dummy >> 16) & 0xFF) == 0x9E && ((dummy >> 24) & 0xFF) == 0x3F)
      printf("...this looks like IEEE float format\n");
   else if ((dummy & 0xFF) == 0x9E &&
            ((dummy >> 8) & 0xFF) == 0x40 &&
            ((dummy >> 16) & 0xFF) == 0x19 && ((dummy >> 24) & 0xFF) == 0x04)
      printf("...this looks like VAX float format\n");

   dummy = 0;
   memcpy(&dummy, &d, sizeof(f));
   if ((dummy & 0xFF) == 0x8D &&
       ((dummy >> 8) & 0xFF) == 0x97 &&
       ((dummy >> 16) & 0xFF) == 0x6E && ((dummy >> 24) & 0xFF) == 0x12)
      printf("...this looks like IEEE double float format (little endian)\n\n");
   else if ((dummy & 0xFF) == 0x83 &&
            ((dummy >> 8) & 0xFF) == 0xC0 &&
            ((dummy >> 16) & 0xFF) == 0xF3 && ((dummy >> 24) & 0xFF) == 0x3F)
      printf("...this looks like IEEE double float format (big endian)\n\n");
   else if ((dummy & 0xFF) == 0x13 &&
            ((dummy >> 8) & 0xFF) == 0x40 &&
            ((dummy >> 16) & 0xFF) == 0x83 && ((dummy >> 24) & 0xFF) == 0xC0)
      printf("...this looks like VAX G double format\n\n");
   else if ((dummy & 0xFF) == 0x9E &&
            ((dummy >> 8) & 0xFF) == 0x40 &&
            ((dummy >> 16) & 0xFF) == 0x18 && ((dummy >> 24) & 0xFF) == 0x04)
      printf("...this looks like VAX D double format\n\n");


   int i = (char*) (&test_align.d) - (char*) &test_align.c;
   printf("Structure members are %d-byte aligned\n\n", i);

   return 1;
}
