//
//  UUID.h
//  RushHour
//
//  Created by Mark Pauley on 8/10/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//
//  Utilities for dealing with standard 16 byte UUIDs
//

#ifndef UTIL_UUID_h
#define UTIL_UUID_h

#ifdef __cplusplus
extern "C" {
#endif
  
typedef struct UUIDBytes {
    unsigned char bytes[16];
} UUIDBytes, *UUIDBytesRef;

/** Fills the UUID struct in with the UUID in the given string.
 *
 * @param uuid    pointer to UUID bytes
 * @param uuidStr input string of the form "%08x-%04x-%04x-%04x-%012x" + '\0'
 *
 * @return 0 on success, -1 on failure.
 */
int UUIDBytesFromString(UUIDBytesRef uuid, const char* uuidStr);

/** Returns a statically allocated string containing the canonical string representation
 *  of the given UUID.
 *
 * @param uuid    pointer to UUID bytes
 * 
 * @return  Returns a statically allocated string representing UUID bytes of
 * the form "%08x-%04x-%04x-%04x-%012x" + '\0'. (36 bytes + NULL terminator = 37 bytes).
 *
 * @discussion The contents of this string _will_ change when this function is next called.
 */
const char* StringFromUUIDBytes(const UUIDBytesRef uuid);

/** Compares two UUIDs
 *
 * @param first   pointer to UUID bytes
 * @param second  pointer to UUID bytes
 *
 * @return  0 if the UUIDs are equal, or 
 * the difference between the first byte that differs (first[k] - second[k]).
 */
int UUIDCompare(const UUIDBytesRef first, const UUIDBytesRef second);

/** Copy UUID bytes
 *
 * @param destination   pointer to location where UUID bytes will be copied
 * @param source        pointer to source UUID bytes
 */
void UUIDCopy(UUIDBytesRef destination, const UUIDBytesRef source);
  
/** Return an empty UUID
 *
 * @return  static reference to an empty UUID.
 */
const UUIDBytesRef UUIDEmpty();

/** Validates that a string represents a UUID
 *
 * @param uuidStr input string of the form "%08x-%04x-%04x-%04x-%012x" + '\0'
 *
 * @return 1 if uuidStr represents a UUID, 0 if not
 */
int IsValidUUIDString(const char* uuidStr);

#define UUIDClear(uuidBytesRef) UUIDCopy((uuidBytesRef), UUIDEmpty())

#ifdef __cplusplus
} // export C
#endif


#endif
