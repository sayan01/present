#include<hpdf.h>
#include<string.h>
#include<stdio.h>
#include<setjmp.h>
#include"main.h"
jmp_buf env;
HPDF_Doc pdf;
HPDF_Font font;
HPDF_Page page;
int width = 1920, height = 1080; // 1080p monitor
int linebuffer_length = 100000; 
int title_font_size = 96;
int content_font_size = 24;
float content_margin_multiplier = 0.2;
int page_type = 0;

#define TITLE 1
#define CONTENT 2
#define IMAGE 4

/* error handler called by hpdf in case of errors */
void error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
  printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no, (HPDF_UINT)detail_no);
  longjmp(env, 1);
}

/* print provided text at provided X-Y coordinates, with the given font size
 * coordinate plane starts from bottom left.
 * 
 * Y
 * ^
 * |        (4,8)
 * |
 * | (2,2)
 * |
 * L------------> X
 */
void outtextxy(int x, int y, const char* text, int fontsize){
  HPDF_Page_BeginText(page);
  HPDF_Page_SetFontAndSize(page, font, fontsize);
  HPDF_Page_TextOut(page, x, y, text);
  HPDF_Page_EndText(page);
}

/* print text in given Y coordinate, but centered horizontally */
void centertext(int y, const char* text, int fontsize){
  HPDF_Page_SetFontAndSize(page, font, fontsize);
  outtextxy((width - HPDF_Page_TextWidth(page, text)) / 2,y,text,fontsize);
}

/* print text centered both horizontally and vertically */
void centertextinpage(const char* text, int fontsize){
  HPDF_Page_SetFontAndSize(page, font, fontsize);
  centertext(height / 2, text, fontsize);
}

/* insert a new page and set its dimension */
void newpage(){
  page = HPDF_AddPage(pdf);
  HPDF_Page_SetWidth(page, width); HPDF_Page_SetHeight(page, height);
}

/* prints the page number of current page at the bottom of the page */
void print_page_number(int page_number){
  char pagenumber[5];
  sprintf(pagenumber, "%d", page_number);
  outtextxy(width - 50, 20, pagenumber, 24);
}
/* print passed text as title of the page */
void print_title(char* text){
  HPDF_Page_SetFontAndSize(page, font, title_font_size);
  centertext(height * 0.85, text + 1, title_font_size);
}

/* print content text in page */
void print_content(char* content){
  HPDF_Page_SetFontAndSize(page, font, title_font_size);
  centertext(height * 0.7, content, content_font_size);
}
int main(int argc, char** argv){
  /* check if run with src file as a parameter or not */
  if(argc < 2){
    printf("No source file provided. Quitting.\n");
    return 1;
  }

  /* open the src file and check if it exists */
  FILE *src = fopen(argv[1], "r");
  if(!src){
    printf("ERROR: cannot open file provided: %s\n", argv[1]);
    return 1;
  }

  /* create pdf object for writing */
  pdf = HPDF_New(error_handler,NULL);
  if(!pdf){
    printf("ERROR: cannot create pdf object\n");
    return 1;
  }

  /* set default font to be Helvetica */
  font = HPDF_GetFont(pdf, "Helvetica", NULL);

  
  /* iterate over lines of src and generate pages of pdf */
  char linebuffer[linebuffer_length];
  int page_number = 1, line_number = 0;
  newpage();
  print_page_number(1);
  float content_y_multiplier = 0.7;
  while(fgets(linebuffer, linebuffer_length, src)){
    line_number++;

    HPDF_Page_SetFontAndSize(page, font, content_font_size);
    /* if a newline is present, move to new page in pdf */
    if(linebuffer[0] == '\n'){
      content_y_multiplier = 0.7;
      newpage();
      print_page_number(++page_number);
      page_type = 0;
      continue;
    }

    else if (linebuffer[0] == '#'){
      print_title(linebuffer);
      page_type |= TITLE;
    }

    else {
      /* if the line is a content line then clip the line and print it */
      page_type |= CONTENT;
      int printable_index = HPDF_Page_MeasureText( page, linebuffer,
          width*(1 - content_margin_multiplier * 2), HPDF_TRUE, NULL);
      if(printable_index == 0){
        printable_index = HPDF_Page_MeasureText( page, linebuffer, 
            width*(1-content_margin_multiplier * 2), HPDF_FALSE, NULL);
      }
      char printbuffer[printable_index + 2];
      memset(printbuffer, 0, printable_index+2);
      memcpy(printbuffer, linebuffer, printable_index);
      outtextxy(width * content_margin_multiplier, height * content_y_multiplier, printbuffer, content_font_size);
      if(linebuffer[printable_index])
        printf("Line %d is too long for slide. Trimming it at closest word\n", line_number);
      content_y_multiplier -= 0.05;
    }
  }
  
  /* save the file and exit safely */
  HPDF_SaveToFile(pdf, "test.pdf");
  HPDF_Free(pdf);

  /* close src file */
  fclose(src);
  return 0;
}
