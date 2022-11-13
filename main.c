#include<hpdf.h>
#include<string.h>
#include<stdio.h>
#include<setjmp.h>
#include<ctype.h>
#include"main.h"
jmp_buf env;

HPDF_Doc pdf;
HPDF_Font font;
HPDF_Page page;

int width = 1920, height = 1080; // 1080p monitor
int linebuffer_length = 100000; 
int banner_font_size = 128;
int title_font_size = 96;
int content_font_size = 32;
int page_type = 0;

#define TITLE 1
#define CONTENT 2
#define IMAGE 4
#define BANNER 8

float content_margin_multiplier = 0.2;
float content_y_multiplier = 0.7;
float content_y_min_multiplier = 0.2;
float title_margin_multiplier = 0.2;
float title_y_multiplier = 0.85;
float image_width_multiplier = 0.5;
float image_height_multiplier = 0.5;

#define CONTENT_MARGIN_MULTIPLIER_DEFAULT 0.2
#define CONTENT_Y_MULTIPLIER_DEFAULT 0.7


/* helper functions */

/* check if suffix is present at end of str (case insensitive) */
int strendswith(const char *str, const char *suffix) {
  /* if either is null, return false */
  if (!str || !suffix) return 0;
  size_t lenstr = strlen(str), lensuffix = strlen(suffix);
  /* if suffix is bigger than str, return false */
  if (lensuffix >  lenstr) return 0;
  /* temporary buffer to store the lowercase version of the strings */
  char stra[lenstr], suffixa[lensuffix];
  memset(stra,0, lenstr); memset(suffixa, 0, lensuffix);
  memcpy(stra, str, lenstr); memcpy(suffixa, suffix, lensuffix);
  /* convert copy of strings to lowercase */
  for (char* p = stra ; *p; ++p) *p = tolower(*p);
  for (char* p = suffixa ; *p; ++p) *p = tolower(*p);
  /* check if both buffers are same */
  return strncmp(stra + lenstr - lensuffix, suffixa, lensuffix) == 0;
}

float min(float a, float b){ return a < b ? a : b; }
float max(float a, float b){ return a > b ? a : b; }

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

/* print passed text as banner of the page */
void print_banner(const char* text){
  centertextinpage(text, banner_font_size);
}

/* print passed text as title of the page */
void print_title(const char* text){

  /* initialize constants */
  int title_margin = title_margin_multiplier * width;
  int title_y = title_y_multiplier * height;

  /* print the text to pdf */
  centertext(title_y, text, title_font_size);

  /* print horizontal line below title */
  HPDF_Page_SetLineWidth(page, 1);
  HPDF_Page_MoveTo(page, title_margin, title_y - 50);
  HPDF_Page_LineTo(page, width - title_margin, title_y - 50);
  HPDF_Page_Stroke(page);
}

/* print passed text as content of the page */
void print_content(const char* text){

  /* initialize constants */
  int content_margin = content_margin_multiplier * width;
  int content_y = content_y_multiplier * height;

  /* output text to pdf */
  outtextxy(content_margin, content_y, text, content_font_size);
}

/* print passed text as content of the page */
void print_link(const char* text){

  /* initialize constants */
  int content_margin = content_margin_multiplier * width;
  int content_y = content_y_multiplier * height;
  HPDF_Page_SetRGBFill (page, 0, 0, 1);
  /* output text to pdf */
  outtextxy(content_margin, content_y, text, content_font_size);
}

/* see if content fits the width, if not then clip to nearest word and call callback printer */
int cliptext(const char* text, float _width, int line_number, void (*callback)(const char*) ){
  int printable_index = HPDF_Page_MeasureText( page, text, _width, HPDF_TRUE, NULL);
  if(printable_index == 0){
    printable_index = HPDF_Page_MeasureText( page, text, _width, HPDF_FALSE, NULL);
  }
  char printbuffer[printable_index + 2];
  memset(printbuffer, 0, printable_index+2);
  memcpy(printbuffer, text, printable_index);
  if(printbuffer[printable_index-1] == '\n') printbuffer[printable_index-1] = ' ';
  if(printable_index)
    callback(printbuffer);
  if(text[printable_index])
    printf("Line %d is too long. Trimming it at closest word at column %d\n", line_number, printable_index);
  return text[printable_index];
}

int main(int argc, char** argv){
  /* initialize variables */
  int image_width = image_width_multiplier * width;
  int image_height = image_height_multiplier * height;

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
  const char* font_name = HPDF_LoadTTFontFromFile(pdf, "font.ttf", HPDF_TRUE);
  font = HPDF_GetFont(pdf, font_name, NULL);
  //font = HPDF_GetFont(pdf, "Helvetica", NULL);

  
  /* iterate over lines of src and generate pages of pdf */
  char linebuffer[linebuffer_length];
  int page_number = 1, line_number = 0;
  newpage();
  print_page_number(1);
  content_y_multiplier = 0.7;
  while(fgets(linebuffer, linebuffer_length, src)){
    line_number++;

    HPDF_Page_SetFontAndSize(page, font, content_font_size);
    /* if a newline is present, move to new page in pdf */
    if(linebuffer[0] == '\n'){
      content_y_multiplier = CONTENT_Y_MULTIPLIER_DEFAULT;
      content_margin_multiplier = CONTENT_MARGIN_MULTIPLIER_DEFAULT;
      newpage();
      print_page_number(++page_number);
      page_type = 0;
      continue;
    }

    else if (linebuffer[0] == '$'){
      page_type |= BANNER;
      cliptext(linebuffer + 1, width, line_number, print_banner);
    }

    else if (linebuffer[0] == '#'){
      /* if current slide is a banner slide, skip titles */
      if(page_type & BANNER) continue;
      page_type |= TITLE;
      cliptext(linebuffer + 1, width*(1 - title_margin_multiplier * 2), line_number, print_title);
    }

    else if(linebuffer[0] == '!'){
      /* if current slide is a banner slide, skip images */
      if(page_type & BANNER) continue;
      /* if image, render image on page */
      page_type |= IMAGE;
      /* get proper filename of image in buffer */
      int filename_length = strlen(linebuffer) - 2;
      char filename[filename_length+1];
      memset(filename, 0, filename_length+1);
      memcpy(filename, linebuffer+1, filename_length);
      /* try to open image handle depending on image type */
      FILE * img = fopen(filename, "r");
      if(!img){
        printf("Line %d: image file named %s does not exist or is not accessible. Skipping\n", line_number, filename);
        continue;
      }
      HPDF_Image image;
      if(strendswith(filename, ".png"))
        image = HPDF_LoadPngImageFromFile(pdf, filename);
      else if(strendswith(filename, ".jpg") || strendswith(filename, ".jpeg"))
        image = HPDF_LoadJpegImageFromFile(pdf, filename);
      else{
        printf("Line %d: Entered filename for image is not supported. Skipping\n", line_number);
        continue;
      }
      /* if unable to open image, inform and continue */
      if(!image){
        printf("Line %d: Error opening specified file '%s'", line_number, filename);
        continue;
      }
      /* draw image */
      HPDF_Page_DrawImage(page, image, 
          (width - image_width) / 2.0, 
          (height - image_height) / 2.0, 
          image_width, image_height);
    }

    else if(linebuffer[0] == '@'){
      /* if line is an url, create url and display */
      HPDF_Rect rect;
      int linelength = strlen(linebuffer) - 2;
      int firstspace = strcspn(linebuffer, " ");
      /*store url in buffer */
      char url[firstspace];
      memset(url, 0, firstspace);
      strncpy(url, linebuffer + 1, firstspace - 1);
      char url2[firstspace+7];
      char *urlp = url;
      if(memcmp(url, "http", 4)){
        memset(url2, 0, firstspace+7);
        strcat(url2, "http://");
        strcat(url2, url);
        urlp = url2;
      }
      /* point to text part of url in text pointer */
      char * text = linebuffer + firstspace + 1;
      /* output text to pdf */
      cliptext(text, width*(1-content_margin_multiplier*2), line_number, print_link);
      /* create hitbox for url annotation */
      rect.left = width*content_margin_multiplier;
      rect.right = width-rect.left;
      rect.bottom = content_y_multiplier * height;
      float textheight = HPDF_Font_GetCapHeight(font) * content_font_size / 1000.0;
      rect.top = rect.bottom + textheight;
      /* set the annotation to link to the url , no borders */
      HPDF_Annotation annot = HPDF_Page_CreateURILinkAnnot(page, rect, urlp);
      HPDF_LinkAnnot_SetBorderStyle (annot, 0, 0, 0);
    }

    else {
      /* if the line is a content line then clip the line and print it */
      page_type |= CONTENT;
      /* if page is a banner slide, skip contents */
      if(page_type & BANNER) continue;
      if(page_type & IMAGE){
        /* set content_y_multiplier to be after image */
        content_y_multiplier = min(0.5 - (image_height_multiplier / 2) - 0.05, content_y_multiplier);
        content_margin_multiplier = 0.5 - (image_width_multiplier / 2);
      }
      if(content_y_multiplier < content_y_min_multiplier){
        printf("Line %d: too much content on slide %d. Skipping\n", line_number, page_number);
        continue;
      }
      cliptext(linebuffer, width*(1-content_margin_multiplier * 2), line_number, print_content);
      content_y_multiplier -= 0.05;
    }
  }
  
  /* save the file and exit safely */
  int src_filename_length = strlen(argv[1]);
  char dest_filename[src_filename_length+5];
  memset(dest_filename, 0, src_filename_length + 5);
  strncpy(dest_filename, argv[1], src_filename_length);
  strcat(dest_filename, ".pdf");
  HPDF_SaveToFile(pdf,dest_filename);
  HPDF_Free(pdf);

  /* close src file */
  fclose(src);
  return 0;
}
