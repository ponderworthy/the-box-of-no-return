/*
    Copyright (C) 2010 Christian Schoenebeck
*/

#ifndef LS_DYNAMIC_LIBRARIES_H
#define LS_DYNAMIC_LIBRARIES_H

#include "Exception.h"
#include <string>

namespace LinuxSampler {

/**
 * Function signature for callback functions used with function
 * DynamicLibrariesSearch() .
 *
 * @param filename - full qualified filename of DLL
 * @param hDLL - handle of opened DLL
 * @param pFunction - pointer to requested function of DLL
 * @param pCustom - (optional) custom data
 */
typedef void DynamicLibrariesSearchCallbackFunction(
    String filename, void* hDLL, void* pFunction, void* pCustom
);

/**
 * Search and load DLLs in the directory given by @a dir . For each DLL found,
 * the library function supplied by @a funct is tried to be loaded and on
 * success for each such DLL, the callback function @a pCallback is called.
 *
 * @e Note: This function leaves all DLLs open! You should close them once you
 * don't need them anymore by calling DynamicLibraryClose() .
 *
 * @param dir - DLL directory
 * @param funct - entry function to be loaded for each DLL
 * @param pCallback - callback function which is called for each DLL found
 * @param pCustom - (optional) arbitrary data passed to the callback function
 * @throws Exception - if supplied directory cannot be opened
 * @returns amount of DLLs loaded successfully
 */
int DynamicLibrariesSearch(String dir, String funct, DynamicLibrariesSearchCallbackFunction* pCallback, void* pCustom = NULL) throw (Exception);

/**
 * Load the DLL given by @a filename . A DLL can be opened multiple times. A
 * reference count is used in this case.
 *
 * @param filename - DLL to be loaded
 * @returns handle to opened DLL or NULL on error
 */
void* DynamicLibraryOpen(String filename);

/**
 * Retrieve symbol (e.g. function or data structure) with name @a symbol of
 * DLL @a hDLL .
 *
 * @param hDLL - handle of DLL
 * @param symbol - name of symbol to be accessed
 * @returns pointer to requested symbol or NULL on error
 */
void* DynamicLibraryGetSymbol(void* hDLL, String symbol);

/**
 * Unload the DLL given by @a hDLL , previously opened by e.g.
 * DynamicLibrariesSearch() or DynamicLibraryOpen() . If the DLL was opened
 * multiple times, then just the reference count is decremented and finally when
 * the reference count reached zero, the library is freed from memory.
 *
 * @param hDLL - handle of DLL to be closed
 */
void DynamicLibraryClose(void* hDLL);

} // namespace LinuxSampler

#endif // LS_DYNAMIC_LIBRARIES_H
