#include <GL/glc.h>
#include <stdio.h>
#include <string.h>
#include "unichar.h"

int main(void) 
{
	char buffer[256], buffer2[256];
	__glcUniChar s = __glcUniChar("Toto\0\0\0\0", GLC_UCS1);
	__glcUniChar s2, s3, s4;
	char *cptr;
	short *ptr;
	int i;
	
	printf("Longueur en caracteres: %d\n", s.len());
	printf("Longueur en octets: %d\n", s.lenBytes());
	printf("\nConversion GLC_UCS1 -> GLC_UCS1 :\n");
	printf("Conversion en GLC_UCS1 : %s\n", s.convert(buffer, GLC_UCS1, 256));
	printf("Longueur de la conversion : %d\n", strlen(buffer));
	for (i=0; i<s.lenBytes();i++)
		printf("<%d>", buffer[i]);
	printf("\n");
	
	s2 = __glcUniChar(s.convert(buffer, GLC_UCS4, 256), GLC_UCS4);
	printf("\nConversion GLC_UCS1 -> GLC_UCS4 :\n");
	printf("Longueur en caracteres: %d\n", s2.len());
	printf("Longueur en octets: %d\n", s2.lenBytes());
	for (i=0; i<s2.lenBytes();i++)
		printf("<%d>", buffer[i]);
	printf("\n");

	s3 = __glcUniChar(s2.convert(buffer2, GLC_UCS2, 256), GLC_UCS2);
	printf("\nConversion GLC_UCS4 -> GLC_UCS2 :\n");
	printf("Longueur en caracteres: %d\n", s3.len());
	printf("Longueur en octets: %d\n", s3.lenBytes());
	for (i=0; i<s3.lenBytes();i++)
		printf("<%d>", buffer2[i]);
	printf("\n");

	s4 = __glcUniChar(s2.convert(buffer2, GLC_UCS1, 256), GLC_UCS1);
	printf("\nConversion GLC_UCS4 -> GLC_UCS1 :\n");
	printf("Longueur en caracteres: %d\n", s4.len());
	printf("Longueur en octets: %d\n", s4.lenBytes());
	printf("Estimation longueur apres conversion : %d\n",
		s2.estimate(GLC_UCS1));
	printf("Conversion en GLC_UCS1 : %s\n", buffer2);
	printf("\n");
	
	s2 = __glcUniChar("TTo\0tto\0\0\0", GLC_UCS2);
	s2.dup(buffer, 256);
	printf("\nConversion GLC_UCS2 -> GLC_UCS1 :\n");
	for (i=0; i<s2.lenBytes();i++)
		printf("<%d>", buffer[i]);
	printf("\n");
	ptr = (short *)buffer;
	for (i=0; i<s2.len();i++)
		printf("<%d>", ptr[i]);
	printf("\n");

	s4 = __glcUniChar(s2.convert(buffer, GLC_UCS1, 256), GLC_UCS1);
	printf("\nConversion GLC_UCS2 -> GLC_UCS1 :\n");
	printf("Longueur en caracteres: %d\n", s4.len());
	printf("Longueur en octets: %d\n", s4.lenBytes());
	printf("Estimation longueur apres conversion : %d\n",
		s2.estimate(GLC_UCS1));
	printf("Conversion en GLC_UCS1 : %s\n", buffer);
	printf("\n");
	
	s = __glcUniChar("Tato", GLC_UCS1);
	s3 = __glcUniChar("Tota", GLC_UCS1);
	s2 = __glcUniChar(s3.convert(buffer, GLC_UCS2, 256), GLC_UCS2);
	printf("Comparaison : %d\n", s.compare(&s2));
}
