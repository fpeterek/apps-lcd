// **************************************************************************
//
//               Demo program for labs
//
// Subject:      Computer Architectures and Parallel systems
// Author:       Petr Olivka, petr.olivka@vsb.cz, 09/2019
// Organization: Department of Computer Science, FEECS,
//               VSB-Technical University of Ostrava, CZ
//
// File:         OpenCV simulator of LCD
//
// **************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "font8x8.cpp"
#include "fonts/font32x53_lsb.h"

#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#define LCD_NAME        "Virtual LCD"

cv::Mat g_canvas( cv::Size( LCD_WIDTH, LCD_HEIGHT ), CV_8UC3 );

void lcd_put_pixel( int t_x, int t_y, int t_rgb_565 ) {
    cv::Vec3b l_rgb_888( 
            (  t_rgb_565         & 0x1F ) << 3, 
            (( t_rgb_565 >> 5 )  & 0x3F ) << 2, 
            (( t_rgb_565 >> 11 ) & 0x1F ) << 3
            );
    g_canvas.at<cv::Vec3b>( t_y, t_x ) = l_rgb_888;
}

void lcd_clear() {
    cv::Vec3b l_black( 0, 0, 0 );
    g_canvas.setTo( l_black );
}

void lcd_init() {
    cv::namedWindow( LCD_NAME, 0 );
    lcd_clear();
    cv::waitKey( 1 );
}

void test() {

    int l_color_red = 0xF800;
    int l_color_green = 0x07E0;
    int l_color_blue = 0x001F;
    int l_color_white = 0xFFFF;

    int l_limit = 200;

    for ( int ofs = 0; ofs < 20; ofs++ ) {

        for (int i = 0; i < l_limit; i++) {
            lcd_put_pixel(ofs + i, ofs + 0, l_color_red);
            lcd_put_pixel(ofs + 0, ofs + i, l_color_green);
            lcd_put_pixel(ofs + i, ofs + l_limit, l_color_blue);
            lcd_put_pixel(ofs + l_limit, ofs + i, l_color_white);
        }

    }

}

struct Point2D {

    int32_t x;
    int32_t y;

    Point2D(int32_t x, int32_t y) : x(x), y(y) { }

};

struct RGB {

    uint8_t r;
    uint8_t g;
    uint8_t b;

    RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) { }

};

class GraphicElement {

protected:

    void swap_fg_bg_color() {
        RGB l_tmp = fg_color;
        fg_color = bg_color;
        bg_color = l_tmp;
    }

    static int convert_RGB888_to_RGB565(RGB t_color) {

        const int red = (t_color.r >> 3) << 11;
        const int green = (t_color.g >> 2) << 6;
        const int blue = (t_color.b >> 3);

        return red | green | blue;
    }

public:
    RGB fg_color;
    RGB bg_color;

    GraphicElement(RGB t_fg_color, RGB t_bg_color ) : fg_color( t_fg_color ), bg_color( t_bg_color ) {}

    void drawPixel( int32_t t_x, int32_t t_y ) {
        lcd_put_pixel( t_x, t_y, convert_RGB888_to_RGB565( fg_color ) );
    }

    virtual void draw() = 0;

    virtual void hide() {
        swap_fg_bg_color();
        draw();
        swap_fg_bg_color();
    }

};


class Pixel : public GraphicElement {

public:

    Pixel( Point2D t_pos, RGB t_fg_color, RGB t_bg_color ) : pos( t_pos ), GraphicElement(t_fg_color, t_bg_color ) {}

    virtual void draw() {
        drawPixel( pos.x, pos.y );
    }

    Point2D pos;

};


class Circle : public GraphicElement {

public:

    Point2D center;

    int32_t radius;

    Circle( Point2D t_center, int32_t t_radius, RGB t_fg, RGB t_bg ) :
            center( t_center ), radius( t_radius ), GraphicElement(t_fg, t_bg ) {};

    void draw() {

        int f = 1 - radius;
        int ddF_x = 0;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;

        int x0 = center.x;
        int y0 = center.y;

        int fg = convert_RGB888_to_RGB565(fg_color);

        lcd_put_pixel(x0, y0 + radius, fg);
        lcd_put_pixel(x0, y0 - radius, fg);
        lcd_put_pixel(x0 + radius, y0, fg);
        lcd_put_pixel(x0 - radius, y0, fg);

        while(x < y)
        {
            if(f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x + 1;
            lcd_put_pixel(x0 + x, y0 + y, fg);
            lcd_put_pixel(x0 - x, y0 + y, fg);
            lcd_put_pixel(x0 + x, y0 - y, fg);
            lcd_put_pixel(x0 - x, y0 - y, fg);
            lcd_put_pixel(x0 + y, y0 + x, fg);
            lcd_put_pixel(x0 - y, y0 + x, fg);
            lcd_put_pixel(x0 + y, y0 - x, fg);
            lcd_put_pixel(x0 - y, y0 - x, fg);
        }

    }

};

template<typename type, int width, int height>
class Character : public GraphicElement {

    typedef type Font[256][height];

    const Font * font;

public:

    Point2D pos;

    char character;

    void setFont(const Font * const f) {
        font = f;
    }

    Font * getFont() {
        return font;
    }

    Character() : pos(0, 0), GraphicElement(RGB(255, 255, 255), RGB(0, 0, 0)) {
        character = ' ';
    }

    Character( Point2D t_pos, char t_char, RGB t_fg, RGB t_bg ) :
            pos( t_pos ), character( t_char ), GraphicElement(t_fg, t_bg ) {};

    Character & operator=(Character orig) {
        pos = orig.pos;
        font = orig.font;
        character = orig.character;
        return *this;
    }

    void draw() {

        int x = pos.x;
        int y = pos.y;

        int color = convert_RGB888_to_RGB565(fg_color);

        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if ((*font)[character][i] & 1 << j) {
                    lcd_put_pixel(x + j, y + i, color);
                }
            }
        }

    };

};

template<typename type, int width, int height>
class Text : public GraphicElement {

    typedef type Font[256][height];
    typedef Character<type, width, height> Char;

    Char * chars = nullptr;

    uint64_t len;

    const Font * font = nullptr;

public:

    Point2D pos;

    Text(const char * str, Point2D pos, RGB foreground, RGB background) :
            GraphicElement(foreground, background), pos(pos) {

        len = strlen(str);
        chars = new Char[len];
        for (uint64_t i = 0; i < len; ++i) {
            chars[i] = Char(Point2D(pos.x + i*width, pos.y), str[i], fg_color, bg_color);
        }

    }

    void draw() {

        for (uint64_t i = 0; i < len; ++i) {
            chars[i].draw();
        }

    }

    void hide() {

        for (uint64_t i = 0; i < len; ++i) {
            chars[i].hide();
        }

    }

    void setFont(const Font * const f) {
        font = f;
        for (uint64_t i = 0; i < len; ++i) {
            chars[i].setFont(f);
        }
    }

    Font * getFont() {
        return font;
    }

    ~Text() {
        if (chars) {
            delete[] chars;
        }
    }

};

class Line : public GraphicElement {

public:

    Point2D pos1, pos2;

    Line( Point2D t_pos1, Point2D t_pos2, RGB t_fg, RGB t_bg ) :
            pos1( t_pos1 ), pos2( t_pos2 ), GraphicElement(t_fg, t_bg ) {}

    void draw() {

        int x0 = pos1.x;
        int x1 = pos2.x;
        int y0 = pos1.y;
        int y1 = pos2.y;

        int fg = convert_RGB888_to_RGB565(fg_color);

        int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
        int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
        int err = dx+dy, e2; /* error value e_xy */

        for(;;){  /* loop */
            lcd_put_pixel(x0,y0, fg);
            if (x0==x1 && y0==y1) break;
            e2 = 2*err;
            if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
            if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
        }

    };

};

int main() {

    lcd_init();
    lcd_clear();


    Circle circle(Point2D(100, 100), 80, RGB(255, 0, 0), RGB(0, 255, 0));
    circle.draw();

    Line line(Point2D(200, 150), Point2D(300, 80), RGB(0, 255, 255), RGB(0, 255, 0));
    line.draw();

    Character<uint8_t, 8, 8> character(Point2D(100, 100), 't', RGB(255, 255, 255), RGB(0, 255, 0));
    character.setFont(&font8x8);
    character.draw();

    Character<uint32_t, 32, 53> c1(Point2D(110, 100), 'A', RGB(255, 255, 255), RGB(0, 255, 0));
    c1.setFont(&font);
    c1.draw();

    Text<uint32_t, 32, 53> text("Seznam.cz", Point2D(10, 180), RGB(255, 255, 255), RGB(0, 0, 0));
    text.setFont(&font);
    text.draw();

    cv::imshow(LCD_NAME, g_canvas);
    cv::waitKey(0);
}
