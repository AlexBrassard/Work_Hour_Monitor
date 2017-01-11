/*
 *
 * Safe-utils Library  -  Header and code file.
 * Version 1.0
 *
 */

#ifndef LIB_SAFE_UTILS_HEADER_FILE
# define LIB_SAFE_UTILS_HEADER_FILE

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>

/*** Code ***/

/*
 * Copy at most dest_s-1 char from src to dest,
 * filling the buffer with zeros before copying the data.
 */
static inline char* s_strcpy(char *dest, char *src, size_t dest_s)
{
  size_t src_s = 0;
  
  if (!dest || !src || !dest_s){
    errno = EINVAL;
    return NULL;
  }

  if ((src_s = strlen(src)) > dest_s-1){
    errno = EOVERFLOW;
    fprintf(stderr, "%s: Source must be at most destination[n-1]\n\n", __func__);
    dest = NULL;
  }
  else {
    memset(dest, '\0', dest_s);
    memcpy(dest, src, src_s);
  }

  return dest;

} /* s_strcpy() */


/* Convert the double integer src into a dest_s-1 precision string dest. */
static inline char* s_ftoa(char *dest, double src, size_t dest_s)
{
  if (!dest || dest_s < 2) {
    errno = EINVAL;
    return NULL;
  }
  memset(dest, '\0', dest_s);
  if (snprintf(dest, dest_s-1, "%f", src) < 0) {
    fprintf(stderr, "%s: Failed to print the given number to the given buffer\n\n", __func__);
    return NULL;
  }
  dest[strlen(dest)] = '\0';

  return dest;

} /* s_ftoa() */


/* Converts the integer src into a dest_s-1 digits string dest. */
static inline char* s_itoa(char *dest, int src, size_t dest_s)
{
  if (!dest || dest_s < 2) {
    errno = EINVAL;
    return NULL;
  }
  memset(dest, '\0', dest_s);
  if (snprintf(dest, dest_s-1, "%d", src) < 0) {
    fprintf(stderr, "%s: Failed to print the given number to the given buffer\n\n", __func__);
    return NULL;
  }
  dest[strlen(dest)] = '\0';

  return dest;

} /* s_itoa() */



#endif /* LIB_SAFE_UTILS_HEADER_FILE */
