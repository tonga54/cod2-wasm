#ifndef IMPORTS_COREFOUNDATION_H
#define IMPORTS_COREFOUNDATION_H

int CFArrayGetCount();
int CFArrayGetValueAtIndex();
int CFBooleanGetValue();
int CFBundleCopyExecutableURL();
int CFBundleCopyLocalizedString();
int CFBundleGetMainBundle();
int CFDataGetBytePtr();
int CFDictionaryGetValue();
int CFGetTypeID();
int CFNumberGetValue();
int CFPreferencesAppSynchronize();
int CFPreferencesCopyAppValue();
int CFPreferencesSetAppValue();
void CFRelease(void *value);
int CFStringCompare();
int CFStringCreateWithCString();
int CFStringCreateWithCharacters();
int CFStringCreateWithFormat();
int CFStringGetCString();
int CFStringGetPascalString();
int CFStringGetTypeID();
int CFURLCreateFromFSRef();
int CFURLGetFSRef();

extern void **kCFAllocatorDefault;
extern void **kCFPreferencesCurrentApplication;

#endif
