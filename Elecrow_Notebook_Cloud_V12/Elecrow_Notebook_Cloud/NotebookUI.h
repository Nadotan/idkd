#pragma once
#include <lvgl.h>
#include <SPIFFS.h>
#include <esp_heap_caps.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

namespace Notebook {
constexpr int W=980, H=485;
static uint16_t *frame=nullptr;
static lv_obj_t *canvas=nullptr, *status=nullptr;
static uint16_t color=0x18C3;
static int brush=2;
static bool down=false, fsReady=false;
static lv_point_t last{};
static const char *FILE_NAME="/notebook.raw";
static uint16_t rgb(uint32_t c){ return lv_color_to_u16(lv_color_hex(c)); }
static void note(const char *v){ if(status) lv_label_set_text(status,v); }
static void dot(int x,int y){
  if(!frame)return;
  for(int j=-brush;j<=brush;j++) for(int i=-brush;i<=brush;i++){
    int xx=x+i, yy=y+j;
    if(i*i+j*j<=brush*brush && xx>=0 && yy>=0 && xx<W && yy<H)frame[yy*W+xx]=color;
  }
}
static void line(int x0,int y0,int x1,int y1){
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
  while(true){dot(x0,y0);if(x0==x1 && y0==y1)break;int d=2*err;if(d>=dy){err+=dy;x0+=sx;}if(d<=dx){err+=dx;y0+=sy;}}
}
static void touch(lv_event_t *e){
  lv_event_code_t event=lv_event_get_code(e);
  if(event==LV_EVENT_RELEASED || event==LV_EVENT_PRESS_LOST){down=false;return;}
  if(event!=LV_EVENT_PRESSED && event!=LV_EVENT_PRESSING)return;
  lv_indev_t *dev=lv_indev_active(); if(!dev)return;
  lv_point_t p; lv_indev_get_point(dev,&p);
  lv_area_t a;lv_obj_get_coords(canvas,&a);
  p.x-=a.x1;p.y-=a.y1;
  if(p.x<0 || p.y<0 || p.x>=W || p.y>=H){down=false;return;}
  if(down)line(last.x,last.y,p.x,p.y);else dot(p.x,p.y);
  last=p;down=true;lv_obj_invalidate(canvas);
}
static void clear(){if(frame){for(size_t i=0;i<(size_t)W*H;i++)frame[i]=rgb(0xFFFFFF);lv_obj_invalidate(canvas);}}
static bool save(){
  if(!fsReady || !frame)return false;
  File f=SPIFFS.open(FILE_NAME,"w");if(!f)return false;
  const uint8_t *p=(uint8_t*)frame;size_t remain=(size_t)W*H*2;
  while(remain){size_t n=remain>4096?4096:remain;size_t k=f.write(p,n);if(k!=n){f.close();return false;}remain-=k;p+=k;}
  f.close();return true;
}
static bool load(){
  if(!fsReady || !frame)return false;
  File f=SPIFFS.open(FILE_NAME,"r");if(!f || f.size()!=(size_t)W*H*2){if(f)f.close();return false;}
  uint8_t *p=(uint8_t*)frame;size_t remain=(size_t)W*H*2;
  while(remain){size_t n=remain>4096?4096:remain;size_t k=f.read(p,n);if(!k){f.close();return false;}remain-=k;p+=k;}
  f.close();lv_obj_invalidate(canvas);return true;
}
static void action(lv_event_t *e){
  unsigned id=(uintptr_t)lv_event_get_user_data(e);
  down=false;
  switch(id){
    case 1:color=rgb(0x202734);brush=2;note("Black pen");break;
    case 2:color=rgb(0xD93E47);brush=2;note("Red pen");break;
    case 3:color=rgb(0x2176CB);brush=2;note("Blue pen");break;
    case 4:color=rgb(0xFFFFFF);brush=12;note("Eraser");break;
    case 5:clear();note("Cleared. SAVE to keep");break;
    case 6:note(save()?"Saved to flash":"Save failed: check SPIFFS partition");break;
    case 7:note(load()?"Loaded saved page":"No saved page / load failed");break;
  }
}
static void add_button(lv_obj_t *bar,const char *text,int x,int width,unsigned id){
  lv_obj_t *b=lv_button_create(bar);lv_obj_set_size(b,width,44);lv_obj_set_pos(b,x,12);
  lv_obj_add_event_cb(b,action,LV_EVENT_CLICKED,(void*)(uintptr_t)id);
  lv_obj_t *l=lv_label_create(b);lv_label_set_text(l,text);lv_obj_center(l);
}
static void start(){
  if(!lvgl_port_lock(-1)){Serial.println("LVGL lock failed");return;}
  fsReady=SPIFFS.begin(false); // Never format user storage automatically
  lv_obj_t *scr=lv_screen_active();lv_obj_set_style_bg_color(scr,lv_color_hex(0xE8ECF3),0);
  lv_obj_remove_flag(scr,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *bar=lv_obj_create(scr);lv_obj_set_pos(bar,0,0);lv_obj_set_size(bar,1024,77);
  lv_obj_set_style_bg_color(bar,lv_color_hex(0x263348),0);lv_obj_set_style_border_width(bar,0,0);
  lv_obj_set_style_pad_all(bar,0,0);lv_obj_remove_flag(bar,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_text_color(bar,lv_color_white(),0);
  add_button(bar,"BLACK",12,116,1);add_button(bar,"RED",137,100,2);
  add_button(bar,"BLUE",246,100,3);add_button(bar,"ERASER",355,126,4);
  add_button(bar,"CLEAR",490,116,5);add_button(bar,"SAVE",615,119,6);
  add_button(bar,"LOAD",743,119,7);
  status=lv_label_create(bar);lv_obj_set_pos(status,15,59);lv_label_set_text(status,"Draw with your finger");
  frame=(uint16_t*)heap_caps_malloc((size_t)W*H*2,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!frame){note("ERROR: no PSRAM for canvas");lvgl_port_unlock();return;}
  canvas=lv_canvas_create(scr);
  lv_canvas_set_buffer(canvas,frame,W,H,LV_COLOR_FORMAT_RGB565);
  lv_obj_set_pos(canvas,22,94);clear();
  lv_obj_add_flag(canvas,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(canvas,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(canvas,touch,LV_EVENT_ALL,nullptr);
  color=rgb(0x202734);
  if(fsReady && load())note("Saved page restored");
  else if(!fsReady)note("Drawing ready; storage unavailable");
  lvgl_port_unlock();
}
}
