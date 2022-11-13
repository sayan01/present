#include<hpdf.h>
int strendswith(const char *str, const char *suffix);
void error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data);
void outtextxy(int x, int y, const char* text, int fontsize);
void centertext(int y, const char* text, int fontsize);
void centertextinpage(const char* text, int fontsize);
void newpage();
void print_page_number(int page_number);
void print_title(const char* text);
