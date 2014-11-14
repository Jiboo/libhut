/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <GLES2/gl2.h>

#include "libhut/wayland/application.hpp"
#include "libhut/wayland/window.hpp"
#include "libhut/wayland/display.hpp"

namespace hut {

    std::string to_utf8(char32_t ch) {
        //Credits to libcaca
        static const uint8_t mark[7] = {
                0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
        };

        if(ch < 0x80) {
            return std::string {(char)ch};
        }
        else {
            char result[4];
            size_t bytes = (ch < 0x800) ? 2 : (ch < 0x10000) ? 3 : 4;

            auto it = result + bytes;

            switch(bytes)
            {
                case 4: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
                case 3: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
                case 2: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
            }
            *--it = (char)(ch | mark[bytes]);

            return std::string {result, bytes};
        }

        return std::string {};
    }

    std::shared_ptr<texture> load_png(std::string filename) {
        // https://github.com/DavidEGrayson/ahrs-visualizer/blob/master/png_texture.cpp

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if(!png_ptr) {
            throw std::runtime_error("png_create_read_struct failed");
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if(!info_ptr) {
            png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
            throw std::runtime_error("png_create_info_struct failed");
        }

        png_infop end_info = png_create_info_struct(png_ptr);
        if(!end_info) {
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
            throw std::runtime_error("png_create_info_struct failed");
        }

        // the code in this if statement gets called if libpng encounters an error
        if(setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            throw std::runtime_error("setjmp failed");
        }

        png_byte header[8];
        FILE *fp = fopen(filename.c_str(), "rb");
        if (!fp) {
            throw std::runtime_error("fopen failed");
        }

        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8)) {
            fclose(fp);
            throw std::runtime_error("not a png");
        }

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);

        uivec2 size;
        size[0] = png_get_image_width(png_ptr, info_ptr);
        size[1] = png_get_image_height(png_ptr, info_ptr);
        uint32_t bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        uint32_t channels = png_get_channels(png_ptr, info_ptr);
        uint32_t color_type = png_get_color_type(png_ptr, info_ptr);

        if(bit_depth == 16) {
            png_set_strip_16(png_ptr);
            bit_depth = 8;
        }

        pixel_format format;
        switch(color_type) {
            case PNG_COLOR_TYPE_PALETTE:
                format = pixel_format::RGBA8888;
                png_set_palette_to_rgb(png_ptr);
                bit_depth = 8;
                channels = 3;
                break;
            case PNG_COLOR_TYPE_GRAY:
                format = pixel_format::L8;
                if(bit_depth < 8)
                    png_set_expand_gray_1_2_4_to_8(png_ptr);
                bit_depth = 8;
                break;
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                format = pixel_format::LA88;
                if(bit_depth < 8)
                    png_set_expand_gray_1_2_4_to_8(png_ptr);
                bit_depth = 8;
                break;
            case PNG_COLOR_TYPE_RGB:
                if(bit_depth != 8)
                    throw std::runtime_error("unsuported bit depth for RGB color type, only 8 is supported");
                format = pixel_format::RGBA8888;
                break;
            case PNG_COLOR_TYPE_RGB_ALPHA:
                if(bit_depth != 8 && bit_depth != 4)
                    throw std::runtime_error("unsuported bit depth for RGBA color type, only 4 and 8 is supported");
                format = bit_depth == 8 ? pixel_format::RGBA8888 : pixel_format::RGBA4444;
                break;
            default:
                png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
                fclose(fp);
                throw std::runtime_error("unsuported color type");
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr);
            channels += 1;
        }

        if(channels == 3) {
            png_set_filler(png_ptr, (png_uint_32)0xFF, PNG_FILLER_AFTER);
            channels += 1;
        }

        png_read_update_info(png_ptr, info_ptr);
        png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        rowbytes += 3 - ((rowbytes-1) % 4); // glTexImage2D requires rows to be 4-byte aligned

        png_byte * image_data = (png_byte*)malloc(rowbytes * size[1] * sizeof(png_byte)+15);
        if (image_data == NULL) {
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            fclose(fp);
            throw std::runtime_error("malloc failed");
        }

        png_bytep * row_pointers = (png_bytep*)malloc(size[1] * sizeof(png_bytep));
        if (row_pointers == NULL) {
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            free(image_data);
            fclose(fp);
            throw std::runtime_error("malloc failed");
        }

        for(uint32_t i = 0; i < size[1]; i++) {
            row_pointers[i] = image_data + i * rowbytes;
        }

        png_read_image(png_ptr, row_pointers);

        texture* result = new texture(size, format, image_data, format, true);

        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        free(image_data);
        free(row_pointers);
        fclose(fp);

        return std::shared_ptr<texture>(result);
    }

} //namespace hut