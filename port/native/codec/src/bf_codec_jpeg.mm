// JPEG through ImageIO. Colours are passed through untouched: images are
// retagged as device colour space before drawing, as the Intel library
// knew no colour management.

#include "bf_codec.h"
#include "bf_trace.h"

#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>

#include <stdlib.h>
#include <string.h>
#include <vector>

static int bytes_per_pixel(bf_codec_pixel_format f)
{
	return f == BF_CODEC_BGR24 ? 3 : f == BF_CODEC_BGRA32 ? 4 : 0;
}

static CGImageSourceRef open_source(const void *data, size_t size)
{
	if (!data || !size) return NULL;
	CFDataRef cf = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)data, (CFIndex)size);
	if (!cf) return NULL;
	CGImageSourceRef src = CGImageSourceCreateWithData(cf, NULL);
	CFRelease(cf);
	if (src) {
		CFStringRef type = CGImageSourceGetType(src);
		if (!type || !CFEqual(type, CFSTR("public.jpeg")) || CGImageSourceGetCount(src) < 1) {
			CFRelease(src);
			return NULL;
		}
	}
	return src;
}

extern "C" int bf_codec_jpeg_info(const void *data, size_t size, int *width, int *height)
{
	if (!data || !size || !width || !height) {
		BF_TRACE(BFT_ERR, "codec: jpeg_info: bad argument");
		return BF_CODEC_E_ARG;
	}
	int rc = BF_CODEC_E_DATA;
	CGImageSourceRef src = open_source(data, size);
	if (src) {
		CFDictionaryRef props = CGImageSourceCopyPropertiesAtIndex(src, 0, NULL);
		if (props) {
			int w = 0, h = 0;
			CFNumberRef nw = (CFNumberRef)CFDictionaryGetValue(props, kCGImagePropertyPixelWidth);
			CFNumberRef nh = (CFNumberRef)CFDictionaryGetValue(props, kCGImagePropertyPixelHeight);
			if (nw && nh && CFNumberGetValue(nw, kCFNumberIntType, &w) &&
			    CFNumberGetValue(nh, kCFNumberIntType, &h) && w > 0 && h > 0) {
				*width = w;
				*height = h;
				rc = BF_CODEC_OK;
			}
			CFRelease(props);
		}
		CFRelease(src);
	}
	if (rc) BF_TRACE(BFT_ERR, "codec: jpeg_info: not a readable JPEG (%zu bytes)", size);
	return rc;
}

extern "C" int bf_codec_jpeg_decode(const void *data, size_t size, void *pixels,
                                    int width, int height, size_t stride,
                                    bf_codec_pixel_format format)
{
	int bpp = bytes_per_pixel(format);
	if (!data || !size || !pixels || width <= 0 || height <= 0 || !bpp) {
		BF_TRACE(BFT_ERR, "codec: jpeg_decode: bad argument");
		return BF_CODEC_E_ARG;
	}
	if (!stride) stride = (size_t)width * bpp;
	if (stride < (size_t)width * bpp) {
		BF_TRACE(BFT_ERR, "codec: jpeg_decode: stride %zu too small", stride);
		return BF_CODEC_E_ARG;
	}

	int rc = BF_CODEC_E_DATA;
	@autoreleasepool {
		CGImageSourceRef src = open_source(data, size);
		CGImageRef image = src ? CGImageSourceCreateImageAtIndex(src, 0, NULL) : NULL;
		if (src) CFRelease(src);
		if (!image) {
			BF_TRACE(BFT_ERR, "codec: jpeg_decode: cannot decode %zu bytes", size);
			return BF_CODEC_E_DATA;
		}
		if ((int)CGImageGetWidth(image) != width || (int)CGImageGetHeight(image) != height) {
			BF_TRACE(BFT_ERR, "codec: jpeg_decode: image is %zux%zu, caller said %dx%d",
			         CGImageGetWidth(image), CGImageGetHeight(image), width, height);
			CGImageRelease(image);
			return BF_CODEC_E_ARG;
		}

		// Retag RGB and grey images as device spaces so that drawing copies values.
		CGColorSpaceRef cs = CGImageGetColorSpace(image);
		CGImageRef tagged = NULL;
		if (cs) {
			CGColorSpaceModel model = CGColorSpaceGetModel(cs);
			CGColorSpaceRef dev = model == kCGColorSpaceModelRGB ? CGColorSpaceCreateDeviceRGB()
			                    : model == kCGColorSpaceModelMonochrome ? CGColorSpaceCreateDeviceGray() : NULL;
			if (dev) {
				tagged = CGImageCreateCopyWithColorSpace(image, dev);
				CGColorSpaceRelease(dev);
			}
		}
		CGImageRef use = tagged ? tagged : image;

		// Draw into BGRX, in the caller's buffer for BGRA32, else in a temporary.
		std::vector<uint8_t> temp;
		uint8_t *buf = (uint8_t *)pixels;
		size_t bstride = stride;
		if (format == BF_CODEC_BGR24) {
			bstride = (size_t)width * 4;
			try { temp.resize(bstride * height); } catch (...) { temp.clear(); }
			buf = temp.data();
		}
		if (!buf) {
			rc = BF_CODEC_E_MEMORY;
		} else {
			CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
			CGContextRef ctx = CGBitmapContextCreate(buf, width, height, 8, bstride, rgb,
				(CGBitmapInfo)kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
			CGColorSpaceRelease(rgb);
			if (!ctx) {
				rc = BF_CODEC_E_MEMORY;
			} else {
				CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
				CGContextDrawImage(ctx, CGRectMake(0, 0, width, height), use);
				CGContextRelease(ctx);
				if (format == BF_CODEC_BGRA32) {
					for (int y = 0; y < height; y++) {
						uint8_t *p = buf + (size_t)y * bstride;
						for (int x = 0; x < width; x++) p[x * 4 + 3] = 0xff;
					}
				} else {
					for (int y = 0; y < height; y++) {
						const uint8_t *s = buf + (size_t)y * bstride;
						uint8_t *d = (uint8_t *)pixels + (size_t)y * stride;
						for (int x = 0; x < width; x++) {
							d[x * 3 + 0] = s[x * 4 + 0];
							d[x * 3 + 1] = s[x * 4 + 1];
							d[x * 3 + 2] = s[x * 4 + 2];
						}
					}
				}
				rc = BF_CODEC_OK;
			}
		}
		if (rc == BF_CODEC_E_MEMORY) BF_TRACE(BFT_ERR, "codec: jpeg_decode: out of memory");
		if (tagged) CGImageRelease(tagged);
		CGImageRelease(image);
	}
	return rc;
}

// Builds the JPEG in a CFMutableData (the file variant writes it out).
static int encode(CFMutableDataRef out, CFURLRef url, const void *pixels,
                  int width, int height, size_t stride,
                  bf_codec_pixel_format format, int quality)
{
	int bpp = bytes_per_pixel(format);
	if (!pixels || width <= 0 || height <= 0 || !bpp || quality < 1 || quality > 100) {
		BF_TRACE(BFT_ERR, "codec: jpeg_encode: bad argument");
		return BF_CODEC_E_ARG;
	}
	if (!stride) stride = (size_t)width * bpp;
	if (stride < (size_t)width * bpp) {
		BF_TRACE(BFT_ERR, "codec: jpeg_encode: stride %zu too small", stride);
		return BF_CODEC_E_ARG;
	}

	int rc = BF_CODEC_E_MEMORY;
	@autoreleasepool {
		std::vector<uint8_t> temp;
		const uint8_t *buf = (const uint8_t *)pixels;
		size_t bstride = stride;
		if (format == BF_CODEC_BGR24) {
			bstride = (size_t)width * 4;
			try { temp.resize(bstride * height); } catch (...) { temp.clear(); }
			if (temp.empty()) {
				BF_TRACE(BFT_ERR, "codec: jpeg_encode: out of memory");
				return BF_CODEC_E_MEMORY;
			}
			for (int y = 0; y < height; y++) {
				const uint8_t *s = (const uint8_t *)pixels + (size_t)y * stride;
				uint8_t *d = temp.data() + (size_t)y * bstride;
				for (int x = 0; x < width; x++) {
					d[x * 4 + 0] = s[x * 3 + 0];
					d[x * 4 + 1] = s[x * 3 + 1];
					d[x * 4 + 2] = s[x * 3 + 2];
					d[x * 4 + 3] = 0xff;
				}
			}
			buf = temp.data();
		}

		CGDataProviderRef prov = CGDataProviderCreateWithData(NULL, buf, bstride * height, NULL);
		CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
		CGImageRef image = prov ? CGImageCreate(width, height, 8, 32, bstride, rgb,
			(CGBitmapInfo)kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
			prov, NULL, false, kCGRenderingIntentDefault) : NULL;
		CGColorSpaceRelease(rgb);
		if (prov) CGDataProviderRelease(prov);

		CGImageDestinationRef dest = NULL;
		if (image) {
			dest = out ? CGImageDestinationCreateWithData(out, CFSTR("public.jpeg"), 1, NULL)
			           : CGImageDestinationCreateWithURL(url, CFSTR("public.jpeg"), 1, NULL);
		}
		if (dest) {
			float q = quality / 100.0f;
			CFNumberRef nq = CFNumberCreate(NULL, kCFNumberFloatType, &q);
			const void *keys[] = { kCGImageDestinationLossyCompressionQuality };
			const void *vals[] = { nq };
			CFDictionaryRef opts = CFDictionaryCreate(NULL, keys, vals, 1,
				&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CGImageDestinationAddImage(dest, image, opts);
			if (CGImageDestinationFinalize(dest)) rc = BF_CODEC_OK;
			else rc = out ? BF_CODEC_E_MEMORY : BF_CODEC_E_IO;
			CFRelease(opts);
			CFRelease(nq);
			CFRelease(dest);
		} else if (image) {
			rc = out ? BF_CODEC_E_MEMORY : BF_CODEC_E_IO;
		}
		if (image) CGImageRelease(image);
		if (rc) BF_TRACE(BFT_ERR, "codec: jpeg_encode: failed (%d) for %dx%d", rc, width, height);
	}
	return rc;
}

extern "C" int bf_codec_jpeg_encode_mem(void **out, size_t *out_size, const void *pixels,
                                        int width, int height, size_t stride,
                                        bf_codec_pixel_format format, int quality)
{
	if (!out || !out_size) {
		BF_TRACE(BFT_ERR, "codec: jpeg_encode_mem: bad argument");
		return BF_CODEC_E_ARG;
	}
	*out = NULL;
	*out_size = 0;
	CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
	if (!data) return BF_CODEC_E_MEMORY;
	int rc = encode(data, NULL, pixels, width, height, stride, format, quality);
	if (rc == BF_CODEC_OK) {
		size_t n = (size_t)CFDataGetLength(data);
		void *p = n ? malloc(n) : NULL;
		if (!p) {
			rc = BF_CODEC_E_MEMORY;
		} else {
			memcpy(p, CFDataGetBytePtr(data), n);
			*out = p;
			*out_size = n;
		}
	}
	CFRelease(data);
	return rc;
}

extern "C" int bf_codec_jpeg_encode_file(const char *path, const void *pixels,
                                         int width, int height, size_t stride,
                                         bf_codec_pixel_format format, int quality)
{
	if (!path || !*path) {
		BF_TRACE(BFT_ERR, "codec: jpeg_encode_file: no path");
		return BF_CODEC_E_ARG;
	}
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
		(const UInt8 *)path, (CFIndex)strlen(path), false);
	if (!url) return BF_CODEC_E_ARG;
	int rc = encode(NULL, url, pixels, width, height, stride, format, quality);
	CFRelease(url);
	if (rc == BF_CODEC_E_IO) BF_TRACE(BFT_ERR, "codec: jpeg_encode_file: cannot write %s", path);
	return rc;
}

extern "C" void bf_codec_free(void *p)
{
	free(p);
}
