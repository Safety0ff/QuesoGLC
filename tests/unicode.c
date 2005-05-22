/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* This is some Unicode UTF-8 text rendered with QuesoGLC
 * Unicode text comes from Markus Kuhn's example file 'quickbrown.txt'
 *
 * The copyright for the Thai example is owned by The Computer Association of
 * Thailand under the Royal Patronage of His Majesty the King.
 */
/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>
#if defined __APPLE__ && defined __MACH__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <stdlib.h>

int language = 0;
int lines [] = {4, 6, 3, 4, 9, 3, 2, 4, 5, 3, 2, 2, 3, 5, 2};
char* danish[] = {"Danish:",
		  "Quizdeltagerne spiste jordbær med fløde,",
		  "mens cirkusklovnen",
		  "Wolther spillede på xylofon."};
char* german[] = {"German:",
		  "Falsches Üben von Xylophonmusik quält",
		  "jeden größeren Zwerg",
		  "Zwölf Boxkämpfer jagten Eva quer über",
		  "den Sylter Deich",
		  "Heizölrückstoßabdämpfung"};
char* english[] = {"English:",
		   "The quick brown fox jumps over the",
		   "lazy dog"};
char* spanish[] = {"Spanish:",
		   "El pingüino Wenceslao hizo kilómetros",
		   "bajo exhaustiva lluvia y frío, añoraba a su",
		   "querido cachorro."};
char* french[] = {"French:",
		  "Portez ce vieux whisky au juge blond qui",
		  "fume sur son île intérieure, à côté de",
		  "l'alcôve ovoïde, où les bûches se",
		  "consument dans l'âtre, ce qui lui permet de",
		  "penser à la cænogenèse de l'être dont il est",
		  "question dans la cause ambiguë entendue",
		  "à Moÿ, dans un capharnaüm qui, pense-t-il,",
		  "diminue çà et là la qualité de son œuvre."};
char* irish_gaelic[] = {"Irish Gaelic:",
			"D'fhuascail Íosa, Úrmhac na hÓighe",
			"Beannaithe, pór Éava agus Ádhaimh"};
char* hungarian[] = {"Hungarian:",
		     "Árvíztűrő tükörfúrógép"};
char* icelandic[] = {"Icelandic",
		     "Kæmi ný öxi hér ykist þjófum nú bæði víl",
		     "og ádrepa",
		     "Sævör grét áðan því úlpan var ónýt"};
char* japanese_hiragana[] = {"Japanese Hiragana: (Iroha)",
			     "いろはにほへとちりぬるを",
			     "わかよたれそつねならむ",
			     "うゐのおくやまけふこえて",
			     "あさきゆめみしゑひもせす"};
char* japanese_katakana[] = {"Japanese Katakana:",
			     "イロハニホヘト チリヌルヲ ワカヨタレソ ツネナラム",
			     "ウヰノオクヤマ ケフコエテ アサキユメミシ ヱヒモセスン"};
char* hebrew[] = {"Hebrew:",
		  "? דג סקרן שט בים מאוכזב ולפתע מצא לו חברה איך הקליטה"};
char* polish[] = {"Polish:",
		  "Pchnąć w tę łódź jeża lub ośm skrzyń fig"};
char* russian[] = {"Russian:",
		   "В чащах юга жил бы цитрус?",
		   "Да, но фальшивый экземпляр!"};
char* thai[] = {"Thai:",
		"๏ เป็นมนุษย์สุดประเสริฐเลิศคุณค่า  กว่าบรรดาฝูงสัตว์เดรัจฉาน",
		"จงฝ่าฟันพัฒนาวิชาการ           อย่าล้างผลาญฤๅเข่นฆ่าบีฑาใคร",
		"ไม่ถือโทษโกรธแช่งซัดฮึดฮัดด่า     หัดอภัยเหมือนกีฬาอัชฌาสัย",
		"ปฏิบัติประพฤติกฎกำหนดใจ        พูดจาให้จ๊ะๆ จ๋าๆ น่าฟังเอย ฯ"};
char* broken[] = {"This broken Unicode character should not be rendered: �",
		  "Press <ESC> to quit."};

void display(void)
{
  char** text = NULL;
  int i = 0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(0., 1., 0.);

  switch(language) {
  case 0:
    text = danish;
    break;
  case 1:
    text = german;
    break;
  case 2:
    text = english;
    break;
  case 3:
    text = spanish;
    break;
  case 4:
    text = french;
    break;
  case 5:
    text = irish_gaelic;
    break;
  case 6:
    text = hungarian;
    break;
  case 7:
    text = icelandic;
    break;
  case 8:
    text = japanese_hiragana;
    break;
  case 9:
    text = japanese_katakana;
    break;
  case 10:
    text = hebrew;
    break;
  case 11:
    text = polish;
    break;
  case 12:
    text = russian;
    break;
  case 13:
    text = thai;
    break;
  default:
    text = broken;
    language = 14;
    break;
  }

  for (i = 0; i < lines[language]; i++) {
    if (!i)
      glColor3f(1., 0., 0.);
    else
      glColor3f(0., 1., 0.);
    glLoadIdentity();
    glTranslatef(10., 450. - i * 40., 0.);
    glScalef(25., 25., 0.);
    glcRenderString(text[i]);
  }

  glFlush();
}

void reshape(int width, int height)
{
  glClearColor(0., 0., 0., 0.);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.325, width - 0.325, -0.325, height - 0.325);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
  int i = 0;
  int count = 0;
  int font = 0;

  switch(key) {
  case 27: /* Escape Key */
    count = glcGeti(GLC_CURRENT_FONT_COUNT);
    printf("%d fonts were used :\n", count);
    for (i = 0; i < count; i++) {
      font = glcGetListi(GLC_CURRENT_FONT_LIST, i);
      /* The output of the family name and the face is printed using 2 steps
       * because glcGetFontc and glcGetFontFace return their result in the
       * same buffer (according to GLC specs).
       */
      printf("Font #%d : %s", font,
	     (const char*) glcGetFontc(font, GLC_FAMILY));
      printf(" %s\n", (const char*) glcGetFontFace(font));
    }
    exit(0);
  case 32:
    language++;
    display();
    break;
  default:
    break;
  }
}

int main(int argc, char **argv)
{
  int ctx = 0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(640, 480);
  glutCreateWindow("Unicode test");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glEnable(GL_TEXTURE_2D);

  ctx = glcGenContext();
  glcContext(ctx);

  glcDisable(GLC_GL_OBJECTS);
  glcRenderStyle(GLC_TEXTURE);
  glcStringType(GLC_UTF8_QX);

  glutMainLoop();
  return 0;
}
