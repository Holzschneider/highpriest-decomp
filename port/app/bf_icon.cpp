// bifrost-icon: draws the application icon (M12.1) and writes it as the
// PNG files of an .iconset, which the build turns into the bundle's .icns
// with iconutil. The picture is made here, from shapes only: a night sky in
// the rounded square of a macOS icon, crossed by a rainbow bridge. No game
// artwork goes into it.
//
//   bifrost-icon <folder.iconset>
//
// Core Graphics and ImageIO only; deterministic (same bytes every build).
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include <math.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

namespace {

CGColorRef Rgb(CGColorSpaceRef space, double r, double g, double b, double a = 1.0)
{
	const CGFloat c[4] = {(CGFloat)r, (CGFloat)g, (CGFloat)b, (CGFloat)a};
	return CGColorCreate(space, c);
}

// The picture on a square of `size` pixels; the geometry is that of a
// 1024-pixel icon scaled down (the body is 824 of 1024, corner radius 185:
// Apple's macOS icon grid).
void Draw(CGContextRef ctx, CGColorSpaceRef space, double size)
{
	double k = size / 1024.0;
	CGContextScaleCTM(ctx, k, k);
	CGRect body = CGRectMake(100, 100, 824, 824);
	CGPathRef shape = CGPathCreateWithRoundedRect(body, 185, 185, nullptr);

	// Shadow under the body.
	CGContextSaveGState(ctx);
	CGColorRef shadow = Rgb(space, 0, 0, 0, 0.35);
	CGContextSetShadowWithColor(ctx, CGSizeMake(0, -10), 28, shadow);
	CGContextAddPath(ctx, shape);
	CGContextSetFillColorWithColor(ctx, shadow);
	CGContextFillPath(ctx);
	CGColorRelease(shadow);
	CGContextRestoreGState(ctx);

	CGContextSaveGState(ctx);
	CGContextAddPath(ctx, shape);
	CGContextClip(ctx);

	// The sky: a vertical gradient, dark at the top.
	const CGFloat sky[] = {0.04, 0.07, 0.20, 1.0, 0.13, 0.22, 0.48, 1.0};
	const CGFloat at[] = {0.0, 1.0};
	CGGradientRef g = CGGradientCreateWithColorComponents(space, sky, at, 2);
	CGContextDrawLinearGradient(ctx, g, CGPointMake(0, 924), CGPointMake(0, 100), 0);
	CGGradientRelease(g);

	// Stars, from a fixed sequence.
	unsigned seed = 20080401;
	CGColorRef star = Rgb(space, 1, 1, 0.95, 0.9);
	CGContextSetFillColorWithColor(ctx, star);
	for (int i = 0; i < 46; ++i) {
		seed = seed * 1103515245u + 12345u;
		double x = 100 + (seed >> 8) % 824;
		seed = seed * 1103515245u + 12345u;
		double y = 470 + (seed >> 8) % 454;
		seed = seed * 1103515245u + 12345u;
		double r = 2.5 + (seed >> 8) % 5;
		CGContextFillEllipseInRect(ctx, CGRectMake(x - r, y - r, 2 * r, 2 * r));
	}
	CGColorRelease(star);

	// The bridge: seven bands on one centre below the body, red outside.
	const double bands[7][3] = {
		{0.93, 0.20, 0.22}, {0.98, 0.55, 0.15}, {0.99, 0.85, 0.20}, {0.30, 0.78, 0.35},
		{0.20, 0.60, 0.95}, {0.33, 0.33, 0.85}, {0.62, 0.35, 0.85}};
	CGPoint centre = CGPointMake(512, 20);
	double outer = 700, width = 34;
	CGContextSetLineCap(ctx, kCGLineCapButt);
	CGContextSetLineWidth(ctx, width + 1);   // the bands overlap by a pixel: no seams
	for (int i = 0; i < 7; ++i) {
		CGColorRef c = Rgb(space, bands[i][0], bands[i][1], bands[i][2]);
		CGContextSetStrokeColorWithColor(ctx, c);
		CGContextAddArc(ctx, centre.x, centre.y, outer - width * (i + 0.5), 0, M_PI, 0);
		CGContextStrokePath(ctx);
		CGColorRelease(c);
	}
	// A glow along the outside of the arc.
	CGColorRef glow = Rgb(space, 1, 1, 1, 0.18);
	CGContextSetStrokeColorWithColor(ctx, glow);
	CGContextSetLineWidth(ctx, 10);
	CGContextAddArc(ctx, centre.x, centre.y, outer + 5, 0, M_PI, 0);
	CGContextStrokePath(ctx);
	CGColorRelease(glow);

	// The ground the bridge stands on.
	const CGFloat land[] = {0.05, 0.10, 0.12, 1.0, 0.09, 0.20, 0.18, 1.0};
	CGGradientRef lg = CGGradientCreateWithColorComponents(space, land, at, 2);
	CGContextSaveGState(ctx);
	CGContextAddEllipseInRect(ctx, CGRectMake(-150, -330, 1324, 640));
	CGContextClip(ctx);
	CGContextDrawLinearGradient(ctx, lg, CGPointMake(0, 100), CGPointMake(0, 310), 0);
	CGContextRestoreGState(ctx);
	CGGradientRelease(lg);

	CGContextRestoreGState(ctx);

	// A thin rim.
	CGColorRef rim = Rgb(space, 1, 1, 1, 0.12);
	CGContextAddPath(ctx, shape);
	CGContextSetStrokeColorWithColor(ctx, rim);
	CGContextSetLineWidth(ctx, 4);
	CGContextStrokePath(ctx);
	CGColorRelease(rim);
	CGPathRelease(shape);
}

bool WritePng(const std::string &path, int size)
{
	CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef ctx = CGBitmapContextCreate(nullptr, size, size, 8, 0, space,
	                                         (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
	if (!ctx) {
		CGColorSpaceRelease(space);
		return false;
	}
	CGContextClearRect(ctx, CGRectMake(0, 0, size, size));
	CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
	Draw(ctx, space, size);
	CGImageRef image = CGBitmapContextCreateImage(ctx);
	CFStringRef p = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, p, kCFURLPOSIXPathStyle, false);
	CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
	bool ok = false;
	if (dest) {
		CGImageDestinationAddImage(dest, image, nullptr);
		ok = CGImageDestinationFinalize(dest);
		CFRelease(dest);
	}
	CFRelease(url);
	CFRelease(p);
	CGImageRelease(image);
	CGContextRelease(ctx);
	CGColorSpaceRelease(space);
	return ok;
}

}  // namespace

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <folder.iconset>\n", argv[0]);
		return 2;
	}
	std::string dir = argv[1];
	mkdir(dir.c_str(), 0755);
	// The names iconutil expects: icon_<n>x<n>.png and icon_<n>x<n>@2x.png.
	const int points[] = {16, 32, 128, 256, 512};
	for (int n : points) {
		for (int scale = 1; scale <= 2; ++scale) {
			char name[64];
			snprintf(name, sizeof name, "/icon_%dx%d%s.png", n, n, scale == 2 ? "@2x" : "");
			if (!WritePng(dir + name, n * scale)) {
				fprintf(stderr, "%s: cannot write %s%s\n", argv[0], dir.c_str(), name);
				return 1;
			}
		}
	}
	return 0;
}
