#ifndef _Sofia_H_
#define _Sofia_H_

#ifndef _WIN32
#define __stdcall
#endif

#define SofiaAPI __stdcall

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An placement for any API function
 */
typedef void* TSofiaFunction;

/**
 * A prototype of function "GetSofiaFunction" that provides a unified interface for obtaining any other interface function.
 * @param name is the name of the requested function
 * @param nLength is the length of the name string
 * @return is the requested function
 */
typedef TSofiaFunction (SofiaAPI *TGetSofiaFunction)(const char* name, int nLength);

/**
 * A protype of the function "InitializeModule" that any module should implement 
 * @param func is the function for creating any module available at the system
 * @
 */
typedef bool (SofiaAPI *TInitializeModuleFunc)( TGetSofiaFunction func );
/**
 * A protype of the function "GetModuleDescription" that any module should implement 
 * @return the pointer to a string containing a json array 
 * with the maps from type-name pair to the address of the appropriate CreateModule function.
 */
typedef const char* (SofiaAPI *TGetModuleDescriptionFunc)();

///////////////////////////////////
// A number of available functions
///////////////////////////////////

/**
 * Data type for a module interface
 */
typedef void* TModule;

/**
 * A function that creates modules by name
 * @param type is the string with the module type
 * @param tLength is the length of the type string
 * @param name is the string with the module name
 * @param nLength is the length of the name string
 * @return the pointer to the requested module interface
 */
typedef TModule (SofiaAPI *TCreateModuleFunc)( const char* type, int tLegnth, const char* name, int nLength, const char* jsonParams, int jLength );
/**
 * "CreateModuleFunc" name
 */
static const char CreateModuleFunction[] = "CreateModuleFunc";

#ifdef __cplusplus
}
#endif

#endif
