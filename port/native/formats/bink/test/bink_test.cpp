// bifrost-bink-test: checks the Bink decoder.
//
//   bifrost-bink-test [--yuv] [--every N] [file.bik ...]
//
// Without game data it checks the prefix codes, the fast audio transforms
// against plain sums, and decodes a small movie built here bit by bit.
// With game data (BF_CLIENT_DIR, default ~/Games/<app>) it decodes
// data\video\ad\01.bik from data.grf, openning.bik from the game folder if
// there is one, every *.bik in test/bink/samples/ and the files named on the
// command line, completely. For each it reports frames, bitstream errors and
// audio, and writes every Nth frame (default 30) as PNG and each audio track
// as WAV into <game folder>/test/bink/<name>/ (game artwork: outside git;
// without a game folder, into bink-out/ beside the program).
// --yuv also writes all frames as raw yuv420p (video.yuv) for comparison
// with another decoder.
#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <string>
#include <vector>

#include <zlib.h>

#include "bf_bink.h"
#include "bf_bink_internal.h"
#include "bf_trace.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *what, const std::string &detail = std::string())
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (!ok)
		++g_failures;
}

std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char *f, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, f);
	vsnprintf(buf, sizeof buf, f, ap);
	va_end(ap);
	return buf;
}

// ---- writing files ---------------------------------------------------------

void put32be(std::vector<uint8_t> &v, uint32_t x)
{
	for (int i = 3; i >= 0; --i)
		v.push_back((uint8_t)(x >> (8 * i)));
}

void png_chunk(FILE *fp, const char *type, const std::vector<uint8_t> &data)
{
	std::vector<uint8_t> c;
	put32be(c, (uint32_t)data.size());
	c.insert(c.end(), type, type + 4);
	c.insert(c.end(), data.begin(), data.end());
	uLong crc = crc32(0, c.data() + 4, (uInt)(c.size() - 4));
	put32be(c, (uint32_t)crc);
	fwrite(c.data(), 1, c.size(), fp);
}

bool write_png(const std::string &path, const uint8_t *bgra, int w, int h)
{
	std::vector<uint8_t> raw;
	raw.reserve((size_t)(w * 3 + 1) * h);
	for (int y = 0; y < h; ++y) {
		raw.push_back(0);
		for (int x = 0; x < w; ++x) {
			const uint8_t *p = bgra + ((size_t)y * w + x) * 4;
			raw.push_back(p[2]);
			raw.push_back(p[1]);
			raw.push_back(p[0]);
		}
	}
	uLongf len = compressBound((uLong)raw.size());
	std::vector<uint8_t> z(len);
	if (compress2(z.data(), &len, raw.data(), (uLong)raw.size(), 6) != Z_OK)
		return false;
	z.resize(len);
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp)
		return false;
	static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
	fwrite(sig, 1, 8, fp);
	std::vector<uint8_t> ihdr;
	put32be(ihdr, (uint32_t)w);
	put32be(ihdr, (uint32_t)h);
	ihdr.push_back(8);   // bit depth
	ihdr.push_back(2);   // RGB
	ihdr.push_back(0);
	ihdr.push_back(0);
	ihdr.push_back(0);
	png_chunk(fp, "IHDR", ihdr);
	png_chunk(fp, "IDAT", z);
	png_chunk(fp, "IEND", {});
	return fclose(fp) == 0;
}

bool write_wav(const std::string &path, const std::vector<int16_t> &pcm, uint32_t rate, int channels)
{
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp)
		return false;
	uint32_t bytes = (uint32_t)(pcm.size() * 2);
	auto u32 = [&](uint32_t v) { fwrite(&v, 4, 1, fp); };
	auto u16 = [&](uint16_t v) { fwrite(&v, 2, 1, fp); };
	fwrite("RIFF", 1, 4, fp);
	u32(36 + bytes);
	fwrite("WAVEfmt ", 1, 8, fp);
	u32(16);
	u16(1);
	u16((uint16_t)channels);
	u32(rate);
	u32(rate * channels * 2);
	u16((uint16_t)(channels * 2));
	u16(16);
	fwrite("data", 1, 4, fp);
	u32(bytes);
	fwrite(pcm.data(), 2, pcm.size(), fp);
	return fclose(fp) == 0;
}

// ---- checks without game data -----------------------------------------------

void check_codes()
{
	bool ok = true;
	for (int t = 0; t < 16; ++t) {
		double kraft = 0;
		for (int s = 0; s < 16; ++s) {
			const char *a = bf_bink_tables::kTreeCodes[t][s];
			kraft += ldexp(1.0, -(int)strlen(a));
			for (int u = 0; u < 16; ++u)
				if (u != s && strncmp(bf_bink_tables::kTreeCodes[t][u], a, strlen(a)) == 0)
					ok = false;
		}
		if (fabs(kraft - 1.0) > 1e-12)
			ok = false;
	}
	check(ok, "the 16 prefix codes are complete and prefix-free");
	bool scan = true;
	bool seen[64] = {};
	for (int i = 0; i < 64; ++i)
		seen[bf_bink_tables::kScan[i]] = true;
	for (bool s : seen)
		scan = scan && s;
	for (int p = 0; p < 16; ++p) {
		bool used[64] = {};
		for (int i = 0; i < 64; ++i)
			used[bf_bink_tables::kRunPatterns[p][i]] = true;
		for (bool s : used)
			scan = scan && s;
	}
	check(scan, "the scan order and the 16 run patterns are permutations of 0..63");
}

void check_transforms()
{
	for (int dct = 0; dct < 2; ++dct) {
		for (int bits = 9; bits <= 11; ++bits) {
			int n = 1 << bits;
			std::vector<float> c(n + 2), ref(n), fast(n);
			uint32_t seed = 12345u + bits + dct;
			for (int i = 0; i < n; ++i) {
				seed = seed * 1103515245u + 12345u;
				c[i] = ((seed >> 8) & 0xFFFF) / 65536.0f - 0.5f;
			}
			if (dct)
				bf_bink_impl::reference_dct_inverse(c.data(), ref.data(), n);
			else
				bf_bink_impl::reference_rdft_inverse(c.data(), ref.data(), n);
			bool ran = bf_bink_impl::fast_transform_check(dct != 0, n, c.data(), fast.data());
			double err = 0, mag = 0;
			for (int i = 0; i < n; ++i) {
				err = fmax(err, fabs(ref[i] - fast[i]));
				mag = fmax(mag, fabs(ref[i]));
			}
			check(ran && err <= 1e-4 * fmax(mag, 1e-6),
			      fmt("%s transform, length %d, matches the plain sum", dct ? "DCT" : "RDFT", n).c_str(),
			      fmt("max error %.3g of %.3g", err, mag));
		}
	}
}

class BitWriter {
public:
	void put(uint32_t v, int n) {
		for (int i = 0; i < n; ++i) {
			if ((bits_ & 7) == 0)
				bytes.push_back(0);
			if ((v >> i) & 1)
				bytes.back() |= (uint8_t)(1 << (bits_ & 7));
			++bits_;
		}
	}
	void align32() {
		while (bits_ & 31)
			put(0, 1);
	}
	std::vector<uint8_t> bytes;

private:
	size_t bits_ = 0;
};

int log2_floor(unsigned v)
{
	int n = -1;
	while (v) {
		v >>= 1;
		++n;
	}
	return n;
}

// One plane of fill blocks of one colour, every bundle with the identity
// code (4 raw bits per symbol).
void write_fill_plane(BitWriter &bw, int width_for_counts, int blocks_w, int blocks_h, int colour)
{
	int w = ((width_for_counts < 8 ? 8 : width_for_counts) + 7) & ~7;
	int len_types = log2_floor((w >> 3) + 511) + 1;
	int len_sub = log2_floor((w >> 4) + 511) + 1;
	int len_colors = log2_floor(blocks_w * 64 + 511) + 1;
	int len_pattern = log2_floor((blocks_w << 3) + 511) + 1;
	int len_motion = len_types;
	int len_run = log2_floor(blocks_w * 48 + 511) + 1;
	for (int n = 0; n < 9; ++n) {
		if (n == 2)
			for (int t = 0; t < 16; ++t)
				bw.put(0, 4);
		if (n != 6 && n != 7)
			bw.put(0, 4);
	}
	int blocks = blocks_w * blocks_h;
	bw.put((uint32_t)blocks, len_types);   // block types: all 6 (fill)
	bw.put(1, 1);
	bw.put(6, 4);
	bw.put(0, len_sub);                    // no 16x16 sub types
	bw.put((uint32_t)blocks, len_colors);  // colours: all the same
	bw.put(1, 1);
	bw.put((uint32_t)(colour >> 4), 4);
	bw.put((uint32_t)(colour & 15), 4);
	bw.put(0, len_pattern);
	bw.put(0, len_motion);
	bw.put(0, len_motion);
	bw.put(0, len_motion);                 // intra DC
	bw.put(0, len_motion);                 // inter DC
	bw.put(0, len_run);
	bw.align32();
}

void check_synthetic()
{
	const int W = 16, H = 16;
	BitWriter v;
	v.put(0, 32);                          // colour planes' size (unused)
	write_fill_plane(v, W, 2, 2, 100);     // Y
	write_fill_plane(v, W >> 1, 1, 1, 160);// coded second: Cr
	write_fill_plane(v, W >> 1, 1, 1, 90); // coded third: Cb
	std::vector<uint8_t> f(44 + 8);
	auto w32 = [&](size_t at, uint32_t x) { memcpy(&f[at], &x, 4); };
	memcpy(&f[0], "BIKi", 4);
	w32(8, 1);
	w32(12, (uint32_t)v.bytes.size());
	w32(16, 1);
	w32(20, W);
	w32(24, H);
	w32(28, 30);
	w32(32, 1);
	w32(36, 0);
	w32(40, 0);
	w32(44, 52 | 1);
	w32(48, (uint32_t)(52 + v.bytes.size()));
	f.insert(f.end(), v.bytes.begin(), v.bytes.end());
	w32(4, (uint32_t)(f.size() - 8));

	bf_bink *b = bf_bink_open_memory(f.data(), f.size());
	check(b != nullptr, "a synthetic 16x16 movie opens");
	if (!b)
		return;
	bf_bink_info info;
	bf_bink_get_info(b, &info);
	check(info.width == 16 && info.height == 16 && info.frames == 1 && info.revision == 'i',
	      "its header reads back", fmt("%ux%u, %u frames, rev %c", info.width, info.height, info.frames, info.revision));
	int r = bf_bink_decode_next(b);
	check(r == 1, "its frame decodes", fmt("result %d", r));
	int pitch, pw, ph;
	const uint8_t *y = bf_bink_plane(b, 0, &pitch, &pw, &ph);
	const uint8_t *u = bf_bink_plane(b, 1, nullptr, nullptr, nullptr);
	const uint8_t *vv = bf_bink_plane(b, 2, nullptr, nullptr, nullptr);
	check(y && u && vv && y[0] == 100 && y[15 * pitch + 15] == 100 && u[0] == 90 && vv[0] == 160,
	      "fill blocks give the coded colours (Y 100, Cb 90, Cr 160)",
	      y && u && vv ? fmt("Y %d %d, Cb %d, Cr %d", y[0], y[15 * pitch + 15], u[0], vv[0]) : "no planes");
	uint8_t px[16 * 16 * 4];
	bf_bink_copy_pixels(b, px, 16 * 4, 16, 16, 0, 0, BF_BINK_BGRA8888);
	// BT.601 of (100, 90, 160): R 149, G 87, B 21.
	check(abs(px[2] - 149) <= 1 && abs(px[1] - 87) <= 1 && abs(px[0] - 21) <= 1 && px[3] == 255,
	      "conversion to BGRA", fmt("R %d G %d B %d A %d", px[2], px[1], px[0], px[3]));
	check(bf_bink_decode_next(b) == 0, "the end of the movie is reported");
	check(bf_bink_seek(b, 0) == 0 && bf_bink_decode_next(b) == 1, "seek back to the start");
	bf_bink_close(b);

	static const uint8_t junk[64] = { 'K', 'B', '2', 'j' };
	check(bf_bink_open_memory(junk, sizeof junk) == nullptr, "Bink 2 is refused");
	check(bf_bink_open_memory("BIKi", 4) == nullptr, "a truncated header is refused");
}

// ---- game data ---------------------------------------------------------------

bool read_file(const std::string &path, std::vector<uint8_t> &out)
{
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	long n = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	out.resize(n > 0 ? (size_t)n : 0);
	bool ok = n > 0 && fread(out.data(), 1, (size_t)n, fp) == (size_t)n;
	fclose(fp);
	return ok;
}

// Reads one file from a GRF archive (version 0x200, not encrypted).
bool grf_extract(const std::string &grf, const char *name, std::vector<uint8_t> &out)
{
	FILE *fp = fopen(grf.c_str(), "rb");
	if (!fp)
		return false;
	uint8_t head[46];
	bool ok = false;
	if (fread(head, 1, 46, fp) == 46 && memcmp(head, "Master of Magic", 15) == 0) {
		uint32_t table, version;
		memcpy(&table, head + 30, 4);
		memcpy(&version, head + 42, 4);
		uint32_t sizes[2];
		if ((version >> 8) == 2 && fseeko(fp, 46 + (off_t)table, SEEK_SET) == 0 && fread(sizes, 4, 2, fp) == 2) {
			std::vector<uint8_t> packed(sizes[0]), raw(sizes[1]);
			uLongf rawlen = sizes[1];
			if (fread(packed.data(), 1, packed.size(), fp) == packed.size() &&
			    uncompress(raw.data(), &rawlen, packed.data(), sizes[0]) == Z_OK) {
				size_t at = 0;
				while (at < rawlen) {
					const char *entry = (const char *)raw.data() + at;
					size_t len = strnlen(entry, rawlen - at);
					if (at + len + 18 > rawlen)
						break;
					const uint8_t *e = raw.data() + at + len + 1;
					if (strcasecmp(entry, name) == 0) {
						uint32_t pk, aligned, size, offset;
						memcpy(&pk, e, 4);
						memcpy(&aligned, e + 4, 4);
						memcpy(&size, e + 8, 4);
						memcpy(&offset, e + 13, 4);
						if ((e[12] & 1) && !(e[12] & 6)) {
							std::vector<uint8_t> z(aligned);
							out.resize(size);
							uLongf outlen = size;
							ok = fseeko(fp, 46 + (off_t)offset, SEEK_SET) == 0 &&
							     fread(z.data(), 1, aligned, fp) == aligned &&
							     uncompress(out.data(), &outlen, z.data(), pk) == Z_OK && outlen == size;
						}
						break;
					}
					at += len + 1 + 17;
				}
			}
		}
	}
	fclose(fp);
	return ok;
}

struct Options {
	int every = 30;
	bool yuv = false;
};

void decode_movie(const std::string &label, const std::vector<uint8_t> &data, const std::string &outdir,
                  const Options &opt)
{
	printf("-- %s (%zu bytes)\n", label.c_str(), data.size());
	bf_bink *b = bf_bink_open_memory(data.data(), data.size());
	if (!b) {
		check(false, fmt("%s opens", label.c_str()).c_str(),
		      data.size() >= 4 ? fmt("signature %.4s", (const char *)data.data()) : "too short");
		return;
	}
	bf_bink_info in;
	bf_bink_get_info(b, &in);
	double fps = (double)in.rate_num / in.rate_den;
	printf("     BIK%c, %ux%u, %u frames at %.3f fps (%.3f s), flags 0x%08x, %d audio track(s)\n",
	       in.revision, in.width, in.height, in.frames, fps, in.frames / fps, in.video_flags, in.audio_tracks);
	mkdir(outdir.c_str(), 0755);

	std::vector<std::vector<int16_t>> pcm(in.audio_tracks);
	std::vector<bf_bink_track_info> tracks(in.audio_tracks);
	for (int t = 0; t < in.audio_tracks; ++t) {
		bf_bink_get_track(b, t, &tracks[t]);
		printf("     track %d: id %u, %u Hz, %d channel(s), %s\n", t, tracks[t].id, tracks[t].sample_rate,
		       tracks[t].channels, tracks[t].uses_dct ? "DCT" : "RDFT");
	}
	FILE *yuv = opt.yuv ? fopen((outdir + "/video.yuv").c_str(), "wb") : nullptr;
	std::vector<uint8_t> bgra((size_t)in.width * in.height * 4);
	int pngs = 0, first_damaged = -1;
	for (;;) {
		int r = bf_bink_decode_next(b);
		if (r == 0)
			break;
		int frame = bf_bink_frame(b);
		if (r < 0 && first_damaged < 0)
			first_damaged = frame;
		for (int t = 0; t < in.audio_tracks; ++t) {
			size_t n = bf_bink_audio_available(b, t);
			size_t at = pcm[t].size();
			pcm[t].resize(at + n * tracks[t].channels);
			bf_bink_audio_read(b, t, pcm[t].data() + at, n);
		}
		if (yuv) {
			for (int p = 0; p < 3; ++p) {
				int pitch, w, h;
				const uint8_t *pl = bf_bink_plane(b, p, &pitch, &w, &h);
				for (int y = 0; y < h; ++y)
					fwrite(pl + (size_t)y * pitch, 1, (size_t)w, yuv);
			}
		}
		if (opt.every > 0 && frame % opt.every == 0) {
			bf_bink_copy_pixels(b, bgra.data(), (int)in.width * 4, (int)in.width, (int)in.height, 0, 0,
			                    BF_BINK_BGRA8888);
			if (write_png(outdir + fmt("/frame_%04d.png", frame), bgra.data(), (int)in.width, (int)in.height))
				++pngs;
		}
	}
	if (yuv)
		fclose(yuv);
	bf_bink_stats st;
	bf_bink_get_stats(b, &st);
	check(st.frames_decoded == in.frames, fmt("%s: all frames decoded", label.c_str()).c_str(),
	      fmt("%u of %u, %d PNG written", st.frames_decoded, in.frames, pngs));
	check(st.video_errors == 0, fmt("%s: no video bitstream errors", label.c_str()).c_str(),
	      st.video_errors ? fmt("%u damaged frames, first %d", st.video_errors, first_damaged) : "");
	for (int t = 0; t < in.audio_tracks; ++t) {
		size_t frames = pcm[t].size() / tracks[t].channels;
		double secs = (double)frames / tracks[t].sample_rate;
		double movie = in.frames / fps;
		write_wav(outdir + fmt("/audio_%d.wav", t), pcm[t], tracks[t].sample_rate, tracks[t].channels);
		double peak = 0, sum = 0;
		for (int16_t s : pcm[t]) {
			peak = fmax(peak, fabs((double)s));
			sum += (double)s * s;
		}
		double rms = pcm[t].empty() ? 0 : sqrt(sum / pcm[t].size());
		// The sound may run on for up to a second after the last picture.
		check(secs >= movie - 0.1 && secs <= movie + 1.0,
		      fmt("%s: track %d length matches the movie", label.c_str(), t).c_str(),
		      fmt("%zu samples = %.3f s for %.3f s of video, peak %.0f, rms %.0f", frames, secs, movie, peak, rms));
	}
	check(st.audio_errors == 0, fmt("%s: no audio bitstream errors", label.c_str()).c_str(),
	      fmt("%u packets, %u damaged", st.audio_packets, st.audio_errors));
	printf("     output in %s\n", outdir.c_str());
	bf_bink_close(b);
}

std::string stem(const std::string &path)
{
	size_t s = path.find_last_of('/');
	std::string n = s == std::string::npos ? path : path.substr(s + 1);
	size_t d = n.find_last_of('.');
	return d == std::string::npos ? n : n.substr(0, d);
}

} // namespace

int main(int argc, char **argv)
{
	bf_trace_init();
	Options opt;
	std::vector<std::string> files;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--yuv"))
			opt.yuv = true;
		else if (!strcmp(argv[i], "--every") && i + 1 < argc)
			opt.every = atoi(argv[++i]);
		else
			files.push_back(argv[i]);
	}

	check_codes();
	check_transforms();
	check_synthetic();

	std::string dir;
	if (const char *env = getenv("BF_CLIENT_DIR"))
		dir = env;
	else if (const char *home = getenv("HOME"))
		dir = std::string(home) + "/Games/" BF_APP_NAME;
	struct stat st;
	bool have = !dir.empty() && stat((dir + "/data.grf").c_str(), &st) == 0;
	std::string out = dir + "/test/bink";
	if (have) {
		mkdir((dir + "/test").c_str(), 0755);
		mkdir(out.c_str(), 0755);
		std::vector<uint8_t> data;
		if (grf_extract(dir + "/data.grf", "data\\video\\ad\\01.bik", data))
			decode_movie("data.grf:data\\video\\ad\\01.bik", data, out + "/01", opt);
		else
			check(false, "data\\video\\ad\\01.bik read from data.grf");
		if (read_file(dir + "/openning.bik", data))
			decode_movie("openning.bik", data, out + "/openning", opt);
		else
			printf("skipped openning.bik: not in %s\n", dir.c_str());
		if (DIR *d = opendir((out + "/samples").c_str())) {
			std::vector<std::string> names;
			while (dirent *e = readdir(d)) {
				std::string n = e->d_name;
				if (n.size() > 4 && strcasecmp(n.c_str() + n.size() - 4, ".bik") == 0)
					names.push_back(out + "/samples/" + n);
			}
			closedir(d);
			std::sort(names.begin(), names.end());
			files.insert(files.begin(), names.begin(), names.end());
		}
	} else {
		printf("skipped game data: no data.grf in %s\n", dir.empty() ? "(no folder)" : dir.c_str());
	}
	for (const std::string &f : files) {
		std::vector<uint8_t> data;
		if (!read_file(f, data)) {
			check(false, fmt("%s read", f.c_str()).c_str());
			continue;
		}
		// Without a game folder, beside the test program.
		std::string exe = argv[0];
		std::string here = exe.find('/') == std::string::npos ? "." : exe.substr(0, exe.find_last_of('/'));
		std::string o = have ? out + "/" + stem(f) : here + "/bink-out";
		if (!have) {
			mkdir(o.c_str(), 0755);
			o += "/" + stem(f);
		}
		decode_movie(f, data, o, opt);
	}
	printf("%s: %d failure(s)\n", g_failures ? "FAILED" : "OK", g_failures);
	return g_failures ? 1 : 0;
}
