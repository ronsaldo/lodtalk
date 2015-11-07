#ifndef LODTALK_DEFINITIONS_H_
#define LODTALK_DEFINITIONS_H_

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#define LODTALK_EXPORT_SYMBOL __declspec(dllexport)
#define LODTALK_IMPORT_SYMBOL __declspec(dllexport)
#else
#define LODTALK_EXPORT_SYMBOL __attribute__ ((visibility ("default")))
#define LODTALK_IMPORT_SYMBOL __attribute__ ((visibility ("default")))
#endif

#ifdef BUILDING_LODTALK_VM
#define LODTALK_VM_EXPORT LODTALK_EXPORT_SYMBOL
#else
#define LODTALK_VM_EXPORT LODTALK_IMPORT_SYMBOL
#endif

#ifdef __cplusplus
#define LODTALK_EXTERN_C extern "C"
#else
#define LODTALK_EXTERN_C
#endif

#ifdef _WIN32
#pragma warning(disable : 4200)
#endif

#if defined(__GNUC__)
#define LODTALK_UNIMPLEMENTED() { \
	fprintf(stderr, "The method %s is unimplemented in %s at line %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
	abort(); }
#else
#define LODTALK_UNIMPLEMENTED() { \
	fprintf(stderr, "The method %s is unimplemented in %s at line %d\n", __func__, __FILE__, __LINE__); \
	abort(); }
#endif

#endif //LODTALK_DEFINITIONS_H_
