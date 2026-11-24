// The settings object: built-in defaults under the values stored in one property list.

#include "bf_settings.h"
#include "bf_file.h"
#include "bf_trace.h"

#import <Foundation/Foundation.h>

#include <string.h>

#include <map>
#include <mutex>
#include <string>

struct bf_settings {
	std::mutex lock;
	std::string path;
	NSMutableDictionary *values;   // what was set; written to the file
	bool changed;
	bool shared;
};

namespace {

const int kFormatVersion = 1;

NSString *Utf8(const char *s)
{
	if (!s)
		return nil;
	return [[NSString alloc] initWithBytes:s length:strlen(s) encoding:NSUTF8StringEncoding];
}

NSData *ZeroGuid()
{
	static const unsigned char zero[16] = { 0 };
	return [NSData dataWithBytes:zero length:sizeof(zero)];
}

// What Setup_HP.exe wrote, and the port's options. Never stored.
NSDictionary *Defaults()
{
	static NSDictionary *defaults;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		defaults = @{
			@BF_SETTINGS_OPTIONS: @{
				@BF_OPT_SKIP_INTRO: @1,
				@BF_OPT_FREE_CAMERA: @1,
				@BF_OPT_LAUNCH_ARGUMENTS: @1,
				@BF_OPT_ATTACK_WITHOUT_CTRL: @0,
			},
			@BF_SETTINGS_ACCOUNT: @{
				@BF_SETTING_ACCOUNT_NAME: @"",
			},
			@BF_SETTINGS_PATHS: @{},
			// HKLM\<g_regPath>: read by ReadRegistry (WinMain.cpp) and the login window.
			@BF_SETTINGS_CLIENT: @{
				@"ISFULLSCREENMODE": @0,
				@"WIDTH": @1024,
				@"HEIGHT": @768,
				@"BITPERPIXEL": @32,
				@"DEVICECNT": @0,
				@"MODECNT": @0,
				@"ISVOODOO": @0,
				@"SHOWTIPSATSTARTUP": @0,
				@"ISLIGHTMAP": @1,
				@"TRILINEARFILTER": @0,
				@"SPRITEMODE": @2,         // full sprites
				@"TEXTUREMODE": @2,        // full textures
				@"FOG": @1,
				@"DEVICENAME": @"",
				@"GUIDDEVICE": ZeroGuid(),
				@"GUIDDRIVER": ZeroGuid(),
				@"SOUNDMODE": @1,          // 2D sound
				@"SPEAKERTYPE": @0,
				@"DIGITALRATETYPE": @0,    // 22050 Hz
				@"DIGITALBITSTYPE": @0,    // 16 bits
				@"NUMSAMPLETYPE": @1,      // 32 voices
				@"PROVIDERNAME": @"Miles Fast 2D Positional Audio",
				@"CHECKSAVE": @0,
				// HKLM\<g_regPath>\Option: read by CSession::InitOptionWithRegistry.
				@"Option": @{
					@"isEffectOn": @1,
					@"bgmIsPaused": @0,
					@"isSoundOn": @1,
					@"streamVolume": @100,
					@"soundVolume": @100,
				},
			},
		};
	});
	return defaults;
}

// The key of dict equal to name without regard to case, or nil.
NSString *KeyIn(NSDictionary *dict, NSString *name)
{
	if (!dict || !name)
		return nil;
	if (dict[name])
		return name;
	for (NSString *key in dict)
		if ([key caseInsensitiveCompare:name] == NSOrderedSame)
			return key;
	return nil;
}

bool SplitGroup(const char *group, NSMutableArray<NSString *> *parts)
{
	if (!group)
		return true;
	NSString *s = Utf8(group);
	if (!s) {
		bf_tracef(BFT_ERR, "settings: group name is not UTF-8");
		return false;
	}
	for (NSString *part in [s componentsSeparatedByCharactersInSet:
	                         [NSCharacterSet characterSetWithCharactersInString:@"/\\"]])
		if (part.length)
			[parts addObject:part];
	return true;
}

// The group dictionary at parts below root, or nil.
NSDictionary *Find(NSDictionary *root, NSArray<NSString *> *parts)
{
	NSDictionary *current = root;
	for (NSString *part in parts) {
		NSString *key = KeyIn(current, part);
		id child = key ? current[key] : nil;
		if (![child isKindOfClass:[NSDictionary class]])
			return nil;
		current = child;
	}
	return current;
}

// The stored group, created on the way. nil if a value is in the way.
NSMutableDictionary *MakeGroup(NSMutableDictionary *root, NSArray<NSString *> *parts)
{
	NSMutableDictionary *current = root;
	for (NSString *part in parts) {
		NSString *key = KeyIn(current, part);
		id child = key ? current[key] : nil;
		if (!child) {
			child = [NSMutableDictionary dictionary];
			current[part] = child;
		} else if (![child isKindOfClass:[NSMutableDictionary class]]) {
			if ([child isKindOfClass:[NSDictionary class]]) {
				child = [child mutableCopy];
				current[key] = child;
			} else {
				return nil;
			}
		}
		current = child;
	}
	return current;
}

// A value: stored first, then default. Must be called with the lock held.
id Lookup(bf_settings *s, const char *group, const char *name)
{
	NSMutableArray<NSString *> *parts = [NSMutableArray array];
	NSString *valueName = Utf8(name);
	if (!valueName || !SplitGroup(group, parts))
		return nil;
	NSDictionary *stored = Find(s->values, parts);
	NSString *key = KeyIn(stored, valueName);
	id value = key ? stored[key] : nil;
	if (value && ![value isKindOfClass:[NSDictionary class]])
		return value;
	NSDictionary *defaults = Find(Defaults(), parts);
	key = KeyIn(defaults, valueName);
	value = key ? defaults[key] : nil;
	if (value && ![value isKindOfClass:[NSDictionary class]])
		return value;
	return nil;
}

bf_setting_type TypeOf(id value)
{
	if ([value isKindOfClass:[NSNumber class]]) return BF_SETTING_INT;
	if ([value isKindOfClass:[NSString class]]) return BF_SETTING_STRING;
	if ([value isKindOfClass:[NSData class]])   return BF_SETTING_DATA;
	return BF_SETTING_NONE;
}

bool Store(bf_settings *s, const char *group, const char *name, id value)
{
	NSMutableArray<NSString *> *parts = [NSMutableArray array];
	NSString *valueName = Utf8(name);
	if (!valueName || !value || !SplitGroup(group, parts)) {
		bf_tracef(BFT_ERR, "settings: cannot set %s/%s: name or value not UTF-8",
		          group ? group : "", name ? name : "(null)");
		return false;
	}
	std::lock_guard<std::mutex> guard(s->lock);
	NSMutableDictionary *dict = MakeGroup(s->values, parts);
	if (!dict) {
		bf_tracef(BFT_ERR, "settings: cannot set %s/%s: a value is in the way", group, name);
		return false;
	}
	NSString *key = KeyIn(dict, valueName);
	if (key && [dict[key] isKindOfClass:[NSDictionary class]]) {
		bf_tracef(BFT_ERR, "settings: cannot set %s/%s: a group of that name exists", group, name);
		return false;
	}
	if (!key) {
		// keep the spelling of the default, if there is one
		NSString *defaultKey = KeyIn(Find(Defaults(), parts), valueName);
		key = defaultKey ? defaultKey : valueName;
	}
	if (![dict[key] isEqual:value]) {
		dict[key] = value;
		s->changed = true;
	}
	return true;
}

int CopyOut(const void *bytes, size_t length, bool terminate, void *out, size_t outSize, size_t *lengthOut)
{
	if (lengthOut)
		*lengthOut = length;
	size_t need = length + (terminate ? 1 : 0);
	if (!out)
		return terminate ? -1 : 1;
	if (outSize < need)
		return -1;
	memcpy(out, bytes, length);
	if (terminate)
		((char *)out)[length] = 0;
	return 1;
}

int LoadLocked(bf_settings *s)
{
	s->changed = false;
	NSData *data = [NSData dataWithContentsOfFile:@(s->path.c_str())];
	if (!data) {
		s->values = [NSMutableDictionary dictionary];
		return 0;
	}
	NSError *error = nil;
	id plist = [NSPropertyListSerialization propertyListWithData:data
	                                                     options:NSPropertyListMutableContainers
	                                                      format:NULL
	                                                       error:&error];
	if (![plist isKindOfClass:[NSMutableDictionary class]]) {
		bf_tracef(BFT_ERR, "settings: %s unreadable (%s); starting empty", s->path.c_str(),
		          error ? error.localizedDescription.UTF8String : "not a dictionary");
		s->values = [NSMutableDictionary dictionary];
		return -1;
	}
	s->values = plist;
	[s->values removeObjectForKey:@"FormatVersion"];
	BF_TRACE(BFT_FILE, "settings: read %s", s->path.c_str());
	return 1;
}

bf_settings *Create(const std::string &path, bool shared)
{
	bf_settings *s = new bf_settings;
	s->path = path;
	s->shared = shared;
	s->changed = false;
	std::lock_guard<std::mutex> guard(s->lock);
	LoadLocked(s);
	return s;
}

std::string SupportFile(const char *profile)
{
	const char *support = bf_file_location(BF_LOCATION_SUPPORT);
	std::string folder = support ? support : bf_file_home();
	if (!profile || !*profile)
		return folder + "/Settings.plist";
	return folder + "/Settings-" + profile + ".plist";
}

std::mutex g_profilesLock;
std::map<std::string, bf_settings *> g_profiles;

} // namespace

extern "C" bf_settings *bf_settings_shared(void)
{
	static bf_settings *shared = Create(SupportFile(NULL), true);
	return shared;
}

extern "C" bf_settings *bf_settings_open(const char *profile)
{
	if (!profile || !*profile)
		return bf_settings_shared();
	std::string path = SupportFile(profile);   // outside the lock: it may create the folder
	std::lock_guard<std::mutex> guard(g_profilesLock);
	bf_settings *&s = g_profiles[profile];
	if (!s)
		s = Create(path, true);
	return s;
}

extern "C" bf_settings *bf_settings_open_file(const char *plist_path)
{
	if (!plist_path)
		return NULL;
	return Create(plist_path, false);
}

extern "C" void bf_settings_close(bf_settings *settings)
{
	if (settings && !settings->shared)
		delete settings;
}

extern "C" const char *bf_settings_path(bf_settings *settings)
{
	return settings->path.c_str();
}

extern "C" int bf_settings_load(bf_settings *settings)
{
	std::lock_guard<std::mutex> guard(settings->lock);
	return LoadLocked(settings);
}

extern "C" int bf_settings_save(bf_settings *settings)
{
	std::lock_guard<std::mutex> guard(settings->lock);
	if (!settings->changed)
		return 0;
	@autoreleasepool {
		NSMutableDictionary *root = [settings->values mutableCopy];
		root[@"FormatVersion"] = @(kFormatVersion);
		NSError *error = nil;
		NSData *data = [NSPropertyListSerialization dataWithPropertyList:root
		                                                          format:NSPropertyListXMLFormat_v1_0
		                                                         options:0
		                                                           error:&error];
		if (!data) {
			bf_tracef(BFT_ERR, "settings: cannot encode: %s", error.localizedDescription.UTF8String);
			return -1;
		}
		NSString *path = @(settings->path.c_str());
		[[NSFileManager defaultManager] createDirectoryAtPath:[path stringByDeletingLastPathComponent]
		                          withIntermediateDirectories:YES
		                                           attributes:nil
		                                                error:NULL];
		if (![data writeToFile:path options:NSDataWritingAtomic error:&error]) {
			bf_tracef(BFT_ERR, "settings: cannot write %s: %s", settings->path.c_str(),
			          error.localizedDescription.UTF8String);
			return -1;
		}
	}
	settings->changed = false;
	BF_TRACE(BFT_FILE, "settings: wrote %s", settings->path.c_str());
	return 1;
}

extern "C" void bf_settings_reset(bf_settings *settings)
{
	std::lock_guard<std::mutex> guard(settings->lock);
	if (settings->values.count)
		settings->changed = true;
	settings->values = [NSMutableDictionary dictionary];
}

extern "C" int bf_settings_has_group(bf_settings *settings, const char *group)
{
	@autoreleasepool {
		NSMutableArray<NSString *> *parts = [NSMutableArray array];
		if (!SplitGroup(group, parts))
			return 0;
		std::lock_guard<std::mutex> guard(settings->lock);
		return (Find(settings->values, parts) || Find(Defaults(), parts)) ? 1 : 0;
	}
}

extern "C" int bf_settings_create_group(bf_settings *settings, const char *group)
{
	@autoreleasepool {
		NSMutableArray<NSString *> *parts = [NSMutableArray array];
		if (!SplitGroup(group, parts))
			return 0;
		std::lock_guard<std::mutex> guard(settings->lock);
		if (Find(settings->values, parts))
			return 1;
		if (!MakeGroup(settings->values, parts))
			return 0;
		settings->changed = true;
		return 1;
	}
}

extern "C" int bf_settings_remove(bf_settings *settings, const char *group, const char *name)
{
	@autoreleasepool {
		NSMutableArray<NSString *> *parts = [NSMutableArray array];
		if (!SplitGroup(group, parts))
			return 0;
		std::lock_guard<std::mutex> guard(settings->lock);
		NSString *target;
		if (!name) {
			if (!parts.count)
				return 0;
			target = parts.lastObject;
			[parts removeLastObject];
		} else {
			target = Utf8(name);
		}
		NSMutableDictionary *parent = (NSMutableDictionary *)Find(settings->values, parts);
		NSString *key = KeyIn(parent, target);
		if (!key)
			return 0;
		bool isGroup = [parent[key] isKindOfClass:[NSDictionary class]];
		if (isGroup != (name == NULL))
			return 0;
		parent = MakeGroup(settings->values, parts);   // make sure it is mutable
		[parent removeObjectForKey:key];
		settings->changed = true;
		return 1;
	}
}

extern "C" bf_setting_type bf_settings_value_at(bf_settings *settings, const char *group, int index,
                                                char *name, size_t name_size)
{
	@autoreleasepool {
		NSMutableArray<NSString *> *parts = [NSMutableArray array];
		if (index < 0 || !SplitGroup(group, parts))
			return BF_SETTING_NONE;
		std::lock_guard<std::mutex> guard(settings->lock);
		NSDictionary *stored = Find(settings->values, parts);
		NSDictionary *defaults = Find(Defaults(), parts);
		NSMutableArray<NSString *> *names = [NSMutableArray array];
		NSMutableArray *values = [NSMutableArray array];
		for (NSString *key in [stored.allKeys sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)]) {
			if ([stored[key] isKindOfClass:[NSDictionary class]])
				continue;
			[names addObject:key];
			[values addObject:stored[key]];
		}
		for (NSString *key in [defaults.allKeys sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)]) {
			if ([defaults[key] isKindOfClass:[NSDictionary class]] || KeyIn(stored, key))
				continue;
			[names addObject:key];
			[values addObject:defaults[key]];
		}
		if ((NSUInteger)index >= names.count)
			return BF_SETTING_NONE;
		const char *utf8 = names[(NSUInteger)index].UTF8String;
		if (strlen(utf8) + 1 > name_size)
			return BF_SETTING_NONE;
		memcpy(name, utf8, strlen(utf8) + 1);
		return TypeOf(values[(NSUInteger)index]);
	}
}

extern "C" bf_setting_type bf_settings_type(bf_settings *settings, const char *group, const char *name)
{
	@autoreleasepool {
		std::lock_guard<std::mutex> guard(settings->lock);
		return TypeOf(Lookup(settings, group, name));
	}
}

extern "C" int bf_settings_get_int(bf_settings *settings, const char *group, const char *name, int32_t *out)
{
	@autoreleasepool {
		std::lock_guard<std::mutex> guard(settings->lock);
		id value = Lookup(settings, group, name);
		if (![value isKindOfClass:[NSNumber class]])
			return 0;
		if (out)
			*out = (int32_t)(uint32_t)[(NSNumber *)value longLongValue];
		return 1;
	}
}

extern "C" int32_t bf_settings_int(bf_settings *settings, const char *group, const char *name, int32_t fallback)
{
	int32_t value = fallback;
	bf_settings_get_int(settings, group, name, &value);
	return value;
}

extern "C" int bf_settings_get_string(bf_settings *settings, const char *group, const char *name,
                                      char *out, size_t out_size, size_t *length)
{
	@autoreleasepool {
		std::lock_guard<std::mutex> guard(settings->lock);
		id value = Lookup(settings, group, name);
		if (![value isKindOfClass:[NSString class]])
			return 0;
		const char *utf8 = [(NSString *)value UTF8String];
		return CopyOut(utf8, strlen(utf8), true, out, out_size, length);
	}
}

extern "C" int bf_settings_get_data(bf_settings *settings, const char *group, const char *name,
                                    void *out, size_t out_size, size_t *length)
{
	@autoreleasepool {
		std::lock_guard<std::mutex> guard(settings->lock);
		id value = Lookup(settings, group, name);
		if (![value isKindOfClass:[NSData class]])
			return 0;
		NSData *data = value;
		return CopyOut(data.bytes, data.length, false, out, out_size, length);
	}
}

extern "C" int bf_settings_set_int(bf_settings *settings, const char *group, const char *name, int32_t value)
{
	@autoreleasepool {
		return Store(settings, group, name, @(value)) ? 1 : 0;
	}
}

extern "C" int bf_settings_set_string(bf_settings *settings, const char *group, const char *name, const char *utf8)
{
	@autoreleasepool {
		NSString *value = Utf8(utf8);
		if (!value) {
			bf_tracef(BFT_ERR, "settings: %s/%s: string is not UTF-8", group ? group : "", name ? name : "");
			return 0;
		}
		return Store(settings, group, name, value) ? 1 : 0;
	}
}

extern "C" int bf_settings_set_data(bf_settings *settings, const char *group, const char *name,
                                    const void *data, size_t size)
{
	@autoreleasepool {
		return Store(settings, group, name, [NSData dataWithBytes:data length:size]) ? 1 : 0;
	}
}

extern "C" int bf_settings_option(bf_settings *settings, const char *option)
{
	return bf_settings_int(settings, BF_SETTINGS_OPTIONS, option, 0) ? 1 : 0;
}

extern "C" int bf_settings_set_option(bf_settings *settings, const char *option, int on)
{
	return bf_settings_set_int(settings, BF_SETTINGS_OPTIONS, option, on ? 1 : 0);
}

extern "C" int bf_settings_account_name(bf_settings *settings, char *out, size_t out_size)
{
	return bf_settings_get_string(settings, BF_SETTINGS_ACCOUNT, BF_SETTING_ACCOUNT_NAME, out, out_size, NULL);
}

extern "C" int bf_settings_set_account_name(bf_settings *settings, const char *utf8)
{
	return bf_settings_set_string(settings, BF_SETTINGS_ACCOUNT, BF_SETTING_ACCOUNT_NAME, utf8);
}
