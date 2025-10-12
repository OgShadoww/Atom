// palette.c — truecolor + 256-color fallback + UTF-8 safe + white color
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <math.h>

typedef struct { const char *name; int r,g,b; } Color;

static const Color P[] = {
  {"bg",      0x0e,0x0f,0x14},
  {"fg",      0xd6,0xd6,0xd6},
  {"white",   0xff,0xff,0xff},
  {"cyan",    0x00,0xd8,0xff},
  {"blue",    0x61,0xaf,0xef},
  {"violet",  0xc6,0x78,0xdd},
  {"green",   0x98,0xc3,0x79},
  {"orange",  0xd1,0x9a,0x66},
  {"red",     0xe0,0x6c,0x75},
  {"gray",    0x5c,0x63,0x70}
};
#define N (int)(sizeof(P)/sizeof(P[0]))

static int use_truecolor(void){
  const char* force = getenv("ATOM_COLOR_MODE");
  if (force){
    if (!strcmp(force,"truecolor")) return 1;
    if (!strcmp(force,"256")) return 0;
  }
  const char* ct = getenv("COLORTERM");
  if (ct && (strstr(ct,"truecolor") || strstr(ct,"24bit"))) return 1;
  const char* prog = getenv("TERM_PROGRAM");
  if (prog && (strstr(prog,"Ghostty") || strstr(prog,"iTerm"))) return 1;
  return 0;
}

static void reset(void){ fputs("\x1b[0m", stdout); }
static void set_fg_true(int r,int g,int b){ printf("\x1b[38;2;%d;%d;%dm",r,g,b); }
static void set_bg_true(int r,int g,int b){ printf("\x1b[48;2;%d;%d;%dm",r,g,b); }

static int rgb_to_256(int r,int g,int b){
  if (r==g && g==b){
    if (r < 8) return 16;
    if (r > 248) return 231;
    return 232 + (int)((r - 8) / 10.7);
  }
  int ri = (int)round(r/255.0*5), gi = (int)round(g/255.0*5), bi = (int)round(b/255.0*5);
  if (ri<0)ri=0; if (ri>5)ri=5; if (gi<0)gi=0; if (gi>5)gi=5; if (bi<0)bi=0; if (bi>5)bi=5;
  return 16 + 36*ri + 6*gi + bi;
}
static void set_fg_256(int r,int g,int b){ printf("\x1b[38;5;%dm", rgb_to_256(r,g,b)); }
static void set_bg_256(int r,int g,int b){ printf("\x1b[48;5;%dm", rgb_to_256(r,g,b)); }

static void set_fg(int r,int g,int b,int truec){ truec? set_fg_true(r,g,b): set_fg_256(r,g,b); }
static void set_bg(int r,int g,int b,int truec){ truec? set_bg_true(r,g,b): set_bg_256(r,g,b); }

int main(void){
  setlocale(LC_ALL, "");
  const int truec = use_truecolor();
  const char *sample = "Sample: 原子 テキストエディタ | ATOM EDITOR";

  // Foreground test
  set_bg(P[0].r,P[0].g,P[0].b,truec);
  set_fg(0xd0,0xd0,0xd0,truec);
  printf("\n%s (%s)\n\n", "Foreground on base background", truec? "truecolor" : "xterm-256");

  for (int i=0;i<N;i++){
    set_bg(P[0].r,P[0].g,P[0].b,truec);
    set_fg(0x88,0x8a,0x92,truec);
    printf(" %-8s ", P[i].name);
    set_fg(P[i].r,P[i].g,P[i].b,truec);
    printf("%s", sample);
    reset(); puts("");
  }
  puts("");

  // Background test
  printf("%s\n\n", "Background swatches (light fg / dark fg)");
  for (int i=0;i<N;i++){
    set_bg(P[i].r,P[i].g,P[i].b,truec);
    set_fg(P[2].r,P[2].g,P[2].b,truec); // white fg
    printf(" %-8s  %s ", P[i].name, sample);
    reset(); printf("  ");
    set_bg(P[i].r,P[i].g,P[i].b,truec);
    set_fg(P[0].r,P[0].g,P[0].b,truec); // bg color as dark fg
    printf("%s", sample);
    reset(); puts("");
  }
  reset(); puts("");

  // Legend
  puts("Legend:");
  for (int i=0;i<N;i++){
    set_fg(P[i].r,P[i].g,P[i].b,truec); fputs("■", stdout); reset();
    printf(" %s  ", P[i].name);
  }
  puts("\n");

  puts("(Tip) Force modes:");
  puts("  ATOM_COLOR_MODE=truecolor ./palette   # force 24-bit");
  puts("  ATOM_COLOR_MODE=256      ./palette   # force xterm-256");
  puts("");
  return 0;
}

