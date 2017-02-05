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
# include <ctype.h>
# include <errno.h>

# define SPACE  ' '
# define USCORE '_'

/* s_strcmp flags. */
enum ls_cmp_flags {
  LS_ICASE  = 0x01,          /* Case insensitive comparaison. */
  LS_USPACE = 0x02,          /* Spaces and underscores are treated the same. */

}; 

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
    return NULL;
  }
  memset(dest, '\0', dest_s);
  memmove(dest, src, src_s);
  return dest;

} /* s_strcpy() */


/* 
 * Compares the first num bytes of s1 and s2 returning
 * an integer less than, equal to or bigger than 0 if 
 * s1 is found to be smaller than, equal to or bigger than s2.
 *
 * Flags are optional parameters to pass to s_strcmp:
 * LS_ICASE  makes comparaisons case-insensitive;
 * LS_USPACE makes space character and underscore character effectively the same character;
 * (ex: "Ville Montreal" and "Ville_Montreal" would return 0, match).
 *
 * The only way to see if s_strcmp had an error is to verify errno 
 * after each calls.
 *
 * Passing a 0 argument to num isn't an error, but makes s_strcmp() behaves like 
 * the standard strstr() so beware!
 */
static inline int s_strcmp(const char *s1, const char *s2, size_t numof_bytes, int flags)
{
  size_t num = numof_bytes;

  errno = 0;
  if (!s1 || !s2) {
    errno = EINVAL;
    return 0;
  }
  while (num-- != 0){
    if (num == 0 || *s1 == '\0' || *s2 == '\0') break;
    if ((*s1 == *s2) 
	|| ((flags & LS_ICASE) && (tolower(*s1) == tolower(*s2)))
	|| ((flags & LS_USPACE) && ((*s1 == SPACE || *s1 == USCORE)
				    && (*s2 == SPACE || *s2 == USCORE)))){
      ++s1;
      ++s2;
    }
    else break;
  }
  if (flags & LS_ICASE) return (tolower(*s1) - tolower(*s2));
  return (*s1 - *s2);
  
} /* s_strcmp() */


/* 
 * Split src into dest[words] of at most word_s-1 characters each,
 * words are all null terminated and
 * separated by the word_delim character casted to an integer (space by default).
 * Set word_delim to -1 for default.
 * dest can hold at most dest_s-1 words.
 */
static inline char** s_split(char **dest, const char *src, size_t dest_s,
			     size_t src_s, size_t word_s, int word_delim)
{
  size_t dest_ind = 0, word_ind = 0;

  if (!dest || !src || src[0] == '\0'
      || !dest_s || !word_s){
    errno = EINVAL;
    return NULL;
  }
  if (word_delim == -1) word_delim = SPACE;

  while(dest_s-- != 0){
    if (src_s == 0 || dest_s == 0 || !dest[dest_ind]) break;
    memset(dest[dest_ind], '\0', word_s);
    for (word_ind = 0; word_ind < word_s-1; word_ind++, src++){
      if (*src == '\0' || src_s-- == 0) break;
      if (*src == word_delim){
	++src;
	break;
      }
      dest[dest_ind][word_ind] = *src;
    }
    dest[dest_ind++][word_ind] = '\0';
  }
  return dest;

} /* s_split() */


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

	       
#undef SPACE
#undef USCORE
#endif /* LIB_SAFE_UTILS_HEADER_FILE */
