#include <stdio.h>
#include "strlst.h"

void affiche_unichar(__glcUniChar *s)
{
	char buffer[256];

	if (!s) {
		printf("<Null>");
		return;
	}
	s->dup(buffer, 256);
	printf("%s", buffer);
}

void affiche_liste(__glcStringList *l)
{
	int j= 0;
	__glcStringList *lp = l;

	if (!l || !l->count) {
		printf("<Null>");
		return;
	}
	printf("Count : %d\n", l->count);
	for (j=0;j<l->count;j++) {
		affiche_unichar(lp->string);
		printf(" ");
		lp = lp->next;
	}
}

void affiche_liste_bytes(__glcStringList *l)
{
	int j= 0, i = 0;
	__glcStringList *lp = l;
	char buffer[256];

	if (!l || !l->count) {
		printf("<Null>");
		return;
	}
	printf("Count : %d\n", l->count);
	for (j=0;j<l->count;j++) {
		lp->string->dup(buffer, 256);
		for(i=0; i<lp->string->lenBytes(); i++)
			printf("<%X>", buffer[i]);
		printf(" ");
		lp = lp->next;
	}
}

int main(void)
{
	char *string[] = { "string 1", "string 2", "string 3", "string 4", "foobar" };
	__glcStringList l = __glcStringList(NULL);
	__glcUniChar s, *sp = NULL;
	int i = 0;
	
	printf("Append :\n");
	printf("Count : %d\n", l.count);
	for (i=0; i<3; i++) {
		s = __glcUniChar(string[i], GLC_UCS1);
		l.append(&s);
		affiche_liste(&l);
		printf("\n");
	}
	
	printf("Prepend :\n");
	s = __glcUniChar(string[3], GLC_UCS1);
	l.prepend(&s);
	affiche_liste(&l);
	printf("\n");
	
	printf("FindIndex :\n");
	for (i=0;i<5;i++) {
		sp = l.findIndex(i);
		printf("%d : ", i);
		affiche_unichar(sp);
		printf("\n");
	}
	i = 10;
	sp = l.findIndex(i);
	printf("%d : ", i);
	affiche_unichar(sp);
	printf("\n");

	printf("Find :\n");
	for (i=0;i<5;i++) {
		s = __glcUniChar(string[i], GLC_UCS1);
		sp = l.find(&s);
		printf("%s : ", string[i]);
		affiche_unichar(sp);
		printf("\n");
	}
	
	printf("Remove :\n");
	s = __glcUniChar(string[0], GLC_UCS1);
	printf("%s : ", string[0]);
	l.remove(&s);
	affiche_liste(&l);
	printf("\n");
	s = __glcUniChar(string[2], GLC_UCS1);
	printf("%s : ", string[2]);
	l.remove(&s);
	affiche_liste(&l);
	printf("\n");
	s = __glcUniChar(string[3], GLC_UCS1);
	printf("%s : ", string[3]);
	l.remove(&s);
	affiche_liste(&l);
	printf("\n");
	s = __glcUniChar(string[4], GLC_UCS1);
	printf("%s : ", string[4]);
	l.remove(&s);
	affiche_liste(&l);
	printf("\n");
	
	for (i=0;i<2;i++) {
		s = __glcUniChar(string[i], GLC_UCS1);
		l.append(&s);
	}
	s = __glcUniChar(string[i], GLC_UCS1);
	l.prepend(&s);
	affiche_liste(&l);
	printf("\n");
	
	printf("Remove Index :\n");
	printf("%d : ", 1);
	l.removeIndex(1);
	affiche_liste(&l);
	printf("\n");
	printf("%d : ", 0);
	l.removeIndex(0);
	affiche_liste(&l);
	printf("\n");
	printf("%d : ", 1);
	l.removeIndex(1);
	affiche_liste(&l);
	printf("\n");
	printf("%d : ", 10);
	l.removeIndex(10);
	affiche_liste(&l);
	printf("\n");
	printf("%d : ", 0);
	l.removeIndex(0);
	affiche_liste(&l);
	printf("\n");
	printf("%d : ", 10);
	l.removeIndex(10);
	affiche_liste(&l);
	printf("\n");

	for (i=0;i<2;i++) {
		s = __glcUniChar(string[i], GLC_UCS1);
		l.append(&s);
	}
	s = __glcUniChar(string[i], GLC_UCS1);
	l.prepend(&s);
	affiche_liste(&l);
	printf("\n");
	
	for (i=0; i<5; i++) {
		s = __glcUniChar(string[i], GLC_UCS1);
		printf("%s : %d\n", string[i], l.getIndex(&s));
	}
	
	affiche_liste_bytes(&l);
	printf("\n");
	
	printf("Convert :\n");
	l.convert(GLC_UCS2);
	affiche_liste_bytes(&l);
	printf("\n");
}
