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

# define LS_ICASE 1

/*** Code ***/



/*
 * At least s_strcpy and s_strcmp are still vulnerable to
 * being fed one or more non-NULL terminated strings.
 * This must be fixed ASAP.
 */



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
  memcpy(dest, src, src_s);
  return dest;

} /* s_strcpy() */


/* 
 * Compares the first num bytes of s1 and s2 returning
 * an integer less than, equal to or bigger than 0 if 
 * s1 is found to be smaller than, equal of or bigger than s2.
 * Flags are optional parameters to pass to s_strcmp, right now only
 * LS_ICASE is available. when this flags is on the comparaisons are
 * case insensitives.
 * The only way to see if s_strcmp had an error is to verify errno 
 * after each calls.
 */
int s_strcmp(const char *s1, const char *s2, size_t num, int flags)
{
  size_t len1 = 0, len2 = 0;
  errno = 0;
  if (!s1 || !s2 || num == 0) {
    errno = EINVAL;
    return 0;
  }
  /* 
   * Make sure both string lenghts are bigger than or equal to num.
   * If any strings are smaller than num, modify num so that it's 
   * equal to the lenght of the smaller string.
   */
  if (num > (len1 = strlen(s1)) || num > (len2 = strlen(s2))){
    if (len1 < len2) num = len1;
    else num = len2;
  }
  
  if (flags & LS_ICASE){
    while(num-- != 0 && tolower(*s1) == tolower(*s2)){
      if (num == 0 || *s1 == '\0' || *s2 == '\0') break;
      s1++;
      s2++;
    }
    
    return (tolower(*s1) - tolower(*s2));
  }

  /* Default, no flags. */
  while (num-- != 0 && *s1 == *s2){
    if (num == 0 || *s1 == '\0' || *s2 == '\0') break;
    s1++;
    s2++;
  }
  return (*s1 - *s2);


} /* s_strcmp() */


/* 
 * Split src into dest[words] of at most word_s-1 characters each,
 * words are all null terminated and
 * separated by the word_delim character casted to an integer (space by default).
 * Set word_delim to -1 for default.
 * dest can hold at most dest_s-1 words.
 */
static inline char** s_split(char **dest, const char* src, size_t dest_s,
			     size_t word_s, int word_delim)
{
  size_t src_ind = 0, dest_ind = 0, word_ind = 0;
  int delimiter = ' ';                             /* Space delimited words unless told otherwise. */

  if (!dest || !src || src[0] == '\0'
      || !dest_s || !word_s){
    errno = EINVAL;
    return NULL;
  }
  if (word_delim >= 0) delimiter = word_delim;
  
  while(dest_ind < dest_s){
    /* 
     * Return successfuly if there's at least 1 word in the 
     * destination array and the next word is NULL. 
     */
    if (!dest[dest_ind]){
      if (dest_ind > 0)
	break;
      else{
	errno = ENODATA;
	fprintf(stderr, "%s: dest must contain at least one initialized char array.\n\n", __func__);
	return NULL;
      }
    }
    /* Clear the current word buffer. */
    memset(dest[dest_ind], '\0', word_s);

    /* Get a word. */
    while(1){
      if (src[src_ind] == '\0') {
	dest[dest_ind][word_ind] = '\0';
	return dest;
      }
      /* Finding a word that doesn't fit in a word buffer is an error. */
      if (word_ind >= word_s){
	errno = EOVERFLOW;
	fprintf(stderr, "%s: The word following \"%s\" does not fit in the given buffer size '%zu'\n\n",
		__func__, ((dest_ind > 0) ? dest[dest_ind-1] : "NO WORD REGISTERED"), word_s);
	return NULL;
      }
      /* Null terminate the current word when hitting the delimiter. */
      if (src[src_ind] == delimiter) {
	dest[dest_ind][word_ind] = '\0';
	word_ind = 0;
	++dest_ind;
	++src_ind;
	break;
      }
      /* Add the current src character to the current dest word. */
      dest[dest_ind][word_ind] = src[src_ind];
      ++word_ind;
      ++src_ind;
    }
  }

  return dest;
}


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
